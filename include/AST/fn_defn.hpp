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
#ifndef __FN_DEFN_HPP__
#define __FN_DEFN_HPP__

#include "AST/symbol_table.hpp"
#include <cassert>

class vector_defn_node;

/** Class to represent all function definitions, stencil, vector and
    point
*/
class fn_defn_node : public ast_node{

protected:
  ///Name of the function
  std::string fn_name;

  ///Symbol table for the current function
  local_symbols* fn_symbols;

  ///Function arguments
  std::deque<vector_defn_node*> fn_args;

  ///The domain of the output expression
  domain_node* expr_domain;

  int pretty_print_size;

public:

  ///Constructor
  fn_defn_node(local_symbols* curr_symbols);

  ///Virtual function to print node info
  virtual void print_node(FILE*) const;

  ///Virtual destructor
  virtual ~fn_defn_node() {
    delete expr_domain;
  };

  ///Helper function to set the name of the function, used by the parser
  inline void set_name(const char* name){
    fn_name.assign(name);
  }

  ///Method to get the name of the function
  inline const char* get_name() const{
    return fn_name.c_str();
  }

  inline const local_symbols* get_fn_symbols() const{
    return fn_symbols;
  }

  ///Method to get the fn args
  const std::deque<vector_defn_node*>& get_args() const{
    return fn_args;
  }

  ///Method to return the dimensionality of the return expression
  virtual int get_return_dim() const=0;    

  ///Method to return the data_type of the return expr
  virtual data_types get_data_type() const =0 ;

  inline std::deque<vector_defn_node*>::const_iterator args_begin(){
    return fn_args.begin();
  }

  inline std::deque<vector_defn_node*>::const_iterator args_end(){
    return fn_args.begin();
  }  

  ///Method to compute the domain of the return expression
  virtual const domain_node* compute_domain(const std::deque<const domain_node*>&)=0;
  const domain_node* get_expr_domain() const{
    return expr_domain;
  }

  void pretty_print() const=0;
  int compute_pretty_print_size()=0;
  
  template<typename State> friend class ASTVisitor;

};

class stmt_node;
class vector_expr_node;

/** Class that represents vector function definitions
    vectorfndef ::= 'vector' ID '(' args+ ')' '{' stmts 'return' vectorexpr ';' '}'
    
    Symbol table contains vector parameters / assigned vector temps
*/
class vectorfn_defn_node : public fn_defn_node{
private:
  
  ///The function body 
  std::deque<stmt_node*> fn_body;

  ///The return value of the function
  vector_expr_node* return_expr;

  ///The symbol table for parameters
  // param_table* fn_parameters;

public:

  ///Constructor
  vectorfn_defn_node(local_symbols* curr_local_symbols):
    fn_defn_node(curr_local_symbols)
  {
    return_expr = NULL;
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Helper function to add a statement to the function body, used by the parser
  inline void add_stmt(stmt_node* new_stmt){
    fn_body.push_back(new_stmt);
  }

  ///Helper function to set the return expr of the function, used by
  ///the parser, Return expression is not set only at the program level
  inline void add_ret_expr(vector_expr_node* new_expr){
    return_expr = new_expr;
  }

  ///Method to return the data_type of the return expr
  data_types get_data_type() const ;

  ///The return dimension is same as the dimensionality of the return expression
  int get_return_dim() const;

  ///Method to compute the domain of the return expression
  const domain_node* compute_domain(const std::deque<const domain_node*>&);
  const domain_node* compute_domain();

  ///Method to thet the function body
  const std::deque<stmt_node*>& get_body() const{
    return fn_body;
  }

  ///Method to get the return expr
  const vector_expr_node* get_return_expr() const {
    return return_expr;
  }

  ///Method to get the function parameters
  // const param_table* get_parameters() const{
  //   return fn_parameters;
  // }

  ///Destructor
  ~vectorfn_defn_node();

  ///Hack function to print the defs and statements at program level
  void print_statements(FILE*) const;
  void print_symbols(FILE*) const;

  // void pretty_print_statements() const;
  // void pretty_print_symbols() const;
  void pretty_print_program() const;
  int compute_pretty_print_program();

  void pretty_print() const;
  int compute_pretty_print_size();

  // void print_simpleC_program() const;
  // int compute_simpleC_size_program();

  template<typename State> friend class ASTVisitor;

};


class pt_stmt_node;
class expr_node;

/** Class that represents stencil function definition
    stencilfndef ::= 'stencil' ID '{' ptstmts  'return' expr ';' '}'

    Symbol table contains only function arguments, everything else is not 'parsed'
*/
class stencilfn_defn_node : public fn_defn_node{
  
private:
  
  ///The function body
  std::deque<pt_stmt_node*> fn_body;

  ///The return value of the function
  expr_node* return_expr;

  ///The dimensionality of the return vector
  int return_dim;

  ///Local scalar variables. For now these are not strict SSA
  local_scalar_symbols* local_scalars;

public:

  ///Constructor
  stencilfn_defn_node(local_symbols*,local_scalar_symbols*);

  ///Helper function to add a statement to the function body, used by the parser
  inline void add_stmt(pt_stmt_node* new_stmt){
    fn_body.push_back(new_stmt);
  }

  ///Helper function to set the return expr of the function, used by
  ///the parser. Return expression must be set
  inline void add_ret_expr(expr_node* new_expr){
    return_expr = new_expr;
  }

  ///Method to return the return_expr
  inline const expr_node* get_return_expr() const {
    return return_expr;
  }

  ///Method to get the scalar symbols
  inline const local_scalar_symbols* get_local_scalars() const {
    return local_scalars;
  }

  ///Method to get the ttype of the return expression , for now returns double only
  data_types get_data_type() const ;

  ///Retruns the dimensionality of the return expression
  inline int get_return_dim() const {
    return return_dim;
  }  

  const std::deque<pt_stmt_node*>& get_body() const {
    return fn_body;
  }

  ///Function to return the domain of the output expressions
  const domain_node* compute_domain(const std::deque<const domain_node*>&);

  ///Destructor
  ~stencilfn_defn_node();

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  int compute_pretty_print_size() ;
  void pretty_print() const ;

  template<typename State> friend class ASTVisitor;
};

/** Class that holds all the function definitions
 */
class fn_defs_table: public symbol_table<fn_defn_node*> {

private:
  
  int pretty_print_size;

public:

  ///Method to add a function definition
  void add_fn_def(fn_defn_node* fn_defn){
    const char* fn_name = fn_defn->get_name();
    int err = add_symbol(fn_name,fn_defn);
    if( err ){
      fprintf(stderr,"Duplicate Function Definition of fn : %s\n",fn_name);
      exit(1);
    }
  }

  ///Method to retrieve a function definition
  fn_defn_node* find_fn_defn(const char* fn_name) const{
    fn_defn_node* fn_defn = find_symbol(fn_name);
    if( !fn_defn ){
      fprintf(stderr,"Unknown Function name : %s\n",fn_name);
      exit(1);
    }
    return fn_defn;
  }
  
  //Function for pretty printing
  void print_node(FILE*) const;

  ///Destructor
  ~fn_defs_table(){
    for( std::deque<std::pair<std::string,fn_defn_node*> >::iterator it = symbols.begin() ; it != symbols.end() ; it++ )
      delete it->second;
  }

  int compute_pretty_print_size();
  void pretty_print() const ;

  template <typename State> friend class ASTVisitor;
};


///Global variable that tracks all function definitions
extern fn_defs_table* fn_defs;


#endif
