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
#ifndef __STENCIL_EXPR_HPP__
#define __STENCIL_EXPR_HPP__

#include "AST/vector_defn.hpp"
#include "AST/pretty_print.hpp"
#include <sstream>

class StencilExprVisitor;

/** Operators supported in stencil definitions
 */
enum operator_type{
  O_ERR, ///< error type
  O_PLUS, ///< '+'
  O_MULT, ///< '*'
  O_MINUS, ///< '-'
  O_DIV, ///< '/'
  O_EXP, ///< '^'
  O_LT, ///< '<'
  O_GT, ///< '>'
  O_LE, ///< '<='
  O_GE, ///< '<='
  O_EQ, ///< '=='
  O_NE, ///< '!='
};


/** Expr types
 */
enum expr_types{
  S_ERR,
  S_VALUE,
  S_UNARYNEG,
  S_ID,
  S_MATHFN,
  S_TERNARY,
  S_BINARYOP,
  S_STENCILOP,
  S_STRUCT,
  S_ARRAYACCESS
};

std::string get_op_string(operator_type);
int get_op_string_size(operator_type);
bool is_relation_op(operator_type);

/**Class to represent expression allowed within the stencil function definition

   expr ::= ID | FLOAT | DOUBLE | INT | expr OP expr | '(' expr ')' |
expr domainfn | ID '(' exprlistx') | '(' expr '?' expr ':' expr ')' 
*/
class expr_node : public vector_expr_node{

protected:

  // ///Data type of the expr_node
  // data_types elem_type;

  ///Expression type
  expr_types s_expr_type;

public:

  ///Constructor
  expr_node() {
    expr_type = VEC_SCALAR;
  }

  ///Destructor
  virtual ~expr_node() {}

  ///Overloaded function for pretty printing
  virtual void print_node(FILE*) const=0;

  // ///Get the tyep of the expressions
  // virtual data_types get_data_type() const {
  //   return elem_type;
  // }

  ///Method to get the expression type
  inline expr_types get_s_type() const {
    return s_expr_type;
  }

  ///Method to compute domain
  inline const domain_node* compute_domain() {
    return expr_domain;
  }

  ///Method to change the data type of expression, only valid with basic data types
  inline void cast_to_type(basic_data_types value){
    assert((elem_type.type != T_STRUCT) && ("[ME]: Error ! Invalid cast of struct operator\n"));
    elem_type.assign(value);
    // base_elem_type.assign(elem_type);
  }

  friend class StencilExprVisitor;
};

/**Class to represent float,doubles or integers used in the stencil definition
 */
template<typename F>
class value_node : public expr_node{
private:
  ///The value stored
  const F val;
  
  ///Get the type of the value;
  void set_type();

public:
  ///Constructor
  value_node(F inp_val) :
    val(inp_val) {
    set_type();
    s_expr_type = S_VALUE;
  }

  ///Method to print the value
  const F get_value() const {
    return val;
  }

  value_node* copy() const {
    return new value_node<F>(val);
  }
  
  ///Overloaded function to print the value
  void print_node(FILE*) const;

  int compute_pretty_print_size();
  void pretty_print() const;

  friend class StencilExprVisitor;
};


template<typename F>
void value_node<F>::print_node(FILE* outfile) const{
  fprintf(outfile,"%d",val);
}

template<> void value_node<double>::print_node(FILE*) const;
template<> void value_node<float>::print_node(FILE*) const;

template<typename F>
int value_node<F>::compute_pretty_print_size()
{
  std::stringstream curr_num;
  curr_num << val ;
  pretty_print_size = curr_num.str().size();
  return pretty_print_size;
}

template<typename F>
void value_node<F>::pretty_print() const
{
  //PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  std::stringstream curr_num;
  curr_num << val;
  PrettyPrinter::instance()->print(PPTokenString(curr_num.str()),curr_num.str().size());
  //PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


/** class to represent unary negation
 */

class unary_neg_expr_node : public expr_node {
private:
  expr_node* base_expr;

public:
  unary_neg_expr_node(expr_node* curr_exp) : base_expr(curr_exp) { 
    elem_type.assign(base_expr->get_data_type());
    s_expr_type = S_UNARYNEG;
  }

  ~unary_neg_expr_node() { delete base_expr; }

  void print_node(FILE*) const { };

  ///Method to get the base expr
  inline const expr_node* get_base_expr() const {
    return base_expr;
  }

  inline int compute_pretty_print_size() { 
    pretty_print_size = base_expr->compute_pretty_print_size();
    return pretty_print_size;
  }

  void pretty_print() const {
    PrettyPrinter::instance()->print(PPTokenString("-"),1);
    base_expr->pretty_print();
  }

  friend class StencilExprVisitor;
};


/** Class to represent the ID production of expr
 */
class id_expr_node : public expr_node{
private:
  ///The varibale name refered to - note that this is not holding a
  ///pointer to a symbol table entry. Scalars in stencil fns can be
  ///redefined, using the final compiler to handle errors.
  std::string id_name;

  ///Pointer to symbol table entry refered to here
  const scalar_sym_info* id_symbol;

  ///If not a local scalar, then it refers to a fn argument which is a scalar
  const vector_defn_node* scalar_defn;

public:

  ///Constructor
  id_expr_node(const char* name,const local_scalar_symbols* curr_local_scalars) :
    id_symbol(curr_local_scalars->find_symbol(name)) ,
    scalar_defn(NULL)  
  {
    //id_symbol = curr_local_scalars->find_symbol(name);
    if (id_symbol == 0 ){
      fprintf(stderr,"[ME]: Error! Unknown reference %s\n",name);
      exit(1);
    }
    id_name.assign(name);
    elem_type.assign(id_symbol->data_type);
    //scalar_defn = NULL;
    s_expr_type = S_ID;
  }

  ///Copy constructor
  id_expr_node(const id_expr_node* orig_expr) :
    id_symbol(orig_expr->id_symbol),
    scalar_defn(orig_expr->scalar_defn)
  {
    id_name.assign(orig_expr->get_name());
    elem_type.assign(orig_expr->get_data_type());
    s_expr_type = S_ID;
  }

  ///Constructor
  id_expr_node(const char* name, vector_expr_node* curr_defn):
    id_symbol(NULL),
    scalar_defn(dynamic_cast<vector_defn_node*>(curr_defn))
  {
    assert(curr_defn->get_dim() == 0 );
    if( scalar_defn == NULL ){
      fprintf(stderr,"[ME] : Error! Unkown symbol %s in stencil function\n",name);
      exit(1);
    }
    elem_type.assign(curr_defn->get_data_type());
    id_name.assign(name);
    s_expr_type = S_ID;
  }


  ///Destructor
  ~id_expr_node(){
    id_name.clear();
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Get the name of the function
  const std::string get_name() const{
    return id_name;
  }
  
  inline int compute_pretty_print_size() {
    pretty_print_size = id_name.size();
    return pretty_print_size;
  }
  void pretty_print() const {
    //PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
    PrettyPrinter::instance()->print(PPTokenString(id_name),pretty_print_size);
    //PrettyPrinter::instance()->print(PPTokenEnd(),0);
  }

  friend class StencilExprVisitor;
};


/**Class to represent math function expression in function statements

 */
class math_fn_expr_node : public expr_node{
private:
  ///Arguments to the expression
  std::deque<expr_node*> fn_args;
  
  ///Name of function being applied
  std::string fn_name;
  //TODO : Make this search a list of functions for validity
  
public:
  
  ///Method to add function name, used by the parser
  inline void set_name(const char* fname){
    fn_name.assign(fname);
    // elem_type.assign(T_DOUBLE);  ///All math functions are assumed to return
    // 			   ///doubles for now
    assert(fn_args.size() != 0);
    for( std::deque<expr_node*>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
      assert((*it)->get_data_type().type != T_STRUCT);
      if( it == fn_args.begin() )
	elem_type.assign((*it)->get_data_type());
      else
	elem_type.assign(MIN((*it)->get_data_type().type,elem_type.type));
    }
    s_expr_type = S_MATHFN;
  }

  ///Method to add expressions that are arguments to the function
  inline void add_arg(expr_node* new_arg){
    fn_args.push_back(new_arg);
  }

  ///Method to get the name of the function
  inline const std::string& get_name() const  {
    return fn_name;
  }

  ///Method to get the fn_args
  const std::deque<expr_node*>& get_args() const {
    return fn_args;
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Destructor
  ~math_fn_expr_node(){
    for( std::deque<expr_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
      delete (*it);
    }
  }

  int compute_pretty_print_size() ;
  void pretty_print() const ;

  friend class StencilExprVisitor;
};


/** Class to represent ternary expressions
 */
class ternary_expr_node : public expr_node{
private:
  
  ///Condition expression
  expr_node* bool_expr;
  
  ///True expression
  expr_node* true_expr;

  ///False expression
  expr_node* false_expr;

public:
  
  ///Constructor
  ternary_expr_node(expr_node* be, expr_node* te, expr_node* fe){
    bool_expr = be;
    // if( !data_types::is_same_type(te->get_data_type(),fe->get_data_type()) ){
    //   fprintf(stderr,"[ME]:ERROR! ");
    //   print_node(stderr);
    //   fprintf(stderr," : the true and false expressions have different data types, not supported\n");
    //   exit(1);
    // }
    elem_type.assign(MIN(te->get_data_type().type,fe->get_data_type().type));
    true_expr = te;
    false_expr = fe;
    s_expr_type = S_TERNARY;
  }
  
  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Methods to get the conditional and the true and false expr
  const expr_node* get_bool_expr() const { return bool_expr; }
  const expr_node* get_true_expr() const { return true_expr; }
  const expr_node* get_false_expr() const { return false_expr; }

  ///Destructor
  ~ternary_expr_node(){
    delete bool_expr;
    delete true_expr;
    delete false_expr;
  }

  int compute_pretty_print_size();
  void pretty_print() const;

  friend class StencilExprVisitor;

};


/** Class to represent the binary operation/function application productions of expr
 */
class expr_op_node : public expr_node{

private:
  ///Operator used
  operator_type op;

  ///Right hand side expression
  expr_node* rhs_expr;

  ///Left hand side expression
  expr_node* lhs_expr;

public:

  ///Constructor
  expr_op_node(expr_node* lhs, expr_node* rhs, operator_type curr_op){
    op = curr_op;
    lhs_expr = lhs;
    rhs_expr = rhs;
    if( lhs_expr->get_data_type().type == T_STRUCT || rhs_expr->get_data_type().type == T_STRUCT ){
      fprintf(stderr,"[ME] : Error! In Stencil function, binary operation on struct is undefined\n");
      exit(1);
    }
    if( is_relation_op(curr_op) ){
      elem_type.assign(T_INT);
    }
    else{
      // bool rhs_condn = rhs_expr->get_s_type() == S_VALUE && rhs_expr->get_data_type().type == T_INT ;
      // bool lhs_condn =  lhs_expr->get_s_type() == S_VALUE && lhs_expr->get_data_type().type == T_INT ;
      // if( rhs_condn && !lhs_condn ){
      // 	elem_type.assign(lhs_expr->get_data_type().type);
      // }
      // else if( lhs_condn && !rhs_condn ) {
      // 	elem_type.assign(rhs_expr->get_data_type().type);
      // }
      // else{
      // 	elem_type.assign( MIN(lhs_expr->get_data_type().type,rhs_expr->get_data_type().type) );
      // }
      basic_data_types rhs_type = rhs_expr->get_data_type().type;
      basic_data_types lhs_type = lhs_expr->get_data_type().type;
      if( rhs_type == T_DOUBLE || lhs_type == T_DOUBLE )
	elem_type.assign(T_DOUBLE);
      else if( rhs_type == T_FLOAT || lhs_type == T_FLOAT )
	elem_type.assign(T_FLOAT);
      else 
	elem_type.assign(T_INT);
    }
    s_expr_type = S_BINARYOP;
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Methods to retrieve the parts of the expression
  operator_type get_op() const { return op; };
  const expr_node* get_rhs_expr() const { return rhs_expr; };
  const expr_node* get_lhs_expr() const { return lhs_expr; };

  ///Destructor
  ~expr_op_node(){
    //printf("Deleting : %p\n",lhs_expr);
    delete lhs_expr;
    //printf("Deleting : %p\n",rhs_expr);
    delete rhs_expr;
  }

  int compute_pretty_print_size();
  void pretty_print() const;

  friend class StencilExprVisitor;

};


/** Class that represents direct array accesses within stencil functions
    example : X[a]
*/
class array_access_node : public expr_node{
private: 
  ///array indices
  std::deque<expr_node*> index_exprs;

  ///Name of the parameter used
  std::string param_name;
  
  ///Defn of the current node
  vector_defn_node* defn;

public:

  array_access_node(expr_node* first_index){
    index_exprs.push_back(first_index);
  }

  inline void add_index(expr_node* curr_expr){
    index_exprs.push_back(curr_expr);
  }
  
  inline void set_name(const char* varname, const local_symbols* fn_args){
    defn = dynamic_cast<vector_defn_node*>(fn_args->find_local_symbol(varname));
    assert(defn);
    if( (unsigned)defn->get_dim() != index_exprs.size() ){
      fprintf(stderr,"ERROR: Dimension mismatch in array index expr : %s\n",varname);
      exit(1);
    }
    defn->set_direct_access();
    param_name.assign(varname);
    elem_type.assign(defn->get_data_type());
    base_elem_type.assign(elem_type);
    s_expr_type = S_ARRAYACCESS;
  }

  const std::deque<expr_node*>& get_index_exprs() const {
    return index_exprs;
  }

  const vector_defn_node* get_var() const {
    return defn;
  }
  
  const std::string& get_name() const {
    return param_name;
  }

  void print_node(FILE*) const;

  int compute_pretty_print_size();
  void pretty_print() const;

  friend class StencilExprVisitor;

};



/**Class that represents stencil ops  in stencil definitions
   example : X@[-1,0]
*/
class stencil_op_node : public expr_node{

private:
  ///domainfn to be used
  domainfn_node* scale_fn;
  
  ///Name of the parameter used as input to the domainfn
  std::string param_name;

  ///Vector definition of the current ID
  vector_defn_node* defn;

public:

  ///Constructor
  stencil_op_node(const char* name, domainfn_node* new_scale_fn, const local_symbols* fn_args){
    defn = dynamic_cast<vector_defn_node*>(fn_args->find_local_symbol(name));
    assert(defn);
    if( defn->get_dim() != new_scale_fn->get_dim() ){
      fprintf(stderr,"ERROR: Dimension mismatch in '@' operation : %s",name);
      new_scale_fn->print_node(stderr);
      fprintf(stderr,"\n");
      exit(1);
    }
    defn->update_stencil_access_info(new_scale_fn);
    defn->add_usage(this);
    param_name.assign(name);
    scale_fn = new_scale_fn;
    elem_type.assign(defn->get_data_type());
    base_elem_type.assign(elem_type);
    s_expr_type = S_STENCILOP;
  }

  ///Constructor
  stencil_op_node(const char* name, domainfn_node* new_scale_fn, vector_defn_node* curr_param){
    defn = curr_param;
    defn->update_stencil_access_info(new_scale_fn);
    param_name.assign(name);
    scale_fn = new_scale_fn;
    elem_type.assign(defn->get_data_type());
    base_elem_type.assign(defn->get_base_type());
    s_expr_type = S_STENCILOP;
  }


  ///Constructor for a point reference to vector
  stencil_op_node(vector_defn_node* curr_defn){
    assert((curr_defn) && ("Initializer in stencil_op_node called with null definition"));
    defn = curr_defn;
    domainfn_node* new_scale_fn = new domainfn_node(curr_defn->get_dim());
    defn->update_stencil_access_info(new_scale_fn);
    defn->add_usage(this);
    param_name.assign(curr_defn->get_name());
    scale_fn = new_scale_fn;
    elem_type.assign(defn->get_data_type());
    base_elem_type.assign(defn->get_base_type());
    s_expr_type = S_STENCILOP;
  }

  ///Destructor
  ~stencil_op_node(){
    param_name.clear();
    //printf("Deleting : %p\n",scale_fn);
    defn->remove_usage(this);
    delete scale_fn;
  }

  ///Function to get the definition and domainfn
  const vector_defn_node* get_var() const { return defn; }
  const domainfn_node* get_scale_fn() const { return scale_fn; }
  const std::string& get_name() const { return param_name; }

  
  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  int compute_pretty_print_size() ;
  void pretty_print() const ;

  friend class StencilExprVisitor;

};


/** Class to represent the struct expression on return
 */

class pt_struct_node : public expr_node{

private:
  
  std::deque<expr_node*> field_expr;

  defined_data_type* struct_info;

public:

  pt_struct_node(expr_node* new_expr){
    field_expr.push_back(new_expr);
    s_expr_type = S_STRUCT;
  }

  void add_field(expr_node* new_expr){
    field_expr.push_back(new_expr);
  }

  void find_struct_definition(const char* struct_name){
    struct_info = defined_types->find_symbol(struct_name);
    if( !struct_info ){
      fprintf(stderr,"[ME] : Unknown data type %s \n",struct_name);
      exit(1);
    }
    if( struct_info->fields.size() != field_expr.size() ){
      fprintf(stderr,"[ME] : Mismatch in number of fields specified, and defined in struct %s\n",struct_name);
      exit(1);
    }
    elem_type.type = T_STRUCT;
    elem_type.struct_info = struct_info;
    base_elem_type.assign(elem_type);
  }

  ~pt_struct_node() {
    for( std::deque<expr_node*>::iterator it = field_expr.begin() ; it != field_expr.end() ; it++ ){
      delete *it;
    }
  }

  const std::deque<expr_node*>& get_field_exprs() const{
    return field_expr;
  }

  inline const std::string& get_struct_name() const{
    return struct_info->name;;
  }

  void print_node(FILE*) const { }

  int compute_pretty_print_size();
  void pretty_print() const;

  friend class StencilExprVisitor;
};



/** Class to represent statements within the stencil function definition
    ptstmt ::= ID '=' expr ';'
*/
class pt_stmt_node : public ast_node{

private:
  ///The identifier used on the left-hand side
  std::string lhs_name;
  
  ///The righ-hand side expression
  expr_node* rhs_expr;

public:

  ///Constructor
  pt_stmt_node(const char* name, expr_node* rhs, local_symbols* fn_args){
    vector_expr_node* symbol_def = fn_args->find_symbol(name);
    if( symbol_def ){
      fprintf(stderr,"ERROR : redefinig parameters within stencil function not supported\n");
      exit(1);
    }
    lhs_name.assign(name);
    rhs_expr = rhs;
    //elem_type = rhs_expr->get_data_type();
  }

  pt_stmt_node(const char* name, expr_node* rhs){
    lhs_name.assign(name);
    rhs_expr = rhs;
  }

  ///Destructor
  ~pt_stmt_node(){
    //printf("Deleting : %p\n",rhs_expr);
    delete rhs_expr;
    lhs_name.clear();
  }

  ///Method to return the name of the lhs
  const std::string& get_lhs() const {
    return lhs_name;
  }

  ///Method to return the rhs
  const expr_node* get_rhs() const {
    return rhs_expr;
  }
  
  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  int compute_pretty_print_size() ;
  void pretty_print() const ;

  friend class StencilExprVisitor;
};


#endif
