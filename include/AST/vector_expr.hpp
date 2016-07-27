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
#ifndef __VECTOR_EXPR_HPP__
#define __VECTOR_EXPR_HPP__

#include "AST/domain.hpp"
#include "AST/fn_defn.hpp"
#include <set>

#define ISIN(a,b,c) ( a >= b  && a <= c ? true : false )

struct for_iterator;

enum vector_expr_type{
  VEC_ERR,
  VEC_DEFN,
  VEC_ID,
  VEC_SCALE,
  VEC_FN,
  VEC_COMPOSE,
  VEC_MAKESTRUCT,
  VEC_STMT,
  VEC_DOSTMT,
  VEC_FORSTMT,
  VEC_MULTISTMT,
  VEC_SCALAR,
};

/**Class to represent vector expressions
   vectorexpr ::= ID | ID '(' vectorexprlist ')' | vectorexpr domain | '(' compose_expr ')'   
*/
class vector_expr_node : public ast_node{ 

protected:

  ///The subdomain of the base vector expression to operate on
  domain_node* sub_domain;
  
  ///The domain of the current expression
  domain_node* expr_domain;

  ///Dimensionality of the vector
  int ndims;
 
  ///Data type of the vector elements
  data_types elem_type;
  data_types base_elem_type;

  ///Field of the struct accessed, used only for T_STRUCT == elem_type.type
  int field_num;

  ///Type of the vector expression
  vector_expr_type expr_type;

  ///Uses of the statement
  std::deque<vector_expr_node*> uses;
   
public:
  
  ///Constructor
  vector_expr_node() { 
    expr_domain = new domain_node();
    sub_domain = NULL;
    field_num = -1;
    ndims = 0;
  }

  ///Helper function to set the operating domain, used by the parser
  inline void set_domain(domain_node* sd) { 
    if( sd->get_dim() != ndims ){
      fprintf(stderr,"[ME] : ERROR! : ");
      print_node(stderr); sd->print_node(stderr);
      fprintf(stderr," Sub-domain not same dimensionality as the expression\n");
      exit(1);
    }
    sub_domain = sd; 
  }

  ///Return the sub-domain of the expression
  virtual const domain_node* get_sub_domain() const {
    return sub_domain;
  }

  ///When the field of a struct is accessed
  void add_access_field(const char*);
  void set_access_field(int a) { field_num = a; }
  std::string get_field_name() const;

  ///Method to return the field_num of the vector_expr
  inline int get_access_field() const {
    return field_num;
  }

  ///Return the expr-domain of the expression
  virtual const domain_node* get_expr_domain() const{
    return expr_domain;
  }

  ///overloaded function for pretty printing
  virtual void print_node(FILE*) const =0;

  ///Function to get the dimensionality of the expr
  inline int get_dim() const { return ndims; }

  ///Method to get the data type of the expression
  inline const data_types& get_data_type() const {
    return elem_type;
  }

  ///Function to compute the domain of the expr
  virtual const domain_node* compute_domain()=0;

  ///Destructor
  virtual ~vector_expr_node(){
    delete expr_domain;
    if( sub_domain ){
      delete sub_domain;
    }
    //uses.clear();
  }

  ///Method to return the type of the vector expr
  inline vector_expr_type get_type() const{
    return expr_type;
  }
  
  ///Method to get the base expr of the vector exp
  inline const data_types& get_base_type() const{
    return base_elem_type;
  }

  ///Add/Remove a usage of the defined variable
  inline void add_usage(vector_expr_node* new_usage){
    uses.push_back(new_usage);
  }
  inline void remove_usage(const vector_expr_node* prev_usage){
    for( std::deque<vector_expr_node*>::iterator it = uses.begin() ; it != uses.end() ; it++ ){
      if( prev_usage == (*it) ){
  	uses.erase(it);
  	break;
      }
    }
  }
  const std::deque<vector_expr_node*>& get_usage() const{
    return uses;
  }
  
  template <typename State> friend class ASTVisitor;
};



/** Class that represent the compose expressions
    compose ::= '(' partcompose ')'
    partcompose ::= domain '=' vectorexpr ';' | domainfn '=' vectorexpr ';'
*/
class compose_expr_node : public vector_expr_node{
  
protected:

  ///vector expressions that map to contiguous domains
  std::deque<std::pair<domain_desc_node*,vector_expr_node*> > output_domains;

  inline void check_dims(const domain_desc_node* new_domain, const vector_expr_node* new_expr){
    if( ndims ){
      if( new_domain->get_dim() != ndims || !data_types::is_same_type(elem_type,new_expr->get_data_type()) ){
	fprintf(stderr,"[ME] : ERROR! : ");
	new_domain->print_node(stderr);
	fprintf(stderr," = ");
	new_expr->print_node(stderr);
	if( new_domain->get_dim() != ndims )
	  fprintf(stderr," : Dimension mismatch with previous parts of compose expr\n");
	else
	  fprintf(stderr," : Data type mismatch with previous parts of compose expr\n");
	exit(1);
      }
    }
    else{
      ndims = new_domain->get_dim();
      elem_type.assign(new_expr->get_data_type());
      base_elem_type.assign(elem_type);
    }
  }

public:

  ///Constructor
  compose_expr_node() { 
    expr_type = VEC_COMPOSE;
  }

  ///Functions to add a domain descriptor at the output for a component
  ///of the compose expression, used by the parser
  inline void add_expr(domain_node* new_domain, vector_expr_node* new_expr) {
    if( new_expr->get_type() == VEC_SCALAR ){
      if( !new_domain->check_defined() ){
	fprintf(stderr," [ME] : ERROR! : Improperly defined domains, when RHS is a scalar, the domain must be fully specified\n");
	exit(1);
      }
      ndims = new_domain->get_dim();
      elem_type.assign(new_expr->get_data_type());
      base_elem_type.assign(elem_type);
    }
    else{
      if( new_domain->get_dim() < new_expr->get_dim() ){
	fprintf(stderr,"[ME] : ERROR! : ");
	new_domain->print_node(stderr);
	fprintf(stderr," = ");
	new_expr->print_node(stderr);
	fprintf(stderr," : Dimension of the output domain %d is less than the rhs domain %d in compose expr\n",new_domain->get_dim(), new_expr->get_dim());
	exit(1);
      }
      if( !new_domain->check_if_offset() ){
	fprintf(stderr,"[ME] : ERROR! : ");
	new_domain->print_node(stderr);
	fprintf(stderr," = ");
	new_expr->print_node(stderr);
	fprintf(stderr," : The output domain not specified as an offset, the upper-bound must be left free\n");
	exit(1);
      }
      check_dims(new_domain,new_expr);
    }
    new_expr->add_usage(this);
    output_domains.push_back(std::pair<domain_desc_node*,vector_expr_node*>(new_domain,new_expr));
  }
  
  inline void add_expr(domainfn_node* new_domain, vector_expr_node* new_expr){
    if( new_domain->get_dim() != new_expr->get_dim() ){
      fprintf(stderr,"[ME] : ERROR! : ");
      new_domain->print_node(stderr);
      fprintf(stderr," = ");
      new_expr->print_node(stderr);
      fprintf(stderr," : Dimension of the scale fn is not same as the rhs domain in compose expr\n");
      exit(1);
    }
    check_dims(new_domain,new_expr);
    new_expr->add_usage(this);
    output_domains.push_back(std::pair<domain_desc_node*,vector_expr_node*>(new_domain,new_expr));
  }

  ///Method to get the parts of the compose expr
  inline const std::deque<std::pair<domain_desc_node*,vector_expr_node*> >& get_expr_list() const{
    return output_domains;
  }

  const domain_node* compute_domain();

  ///Destructor
  ~compose_expr_node(){
    std::deque<std::pair<domain_desc_node*,vector_expr_node*> >::iterator it;
    for( it = output_domains.begin() ; it != output_domains.end() ; it++ ){
      delete it->first;
      delete it->second;
    }
    output_domains.clear();
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();

  template <typename State> friend class ASTVisitor;
};


/** Class that represent ID production of vector expressions
 */
class vec_id_expr_node : public vector_expr_node{
  
private:
  ///Name of the vector - must have a previous definition
  std::string name;

  ///Definition of the vector
  std::set<vector_expr_node*> defn ;

  ///Iterator used in the definition - used only for qualified variables
  const for_iterator* access_iterator;

  ///Offset to iterator - used only for qualified variables
  int offset;

public:

  ///Constructor
  vec_id_expr_node(const char*,local_symbols*);

  ///Constructor for ID< > 
  vec_id_expr_node(const char*,local_symbols*,int,for_iterator*);

  ///Constructor used for copying
  vec_id_expr_node(const vec_id_expr_node*);

  ///Returns the name of the vector referenced
  inline const char* get_name() const{
    return name.c_str();
  }
  inline const std::string get_name_string() const{
    return name;
  }

  ///Return the access_iterator
  inline const for_iterator* get_access_iterator() const{
    return access_iterator;
  }

  ///Return the offset to iterator
  int get_offset() const {
    return offset;
  }

  ///Return the definition for this id_expr
  inline const std::set<vector_expr_node*>& get_defn() const {
    return defn;
  }

  const domain_node* compute_domain(){
    ///Assuming that the definition refered to here has its domain pre-computed
    assert(defn.size() == 1 );
    expr_domain->init_domain((*defn.begin())->get_expr_domain());
    // if( sub_domain )
    //   expr_domain->compute_intersection(sub_domain);
    return expr_domain;
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();

  ///Remove a definition from the list
  void remove_defn(vector_expr_node* remove_defn){
    defn.erase(remove_defn);
    // size_t n_erased = defn.erase(remove_defn);
    // assert((n_erased == 1) && ("[ME] : Error! Trying to remove non-existent definition\n"));
  }

  // int compute_simpleC_size();
  // void print_simpleC() const;
  
  ///Destructor
  ~vec_id_expr_node();

  template <typename State> friend class ASTVisitor;
};



/** Node used to create structures
 */
class make_struct_node : public vector_expr_node
{
private:

  std::deque<vector_expr_node*> field_inputs;

public:
  
  make_struct_node(vector_expr_node* first_expr) {
    field_inputs.push_back(first_expr);
    ndims = first_expr->get_dim();
    expr_type = VEC_MAKESTRUCT;
    first_expr->add_usage(this);
  }

  inline const std::deque<vector_expr_node*>& get_field_inputs() const{
    return field_inputs;
  }

  inline void add_field_input(vector_expr_node* field_expr){
    if( field_expr->get_dim() != ndims ){
      fprintf(stderr,"[ME] : ERROR! Mismatch in dimensionality while making structure\n");
      exit(1);
    }
    field_expr->add_usage(this);
    field_inputs.push_back(field_expr);
  }

  inline void find_struct_definition(char* struct_name){
    defined_data_type* curr_type = defined_types->find_symbol(struct_name);
    if( curr_type == 0 ){
      fprintf(stderr,"[ME] : ERROR! Unkown structure %s\n",struct_name);
      exit(1);
    }
    if( field_inputs.size() > curr_type->fields.size() ){
      fprintf(stderr,"[ME] : ERROR! Specified fields is more than the number of fields of struct %s\n",struct_name);
      exit(1);
    }
    // std::deque<vector_expr_node*>::iterator it = field_inputs.begin();
    // std::deque<defined_fields>::iterator jt = curr_type->fields.begin();
    // for( ; jt != curr_type->fields.end() ; jt++,it++ ){
    //   if( jt->field_type != (*it)->get_data_type().type ){
    // 	fprintf(stderr,"[ME] : ERROR! Mismatch of data type while making struct %s\n",struct_name);
    // 	exit(1);
    //   }
    // }
    elem_type.assign(T_STRUCT);
    elem_type.struct_info = curr_type;
    base_elem_type.assign(elem_type);
  }

  inline void set_struct_definition(const data_types& curr_type){
    elem_type.assign(T_STRUCT);
    elem_type.struct_info = curr_type.struct_info;
    base_elem_type.assign(elem_type);
  }

  const domain_node* compute_domain() {
    domain_node* temp_domain = new domain_node();
    for( std::deque<vector_expr_node*>::iterator it = field_inputs.begin() ; it != field_inputs.end() ; it++ ){
      ///Initialize the temp domain with the domain of the expression used
      temp_domain->init_domain((*it)->compute_domain());
      ///Get the sub-domain if necessary
      if( (*it)->get_sub_domain())
	temp_domain->compute_intersection((*it)->get_sub_domain());
      
      ///Update the domain of the output expressions as an intersection of the current domain and the temp domain
      if( it == field_inputs.begin() )
	expr_domain->init_domain(temp_domain);
      else
	expr_domain->compute_intersection(temp_domain);
    }
    ///Realign the final domain
    expr_domain->realign_domain();
    delete temp_domain;
    return expr_domain;
  }

  void print_node(FILE*) const { }
  
  void pretty_print() const;
  
  int compute_pretty_print_size();

  ~make_struct_node() {
    for( std::deque<vector_expr_node*>::iterator it = field_inputs.begin() ; it != field_inputs.end() ; it++ ){
      delete *it;
    }
    field_inputs.clear();
  }

  template <typename State> friend class ASTVisitor;
};



/** Bdytypes supported for fn args
 */
enum bdy_type{
  B_NONE,
  B_CONSTANT,
  B_CLAMPED,
  B_EXTEND,
  B_WRAP,
  B_MIRROR,
};

struct bdy_info {
  bdy_type type;
  int value;
  bdy_info(bdy_type curr_type = B_NONE, int val = 0 ) {
    type = curr_type;
    value = val;
  }
  void print_node(FILE*) const;
  int compute_size() const ;
  inline void pretty_print() const;
};

/** Struct to hold the argument information passed to a function
 */
struct arg_info{

  vector_expr_node* arg_expr;
  
  bdy_info* bdy_condn;

  arg_info(vector_expr_node* curr_arg, bdy_info* curr_bdy){
    arg_expr = curr_arg;
    bdy_condn = curr_bdy;
  }
};


class expr_node;

/** Class that represents the function application production of vector expressions
 */
class fnid_expr_node : public vector_expr_node{
  
private:

  ///Name
  std::string id_name;
  
  ///The definition of the function being applied
  fn_defn_node* fn_defn;

  ///Arguments to the function
  std::deque<arg_info> args;

public:

  ///Constructor
  fnid_expr_node()  { 
    expr_type = VEC_FN;
  }

  ///Find the function definition, helper function used by the parser
  void find_definition(const char*);

  ///Return the function definition
  inline fn_defn_node* get_defn() const {
    return fn_defn;
  }

  ///Get the arguments to the function
  const std::deque<arg_info>& get_args() const {
    return args;
  }

  ///Add to list of arguments of the function, helper function used by the parser
  inline void add_arg(vector_expr_node* new_arg, bdy_info* curr_bdy){
    new_arg->add_usage(this);
    args.push_back(arg_info(new_arg,curr_bdy));
  }

  ///Add a scalar argument
  void add_arg(expr_node*);

  const char* get_name() const {
    return id_name.c_str();
  }

  
  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();


  const domain_node* compute_domain();

  ///Destructor
  ~fnid_expr_node(){
    for( std::deque<arg_info>::iterator it = args.begin() ; it != args.end() ; it++ ){
      delete it->arg_expr;
      delete it->bdy_condn;
    }	     
    args.clear();
    id_name.clear();
  }

  template <typename State> friend class ASTVisitor;
  friend class ConvertBoundaries;
};


/** Class to represent domainfn applied to a vectorexpr
 */
class vec_domainfn_node : public vector_expr_node{
private:
  
  ///Base vector_expr
  vector_expr_node* base_vec_expr;

  ///The domain function appliued to the vector expression
  domainfn_node* scale_fn;

public:

  ///Consructor
  vec_domainfn_node(vector_expr_node* oe, domainfn_node* sf){
    if( sf->get_dim() != oe->get_dim() ){
      fprintf(stderr,"[ME] : ERROR! : ");
      oe->print_node(stderr); sf->print_node(stderr);
      fprintf(stderr," ScaleFn not same dimensionality as the expression\n");
      exit(1);
    }
    base_vec_expr = oe;
    scale_fn = sf;
    for( std::deque<scale_coeffs>::const_iterator it = scale_fn->scale_fn.begin() ; it != scale_fn->scale_fn.end() ; it++ ){
      if( it->scale == 1 && it->offset != 0 ){
	fprintf(stderr,"[MW] : Warning : Using 1 as the scaling factor in vector expression results in the domain of the output being reduced by the size of the offset %d. Use a stencil fn to get around this\n",it->offset);
      }
    }
    ndims = oe->get_dim();
    elem_type.assign(base_vec_expr->get_data_type());
    base_elem_type.assign(elem_type);
    oe->add_usage(this);
    expr_type = VEC_SCALE;
  }

  ///overloaded pretty print function
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();

  ///Get the base -expr
  const domainfn_node* get_scale_fn() const {
    return scale_fn;
  }

  inline const vector_expr_node* get_base_expr() const { 
    return base_vec_expr;
  }

  const domain_node* compute_domain(){
    ///Initialize using the domain of the base expression
    expr_domain->init_domain(base_vec_expr->compute_domain());

    ///If a sub-domain is ued, compute intersection with the sub-domain
    if( base_vec_expr->get_sub_domain() )
      expr_domain->compute_intersection(base_vec_expr->get_sub_domain());
    ///Realign the domain 
    expr_domain->realign_domain();

    ///Scale it up and realign again
    expr_domain->compute_scale_down(scale_fn);
    expr_domain->realign_domain();

    return expr_domain;
  }

  ///Destructor
  ~vec_domainfn_node(){
    delete base_vec_expr;
    delete scale_fn;
  }

  template <typename State> friend class ASTVisitor;

};




#endif
