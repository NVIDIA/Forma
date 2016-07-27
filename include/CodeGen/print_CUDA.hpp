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
#ifndef __PRINT_CUDA_HPP__
#define __PRINT_CUDA_HPP__

#include "CodeGen/print_C.hpp"

int get_channel_size(basic_data_types);

enum texture_type{
  TEX_NONE,
  TEX_UCHAR,
  TEX_SHORT,
  TEX_INT,
  TEX_FLOAT,
  TEX_UCHAR2,
  TEX_SHORT2,
  TEX_INT2,
  TEX_FLOAT2,
  TEX_UCHAR4,
  TEX_SHORT4,
  TEX_INT4,
  TEX_FLOAT4,
  TEX_NTYPES
};


#define MAX_GRID_DIM 3

/** Struct to represent var information for CUDA **/
struct cuda_var_info : public c_var_info
{
  std::string pitch;
  cuda_var_info
  (const std::string& name, const domain_node* ed, const data_types& et):
    c_var_info(name,ed,et)
  {
    pitch = "";
  }
  ~cuda_var_info()  { }
};

struct texture_reference_info{
  const int ndims;
  data_types elem_type;
  texture_type texture_data_type;
  std::string name;
  static int counter;
  cuda_var_info* curr_bound_var;
  texture_reference_info(int nd, data_types dt, texture_type texel_type):
    ndims(nd)
  {
    elem_type.assign(dt);
    texture_data_type = texel_type;
    curr_bound_var = NULL;
    std::stringstream name_stream;
    name_stream << "__texture_ref_" << counter++ << "__" ;
    name = name_stream.str();
  }
};


/** Symbol information for CUDA **/
struct cuda_symbol_info : public c_symbol_info {
  texture_reference_info* texture_info;
  bool is_texture_normalized;
  cuda_symbol_info
  (cuda_var_info* mapped_var, const domainfn_node* sd, domain_node* od,
   const std::string& fn, bdy_info* bdy = NULL):
    c_symbol_info(mapped_var,sd,od,fn,bdy) {
    texture_info = NULL;
    is_texture_normalized = false;
  }
  ~cuda_symbol_info() { }
};


/** Symbol Table for CUDA **/
struct cuda_symbol_table : public c_symbol_table{
  void AddSymbol
  (const vector_expr_node* defn, c_var_info* curr_var,
   const domainfn_node* scale_domain, domain_node* offset_domain,
   const std::string& field_name, bdy_info* bdy = NULL){
    assert(dynamic_cast<cuda_var_info*>(curr_var));
    cuda_symbol_info* new_symbol =
      new cuda_symbol_info
      (static_cast<cuda_var_info*>(curr_var),scale_domain,
       offset_domain,field_name, bdy);
    symbol_table.insert(std::make_pair(defn,new_symbol));
  }
  c_symbol_info* NewSymbol
  (c_var_info* curr_var, const domainfn_node* scale_domain,
   domain_node* offset_domain, const std::string& field_name,
   bdy_info* bdy = NULL){
    cuda_symbol_info* new_symbol =
      new cuda_symbol_info
      (static_cast<cuda_var_info*>(curr_var),scale_domain,
       offset_domain,field_name,bdy);
    return new_symbol;
  }
};


class PrintCUDA : public PrintC {

public:

  PrintCUDA() : PrintC(false,false) {
    enable_texture = false;
  };

  ~PrintCUDA() {
    for( std::deque<texture_reference_info*>::iterator it =
           texture_references.begin() ; it != texture_references.end(); it++)
      delete (*it);
    delete function_header;
  }

  void GenerateCode(const program_node*, const program_options& );

private:

  bool is_number(const std::string&) const ;
  bool is_duplicate(const std::deque<std::string>&, const std::string&);
  texture_type get_texture_type(const data_types& type) const;

  /// \brief isTextureAmenable Function to check that a given expression can be
  /// mapped to texture
  /// \param curr_argument Argument being checked for mapping to texture
  /// \param arg_bdy_condn Boundary condition used
  /// \param curr_param Parameter to which the argument is matched
  /// \result true if it can be mapped to textures, false otherwise
  bool isTextureAmenable
  (const vector_expr_node* curr_expr, bdy_info* arg_bdy_condn,
   const vector_defn_node* curr_param) const;

  std::string get_texture_type_string(int);
  std::string get_channel_string(int curr_type);
  void init_channels();

  std::string get_new_kernel_var(){
    static int kernel_num = 0;
    std::stringstream curr_stream;
    curr_stream << "__kernel_" << kernel_name_append <<  kernel_num++ << "__" ;
    curr_kernel_name = curr_stream.str();
    return curr_stream.str();
  }

  std::string kernel_name_append;
  std::string curr_kernel_name;
  std::string warp_size;
  std::string sharedMemVar;
  std::string sharedMemOffset;
  std::string max_shared_mem_size;

  stringBuffer kernel_launch;
  stringBuffer* function_header;

  /// \brief InitializeOutputBlockSizeParams Function to initialize the "actual"
  /// kernel block size, i.e., the output block size computed by the kernel
  /// \param grid_dim Dimensionality of the kernel launch
  /// \param buffer Buffer to which to write to
  /// \param curr_unroll_factors Unroll factors used in the kernel
  void InitializeOutputBlockSizeParams
  (int grid_dim, stringBuffer* buffer,
   const std::deque<int>& curr_unroll_factors);

  /// \brief print_block_loop Function to generate iterators corresponding to
  /// the loop distributed on the kernel grid
  /// \param iterator Iterator name to be used
  /// \param curr_dim the grid dimension
  /// \param grid_dim Dimensionality of the grid
  /// \param curr_lb Lower bound of the loop
  /// \param curr_ub Upper bound of the loop
  /// \param unrollFactor Factor by which this launch dimension is unrolled,
  /// forced to be 1 if isTiled is ture
  void print_block_loop
  (std::string& iterator, int curr_dim, int grid_dim, parametric_exp* lb,
   parametric_exp* ub, int unrollFactor);

  /// Create a new cuda symbol
  c_symbol_info* getNewSymbol
  (c_var_info* curr_var, const domainfn_node* scale_domain,
   domain_node* offset_domain, const std::string& field_name,
   bdy_info* bdy = NULL){
    assert(dynamic_cast<cuda_var_info*>(curr_var));
    return new cuda_symbol_info
      (static_cast<cuda_var_info*>(curr_var),scale_domain,offset_domain,
       field_name,bdy);
  }

  /// \brief print_kernel_signature Function to generate the signature of the
  /// kernel
  /// \param input_exprs List of symbols that represents parameters passed
  /// to the kernel
  /// \param output_var Variable where the output of the launch is to be stored
  /// \param kernel_name
  void print_kernel_signature
  (std::deque<c_symbol_info*>& input_exprs, const c_var_info* output_var,
   const std::string& kernel_name);

  /// \brief print_kernel_header Function to print the header code of a kernel
  /// \param grid_dim the kernel launch dimension
  /// \param curr_unroll_factors Unroll factors used in the kernel
  void print_kernel_header
  (int grid_dim, const std::deque<int>& curr_unroll_factors);

  /// \brief print_kernel_launch Function to generate the launch of the kernel
  /// \param grid_dim Dimensionality of the kernel launch
  /// \param curr_domain the current domain evaluated
  /// \param input_exprs List of symbols that represents parameters passed
  /// to the kernel
  /// \param output_var Variable where the output of the launch is to be stored
  /// \param kernel_name
  /// \param curr_unroll_factors Unroll factors used in the kernel
  void print_kernel_launch
  (int grid_dim, const domain_node* curr_domain,
   std::deque<c_symbol_info*>& input_exprs, const c_var_info* output_var,
   const std::string& kernel_name, const std::deque<int>& curr_unroll_factors);

  c_var_info* printMalloc(std::string,data_types,const domain_node*);
  c_var_info* init_value_var
  (std::string& curr_value,const data_types& curr_data_type){
    return new cuda_var_info(curr_value,NULL,curr_data_type);
  }

  /// \brief print_domain_loops_header Overloaded method to generate kernel
  /// launches that iterate over the domain
  /// \param curr_domain Domain to iterate over
  /// \param input_exprs The vector exprs that are input
  /// \param output_var Buffer which stores the output
  /// \param scale_domain the scaling factor used on the LHS
  /// \param curr_unroll_factors Unroll factors used in kernel
  /// \return number of inner loops that are tiled, for CUDA it is 0 always
  int print_domain_loops_header
  (const domain_node* curr_domain,std::deque<c_symbol_info*>& input_exprs,
   const c_var_info* output_var,const domainfn_node* scale_domain,
   const std::deque<int>& curr_unroll_factors);

  /// Function that decides if the current dimension is expanded or not
  bool isExpandedDimension
  (const offset_hull& offsetInfo, int dim, int loop_dim);

  /// \brief print_unrolled_code_body_helper Helper Function to print body of
  /// the untiled code using unrolling
  /// \param loop_dim Dimension of the iterator unrolled
  /// \param num_loop_dims Total loop dimensionality
  /// \param unroll_factors Current list of unroll factors
  /// \param curr_fn the function expression being handled, is a stencil
  /// function application
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol the buffer to which the result is to be written
  /// \param loop_domain the domain for the current patch
  /// \param is_bdy Boolean to specify if this is a boundary patch
  void print_unrolled_code_body_helper
  (int loop_dim, int num_loop_dims, std::deque<int>& unroll_factors,
   const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   c_symbol_info* output_symbol, bool is_bdy);

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
  void print_unrolled_code_body
  (int loop_dim, int num_loop_dims, std::deque<int>& unroll_factors,
   const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   c_symbol_info* output_symbol, bool is_bdy);

  /// \brief print_unrolled_code_helper Helper Function to print untiled code
  /// using unrolling for GPUs
  /// \param curr_dim Dimension of the unrolled code being generated
  /// \param loop_dim Max DImensionality of the loop to unroll
  /// \param curr_fn the function expression being handled, is a stencil
  /// function application
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param output_symbol the buffer to which the result is to be written
  /// \param loop_domain the domain for the current patch
  /// \param is_bdy Boolean to specify if this is a boundary patch
  /// \param curr_unroll_factors Current list of unroll factors
  void print_unrolled_code_helper
  (int curr_dim, int loop_dim, const fnid_expr_node* curr_fn,
   std::deque<c_symbol_info*>& input_exprs, c_symbol_info* output_symbol,
   domain_node* loop_domain, bool is_bdy, std::deque<int>& curr_unroll_factors);

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
  void print_unrolled_code
  (int curr_dim, int loop_dim, const fnid_expr_node* curr_fn,
   std::deque<c_symbol_info*>& input_exprs, c_symbol_info* output_symbol,
   domain_node* loop_domain, bool is_bdy, std::deque<int>& curr_unroll_factors);


  /// \brief print_domain_loops_footer Method to print footer of loops generated
  /// by print_domain_loops_header
  /// \param loop_domain The loop domain
  /// \param curr_unroll_factors Unroll factors used in kernel
  void print_domain_loops_footer
  (const domain_node* loop_domain, const std::deque<int>& curr_unroll_factors);

  /// \brief print_stencil_op_helper Helper function to print stencil operations
  /// \param curr_symbol The symbol
  /// \param scale_fn Scaling function to be used
  /// \param base_data_type The underlying data type of the expression
  /// (different from data type if cotains field access
  /// \param field_num the filed number accessed
  /// \param handle_bdys
  /// \return string with the stencil access
  std::string print_stencil_op_helper
  (const c_symbol_info* curr_symbol, const domainfn_node* scale_fn,
   const data_types& base_data_type, int field_num, bool handle_bdys);

  void print_array_access
  (const c_var_info*,const std::deque<std::string>&, std::stringstream&);
  void print_texture_point
  (const c_var_info*,const domainfn_node*,std::stringstream&,
   bool is_texture_normalized = false);
  void print_texture_point
  (const c_var_info*,const domainfn_node*,const domain_node*,std::stringstream&,
   bool is_texture_normalized = false);

  /// \brief computeOffsets Computes the offsets the loop bounds that is to be
  /// used
  /// \param fn the current function application expression being evaluated
  /// \param offsets the offset hull that represents the aggregate of all the
  /// arguments
  /// \param output_scale_domain Scaling function used on the LHS (if any)
  /// \param considerBdyInfo boolean to handle bdys here itself (or else handled
  /// later)
  void computeOffsets
  (const fnid_expr_node* fn, std::deque<offset_hull>& offsets,
   const domainfn_node* output_scale_domain, bool considerBdyInfo = false);

  /// \brief init_arg_info Method to initialize an argument to the function,
  /// this method initialize texture units
  /// \param curr_argument Current argument being evaluated
  /// \param arg_bdy_condn Boundary condition used with the argument
  /// \param curr_param The parameter bound to the argument
  /// \param input_expr_symbols List of symbols that are to be mapped to
  /// parameters of the called function
  /// \param arg_domains Vector of domains of the arguments used later in
  /// print_fn_body
  void init_arg_info
  (const vector_expr_node* curr_argument, bdy_info* arg_bdy_condn,
   const vector_defn_node* curr_param, c_symbol_table& fn_bindings,
   std::deque<c_symbol_info*>& input_expr_symbols,
   std::deque<const domain_node*>& arg_domains) ;

  /// \brief deleteArgSymbols Function to delete the symbols for arguments
  /// generated by init_fn_args
  /// \param arg_symbols
  void deleteArgSymbols(std::deque<c_symbol_info*>& arg_symbols);

  // /// \brief print_fnid_expr Method to generate code for fn application exprs
  // /// \param curr_fn current expressions
  // /// \param fn_bindings Current symbol table
  // void print_fnid_expr(const fnid_expr_node*,c_symbol_table&);

  void init_parameters();

  /// \brief print_vectorfn_body Method to generate the body of a vector
  /// function
  /// \param curr_fn the stencil function application expression
  /// \param input_exprs List of symbols that corr. to arguments of stencil fn
  /// \param arg_domains the size of the input arguments
  /// \param curr_output_symbol the symbol corresponding to the return
  /// expression of the function
  void print_vectorfn_body
  (const fnid_expr_node* curr_fn, std::deque<c_symbol_info*>& input_exprs,
   std::deque<const domain_node*>& arg_domains,
   c_symbol_info* curr_output_symbol);

  void print_program_body(const program_node*,c_symbol_table&,std::string&);

  void print_string(const program_node*,const program_options&,std::string&);
  void print_header_info(FILE*);

  bool enable_texture;
  stringBuffer file_scope;
  std::deque<texture_reference_info*> texture_references;
  void prepare_texture_reference
  (c_symbol_info*, const bdy_info*);

  /// paramters representing the blocksizes for each dimension
  std::map<int,parameter_defn*> block_sizes;

  /// Temporary storage location for iterators that correspond to launch
  /// dimensions
  std::deque<c_iterator*> launch_iters;
};


#endif
