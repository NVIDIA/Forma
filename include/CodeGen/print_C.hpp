//****************************************************************************//
//* Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.             *//
//*                                                                          *//
//* Redistribution and use in source and binary forms, with or without       *//
//* modification, are permitted provided that the following conditions       *//
//* are met:                                                                 *//
//*  * Redistributions of source code must retain the above copyright        *//
//*    notice, this list of conditions and the following disclaimer.         *//
//*  * Redistributions in binary form must reproduce the above copyright     *//
//*    notice, this list of conditions and the following disclaimer in the   *//
//*    documentation and/or other materials provided with the distribution.  *//
//*  * Neither the name of NVIDIA CORPORATION nor the names of its           *//
//*    contributors may be used to endorse or promote products derived       *//
//*    from this software without specific prior written permission.         *//
//*                                                                          *//
//* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY     *//
//* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE        *//
//* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR       *//
//* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR        *//
//* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,    *//
//* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,      *//
//* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR       *//
//* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY      *//
//* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT             *//
//* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    *//
//* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     *//
//****************************************************************************//
#ifndef __PRINT_C_HPP__
#define __PRINT_C_HPP__

#include <cstdio>
#include <sstream>
#include <cassert>
#include <stack>
#include <iostream>
#include <fstream>
#include <cmath>
#include <map>
#include "AST/parser.hpp"
#include "ASTVisitor/convert_boundaries.hpp"
#include "program_opts.hpp"
#include "CodeGen/CodeGen.hpp"


/** Struct to represent var information for C **/
struct c_var_info{
  const domain_node* expr_domain;
  std::string symbol_name;
  data_types elem_type;
  domain_node* domainEdges;
  c_var_info
  (const std::string& name, const domain_node* ed, const data_types& et):
    expr_domain(ed),
    symbol_name(name),
    domainEdges(NULL)
  {
    elem_type.assign(et);
  }
  virtual ~c_var_info() { }
};

/** Struct to represent the symbol information associated with an expression **/
struct c_symbol_info{
  c_var_info* var;
  const domainfn_node* scale_domain;
  domain_node* offset_domain;
  std::string field_name;
  bdy_info* bdy_condn; ///Used only for stencil functions
  c_symbol_info
  (c_var_info* mapped_var, const domainfn_node* sd, domain_node* od,
   const std::string& fn, bdy_info* bdy = NULL):
    var(mapped_var),
    scale_domain(sd),
    offset_domain(od),
    field_name(fn)
  {
    if( bdy )
      bdy_condn = new bdy_info(bdy->type,bdy->value);
    else
      bdy_condn = NULL;
  }
  virtual ~c_symbol_info() {
    if( bdy_condn != NULL )
      delete[] bdy_condn;
  }
};

/** Symbol table for CUDA code generation **/
struct c_symbol_table{
  std::map<const vector_expr_node*,c_symbol_info*> symbol_table;
  virtual void
  AddSymbol
  (const vector_expr_node* defn, c_var_info* curr_var,
   const domainfn_node* scale_domain, domain_node* offset_domain,
   const std::string& field_name, bdy_info* bdy = NULL){
    c_symbol_info* new_symbol =
      new c_symbol_info(curr_var,scale_domain,offset_domain,field_name,bdy);
    symbol_table.insert(std::make_pair(defn,new_symbol));
  }
  virtual c_symbol_info*
  NewSymbol
  (c_var_info* curr_var, const domainfn_node* scale_domain,
   domain_node* offset_domain, const std::string& field_name,
   bdy_info* bdy = NULL){
    c_symbol_info* new_symbol =
      new c_symbol_info(curr_var,scale_domain,offset_domain,field_name,bdy);
    return new_symbol;
  }
  void ReplaceSymbol(const vector_expr_node* defn, c_symbol_info* new_symbol){
    RemoveSymbol(defn);
    symbol_table.insert(std::make_pair(defn,new_symbol));
  }
  bool IsPresent(const vector_expr_node* defn){
    if( symbol_table.find(defn) == symbol_table.end() )
      return false;
    else
      return true;
  }
  c_symbol_info* GetSymbolInfo(const vector_expr_node* defn)  {
    std::map<const vector_expr_node*,c_symbol_info*>::iterator it =
      symbol_table.find(defn);
    assert
      ((it != symbol_table.end()) &&
       "During Code-generation, Couldnt find a symbol within the symbol table");
    return it->second;
  }
  void RemoveSymbol(const vector_expr_node* defn) {
    std::map<const vector_expr_node*,c_symbol_info*>::iterator
      find_expr_symbol = symbol_table.find(defn);
    if( find_expr_symbol != symbol_table.end() ){
      delete find_expr_symbol->second;
      symbol_table.erase(find_expr_symbol);
    }
  }
  virtual ~c_symbol_table() {
    for( std::map<const vector_expr_node*,c_symbol_info*>::iterator it =
           symbol_table.begin() ; it != symbol_table.end() ; it++ ){
      if( it->second->offset_domain )
        delete it->second->offset_domain;
      delete it->second;
    }
    symbol_table.clear();
  }
};

///Symbol description for scalars within a stencil functions
struct c_scalar_info{
  std::string name;
  c_scalar_info(const std::string& set_name){
    name = set_name;
  }
};


typedef std::map<std::string,c_scalar_info> c_scalar_table;
typedef std::map<std::string,c_scalar_info>::iterator c_scalar_symbol;


///C-iterator descriptor
struct c_iterator{
  std::string name;
  const parametric_exp* lb;
  const parametric_exp* ub;
  bool unit_trip_count;

  /// Constructor
  c_iterator
  (std::string name, const parametric_exp* lb, const parametric_exp* ub);

   /// Destructor
  ~c_iterator() {
    if( lb)
      delete lb;
    if( ub)
      delete ub;
  }
  bool is_unit_trip_count() const { return unit_trip_count; }
};


/** Main class for C-Code-generator  */
class PrintC  : public CodeGen
{
public:

  ///Constructor
  PrintC(bool gen_omp = true, bool gen_icc = false);

  ///Public method to call the code generator on the entire program
  virtual void GenerateCode(const program_node*,const program_options& ) ;

  ///Destructor
  virtual ~PrintC();
protected:
  ///Variables for the single malloc option
  std::string globalMemVar;
  std::string globalMemOffset;

  bool init_zero; ///Initialize all buffers to zero on Init
  bool use_single_malloc ;

  stringBuffer* output_buffer;
  stringBuffer* deallocate_buffer;
  stringBuffer* host_allocate;

  /// Buffer for the malloc prologue which computes the size of the malloc
  stringBuffer* host_allocate_size;

  ///symbol table for scalars within stencil functions
  c_scalar_table scalar_symbols;

  ///Iterator stack
  std::deque<c_iterator*> iters;

  ///Variables used in the code
  std::deque<c_var_info*> def_vars;

  /// Options that control unrolling
  bool generate_unroll_code;
  std::deque<int> unroll_factors;

  ///Creating unique iterators
  std::string get_new_iterator(stringBuffer*);
  ///Counter for number of iterator variables used (also the current iterator
  ///available)
  int niter_variables;

  ///Creating unique variables for intermediates
  std::string get_new_var(){
    static int var_qualifier = 0;
    std::stringstream new_string;
    new_string << "__var_" << var_qualifier++ << "__";
    return new_string.str();
  }

  ///Creating unique temp_variables for intermediates
  std::string get_new_temp_var(const data_types&, stringBuffer*);
  ///Counter for number of temp variables used (also the current available
  ///tempvariable
  int ntemp_variables;

  /// Create a new symbol
  virtual c_symbol_info* getNewSymbol
  (c_var_info* curr_var, const domainfn_node* scale_domain,
   domain_node* offset_domain, const std::string& field_name,
   bdy_info* bdy = NULL ){
    return
      new c_symbol_info(curr_var,scale_domain,offset_domain,field_name,bdy);
  }

  /// \brief printForHeader Function to print header of for loops
  /// \param lb Lower bound of the loop
  /// \param ub upper bound of the loop
  /// \param unroll_factor to use for this iterator
  /// \param isInnerMost Boolean variable if true implies this is the inner most
  /// loop to be generated - potentially vectorized
  /// \return New iterator created by this call
  c_iterator* printForHeader
  (const parametric_exp* lb,const parametric_exp* ub, int unroll_factor,
   bool isInnerMost);

  /// \breif printForFooter Function to print the footer of the for loop
  void printForFooter();

  /// function to malloc a variable
  virtual c_var_info* printMalloc
  (std::string,data_types,const domain_node*);
  ///Function to initialize the value of a scalar variable
  virtual c_var_info* init_value_var
  (std::string& curr_value, const data_types& curr_data_type){
    return new c_var_info(curr_value,NULL,curr_data_type);
  }

  /// \breif FindDefnSymbol Method to get the symbol for a particular expression
  /// \param curr_expr the expression
  /// \param fn_bindings Current symbol table
  /// \return the symbol to use for the given expression
  c_symbol_info* FindDefnSymbol
  (const vector_expr_node* curr_expr, c_symbol_table& fn_bindings) const;

  /// \brief compute_loop_domain Compute final loop domain adjust for scaling on
  /// output side be generated for its computation
  /// \param curr_domain The base loop domain
  /// \param scale_domain The scaling factor used on the LHS
  /// \return the actual loop domain to be used
  domain_node* compute_loop_domain
  (const domain_node* curr_domain, const domainfn_node* scale_domain);

  /// \brief print_remainder_loop_header
  void print_remainder_loop_header
  (std::string& iterator, parametric_exp* ub, int unrollFactor);

  /// \brief print_remainder_loop_footer
  void print_remainder_loop_footer();

  /// \brief print_domain_loops_header Method to print loops to iterate through
  /// a domain when the output has a scale domain
  /// \param curr_domain Domain to iterate over
  /// \param fn_bindings Current symbol table
  /// \param input_exprs The vector exprs that are input(not used for C codegen)
  /// \param output_var Buffer which stores the output(not used for C codegen)
  /// \param scale_domain the scaling factor used on the LHS
  /// \param curr_unroll_factors Unroll factors used in kernel
  virtual int print_domain_loops_header
  (const domain_node* curr_domain,std::deque<c_symbol_info*>& input_exprs,
   const c_var_info* output_var,const domainfn_node* scale_domain,
   const std::deque<int>& curr_unroll_factors);

  /// \brief print_domain_loops_footer Method to print footer of loops generated
  /// by print_domain_loops_header
  /// \param loop_domain The loop domain
  /// \param curr_unroll_factors Unroll factors used in kernel
  virtual void print_domain_loops_footer
  (const domain_node* loop_domain, const std::deque<int>& curr_unroll_factors);

  virtual void print_array_access
  ( const c_var_info*, const std::deque<std::string>&, std::stringstream& );
  void check_index_value
  (std::string& curr_index, std::string& is_bdy_var,
   parametric_exp* lb, parametric_exp* ub, const bdy_info* curr_bdy);


  ///Simple access to point in a domain
  void print_domain_point(const c_var_info*, std::stringstream& );
  ///Access to a point with given offsets
  void print_domain_point
  (const c_var_info*, std::stringstream&, const domain_node*);

  ///Access an interpolated point , With interpolations
  void print_interpolate_point
  (const c_var_info*, const domainfn_node*, std::stringstream& );
  ///Access an interpolated point within a subdomain
  void print_interpolate_point
  (const c_var_info*,const domainfn_node*,
   const domain_node*, std::stringstream& );

  ///Access an stencil point With interpolations
  void print_stencil_point
  (const c_var_info*, const domainfn_node*, std::stringstream&,
   const bdy_info* curr_bdy_condn = NULL );
  ///Access an stencil point within a subdomain  With interpolations
  void print_stencil_point
  (const c_var_info*, const domainfn_node* , const domain_node*,
   std::stringstream&, const bdy_info* curr_bdy_condn = NULL );


  ///Access an extrapolated point on the LHS
  void print_extrapolate_point
  (const c_var_info*, const domainfn_node*, std::stringstream& );
  ///Access an extrapolated point and then add an offset on the LHS
  void print_extrapolate_point
  (const c_var_info*, const domainfn_node*, const domain_node*,
   std::stringstream&);


  /**
     Function to create a symbol for the output of the RHS
     input Arguments :
     1) const vector_expr_node* the rhs expr,
     2) const domain_node* : the domain of the expression
     2) std::stringstream : the stringBuffer to be used
     3) c_symbol_table& : the current symbol table
     4) const domainfn_node* : the scale fn to be used on the lhs
     5) const domain_node* : the offset to be used on the lhs
     6) string field_name : the field_name used on the lhs
   */
  void init_rhs
  (const vector_expr_node*, const domain_node* expr_domain, std::stringstream& ,
   c_symbol_table& , const domainfn_node* scale_domain = NULL ,
   const domain_node* offset_domain = NULL);

  /** Function to print the rhs variable
      Arguments :
      1) const vector_expr_node* : the current expression
      2) stringstream& : StringBuffer to be used
      3) c_symbol_table& the current symbol table
      4) const domain_node* : sub-domain of the expression used
      5) const domainfn_node* : if interpolation is used on the rhs, the scaling
      function
  */
  void print_rhs_var
  (const vector_expr_node*, std::stringstream&, c_symbol_table&,
   const domain_node*, const domainfn_node*);

  /** Function to print an assignment to domain point
      Arguments:
      1) const symbol_info& : symbol table entry of the LHS variable
      2) string : rhs_expr
      3) stringstream& : StringBuffer to be used
  */
  // void print_lhs_var(const c_symbol_info*, std::stringstream&);
  void print_assignment_stmt
  (const c_symbol_info*, std::string&, std::stringstream&);

  ///Method to redirect printing expressions
  std::string print_expr
  (const expr_node*,c_symbol_table&, bool handle_bdys = false);

  ///Methods to print the different expression types, returns a
  ///variable which refers to the result of the expression printed
  std::string print_mathfn_expr(const math_fn_expr_node*, c_symbol_table&,
                                bool handle_bdys = false);
  std::string print_ternary_exp(const ternary_expr_node*,c_symbol_table&,
                                bool handle_bdys = false);
  std::string print_expr_op(const expr_op_node*, c_symbol_table&,
                            bool handle_bdys = false);

  /// \brief print_stencil_op_helper Helper function to print stencil operations
  /// \param curr_symbol The symbol
  /// \param scale_fn Scaling function to be used
  /// \param base_data_type The underlying data type of the expression
  /// (different from data type if cotains field access
  /// \param field_num the filed number accessed
  /// \param handle_bdys
  /// \return string with the stencil access
  virtual std::string print_stencil_op_helper
  (const c_symbol_info* curr_symbol, const domainfn_node* scale_fn,
   const data_types& base_data_type, int field_num, bool handle_bdys);

  std::string print_stencil_op(const stencil_op_node*, c_symbol_table&,
                                       bool handle_bdys = false);
  std::string print_value_expr(const expr_node*);
  std::string print_id_expr(const id_expr_node*);
  std::string print_array_expr(const array_access_node*, c_symbol_table& ,
                               bool handle_bdys = false );
  void print_pt_stencil
  (const pt_struct_node*,c_symbol_table&, std::deque<std::string>&,
   bool handle_bdys = false);

  ///Method to inline a stencil function, returns the variable name of
  ///the result at a point
  void print_stencil_fn
  (const stencilfn_defn_node*, c_symbol_table&, std::deque<std::string>&,
   bool handle_bdys);

  ///Method used to access the value of vector expressions already
  ///computed
  std::string read_vector_expr
  (const vector_expr_node*,c_symbol_table&, const domain_node*,
   const domainfn_node*);

  /// \brief init_arg_info Method to initialize an argument to the function
  /// \param curr_argument Current argument being evaluated
  /// \param arg_bdy_condn Boundary condition used with the argument
  /// \param curr_param The parameter bound to the argument
  /// \param input_expr_symbols List of symbols that are to be mapped to
  /// parameters of the called function
  /// \param arg_domains Vector of domains of the arguments used later in
  /// print_fn_body
  virtual void init_arg_info
  (const vector_expr_node* curr_argument, bdy_info* arg_bdy_condn,
   const vector_defn_node* curr_param, c_symbol_table& fn_bindings,
   std::deque<c_symbol_info*>& input_expr_symbols,
   std::deque<const domain_node*>& arg_domains);

  /// \brief init_fn_args Method to initialize all function arguments
  /// \param curr_fn current function application being handled
  /// \param fn_bindings Current symbol table
  /// \param arg_symbols Symbols to be used for the parameters
  /// \param arg_domains domains of the arguments
  bool init_fn_args
  (const fnid_expr_node* curr_fn, c_symbol_table& fn_bindings,
   std::deque<c_symbol_info*>& arg_symbols,
   std::deque<const domain_node*>& arg_domains);

  /// \brief deleteArgSymbols Function to delete the symbols for arguments
  /// generated by init_fn_args
  /// \param arg_symbols
  virtual void deleteArgSymbols(std::deque<c_symbol_info*>& arg_symbols);

  /// Function that expands a domain by the given offsets
  void ExpandDomain
  (domain_node* loop_domain, const std::deque<offset_hull>& offsets) const;

  /// \brief print_patch_untiled Function to print untiled code for each patch
  /// while handling stencil function applications
  /// \param curr_fn the function expression being handled, is a stencil
  /// function application
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol the buffer to which the result is to be written
  /// \param loop_domain the domain for the current patch
  /// \param is_bdy Boolean to specify if this is a boundary patch
  /// \param curr_unroll_factors Unroll factors used in kernel
  /// \param unroll_loops Boolean to generate unrolled code
  void print_patch_untiled
  (const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   c_symbol_info* output_symbol, domain_node* loop_domain,
   bool is_bdy, bool unroll_loops, std::deque<int>& curr_unroll_factors);

  /// \brief getCurrUnrollFactors Setup the unroll factors to use in the code
  /// generation
  /// \param ndims Dimensionality of the loop to be generated
  /// \param curr_unroll_factors List of unroll factors, this is updated
  void getCurrUnrollFactors(int ndims, std::deque<int>& curr_unroll_factors);

  /// \brief print_unrolled_code_body Function to print body of the untiled code
  /// using unrolling
  /// \param loop_dim Dimension of the iterator unrolled
  /// \param num_loop_dims Total loop dimensionality
  /// \param unroll_factors Current list of unroll factors
  /// \param curr_fn the function expression being handled, is a stencil
  /// function application
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol the buffer to which the result is to be written
  /// \param loop_domain the domain for the current patch
  /// \param is_bdy Boolean to specify if this is a boundary patch
  virtual void print_unrolled_code_body
  (int loop_dim, int num_loop_dims, std::deque<int>& unroll_factors,
   const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   c_symbol_info* output_symbol, bool is_bdy);

  /// \brief print_unrolled_code Function to print untiled code using unrolling
  /// \param curr_dim Dimension of the unrolled code being generated
  /// \param loop_dim Max DImensionality of the loop to unroll
  /// \param curr_fn the function expression being handled, is a stencil
  /// function application
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol the buffer to which the result is to be written
  /// \param loop_domain the domain for the current patch
  /// \param is_bdy Boolean to specify if this is a boundary patch
  /// \param curr_unroll_factors Current list of unroll factors
  virtual void print_unrolled_code
  (int curr_dim, int loop_dim, const fnid_expr_node* curr_fn,
   std::deque<c_symbol_info*>& input_exprs, c_symbol_info* output_symbol,
   domain_node* loop_domain, bool is_bdy, std::deque<int>& curr_unroll_factors);

  /// \brief printPatches Recursive call that splits the domain including edges
  /// (outer domain) and domain excluding domain (inner domain) into rectangular
  /// patches and generated code for those
  /// \param curr_argument Current stencil function argument being handled
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol Buffer to where to write the result
  /// \param loop_domain The loop domain to be used, filled during recursion
  /// \param outer_domain List of ranges that describe the outer domain
  /// \param inner_domain List of ranges that describe the inner domain
  /// \param dim Current depth of the recursion (same as domain dimension being
  /// processed)
  /// \param ndims The dimensionality of the loop
  /// \param isBdy Boolean to say if the recursion is in a boundary patch
  void printPatches
  (const fnid_expr_node* curr_argument, std::deque<c_symbol_info*>& input_exprs,
   c_symbol_info* output_symbol, domain_node* loop_domain,
   const domain_node* outer_domain, const domain_node* inner_domain,
   int dim, int ndims, bool isBdy);

  /// \brief print_stencilfn_body Method to generate the body of a stencil
  /// function , i.e. basic block that generates the stencil computation
  /// \param curr_fn the stencil function application expression
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol the symbol for the buffer to write to
  /// \param is_bdy boolean to say if boundary conditions rae to be generated
  void print_stencilfn_body
  (const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   c_symbol_info* output_symbol, bool is_bdy);

  /// \brief print_vectorfn_body Method to generate the body of a vector
  /// function
  /// \param curr_fn the stencil function application expression
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param arg_domains the size of the input arguments
  /// \param curr_output_symbol the symbol corresponding to the return
  /// expression of the function
  virtual void print_vectorfn_body
  (const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   std::deque<const domain_node*>& arg_domains,
   c_symbol_info* curr_output_symbol);

  /// \brief print_fn_body Method to generate the body of a stencil or vector
  /// function
  /// \param curr_fn current function application being handled
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param arg_domains the domains of each of the arguments passed
  /// \param curr_output_symbol buffer to which the result is written to
  void print_fn_body
  (const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   std::deque<const domain_node*>& arg_domains,
   c_symbol_info* curr_output_symbol);

  /// \brief print_fnid_expr Method to generate code for fn application exprs
  /// \param curr_fn current expressions
  /// \param fn_bindings Current symbol table
  void print_fnid_expr
  (const fnid_expr_node* curr_fn, c_symbol_table& fn_bindings);


  ///Methods used to precompute values of vector expressions
  void print_vector_expr(const vector_expr_node*,c_symbol_table&);
  void print_vector_defn(const vector_defn_node*,c_symbol_table&);
  void print_domainfn_expr(const vec_domainfn_node*,c_symbol_table&);
  void print_compose_expr(const compose_expr_node*,c_symbol_table&);
  void print_make_struct(const make_struct_node*,c_symbol_table&);
  void print_vec_id_expr(const vec_id_expr_node*,c_symbol_table&);
  void print_stmt(const stmt_node*,c_symbol_table&,bool);

  /// \brief print_vectorfn_body_helper Function that implements the main parts
  /// of printing a vector function body
  /// \param curr_fn the function whose body is being printed
  /// \param fn_bindings the symbol table to use
  /// \param is_inlined Boolean that specifies if curr_fn is to be treated as
  /// being inlined
  void print_vectorfn_body_helper
  (const vectorfn_defn_node* curr_fn, c_symbol_table& fn_bindings,
   bool is_inlined);

  virtual void print_program_body
  (const program_node*, c_symbol_table&,std::string&);

  void print_header_info(FILE*);

  ///Method to print the string to stdout
  virtual void print_string
  (const program_node*, const program_options&, std::string& );

private:
  void print_pointer(int, std::stringstream&);

  /// Code-generation options
  bool generate_affine;
  int supported_affine_dim;
  bool generate_omp_pragmas;
  bool generate_icc_pragmas;

};

#endif
