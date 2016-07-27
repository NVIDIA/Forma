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
#ifndef __VECTOR_STMT_HPP__
#define __VECTOR_STMT_HPP__

#include "AST/vector_expr.hpp"

/**Struct to represent a for-loo iterator
 */
struct for_iterator {
  
  std::string name;

  ///The dimension of the iterator
  domain_node* iter_domain;

  ///Direction 
  bool is_positive;

  for_iterator(char* id, domain_node* new_domain, bool direction = true){
    name.assign(id);
    iter_domain = new_domain;
    is_positive = direction;
  }
  
  ~for_iterator(){
    delete iter_domain;
    name.clear();
  }

};


struct for_stmt_seq{
  std::deque<stmt_node*> stmt_list;
};



/** Class to represent a statement
    stmt ::= ID '=' vectorexpr ';'
*/
class stmt_node: public vector_expr_node{

private:

  ///Extrapolation function if specified
  domainfn_node* scale_domain;

  ///The symbol used to represent the right-hand side, already added
  ///to symbol table by the parser
  std::string lhs;

  ///The right-hand side vector expr
  vector_expr_node* rhs;

  ///Iterator used in the definition
  for_iterator* access_iterator;

  ///Offset to iterator
  int offset;
  
public:

  ///Dummy Constructor
  stmt_node() {
    expr_type = VEC_STMT;
    scale_domain = NULL;
    rhs = NULL;
    access_iterator = NULL;
  } 
  
  ///Constructor with rhs value written to contiguous domain starting at [0,0]
  stmt_node(const char* id_name, vector_expr_node* new_rhs, int ao = DEFAULT_RANGE , for_iterator* ai = NULL){
    lhs.assign(id_name);
    rhs = new_rhs;
    ndims = new_rhs->get_dim();
    scale_domain = NULL;
    access_iterator = ai;
    offset = ao;
    elem_type.assign(new_rhs->get_data_type());
    base_elem_type.assign(elem_type);
    rhs->add_usage(this);
    expr_type = VEC_STMT;
  }

  ///Constructor with rhs value written to a contiguous sub-domain
  stmt_node(const char* id_name, vector_expr_node* new_rhs, domain_node* sub_d, int ao = DEFAULT_RANGE , for_iterator* ai = NULL){
    if( !sub_d->check_if_offset() ){
      fprintf(stderr,"[ME] : ERROR! ");
      sub_d->print_node(stderr);
      fprintf(stderr," Specified domain is not in offset format, currently the ub for all dimensions on RHS must be elft free\n");
      exit(1);
    }
    ///The domain at the output must be at least as much as the domain at the input
    if( new_rhs->get_dim() > sub_d->get_dim() ){
      fprintf(stderr,"[ME] : ERROR! : %s",id_name);
      sub_d->print_node(stderr); 
      fprintf(stderr," = ");
      new_rhs->print_node(stderr);
      fprintf(stderr," : Dimension of the output domain smaller than the domain of the rhs");
      exit(1);
    }
    lhs.assign(id_name);
    rhs = new_rhs;
    ndims = sub_d->get_dim();
    scale_domain = NULL;
    sub_domain = sub_d;
    access_iterator = ai;
    offset = ao;
    elem_type.assign(rhs->get_data_type());
    base_elem_type.assign(elem_type);
    rhs->add_usage(this);
    expr_type = VEC_STMT;
  }

  ///Constructor with rhs value is extrapolated to the lhs
  stmt_node(const char* id_name, vector_expr_node* new_rhs, domainfn_node* sub_d, int ao = DEFAULT_RANGE , for_iterator* ai = NULL){
    ///The domain of the scaling function must be same size as the rhs
    if( new_rhs->get_dim() != sub_d->get_dim() ){
      fprintf(stderr,"[ME] : ERROR! : %s",id_name);
      sub_d->print_node(stderr); 
      fprintf(stderr," = ");
      new_rhs->print_node(stderr);
      fprintf(stderr," : Dimension of the scaling function not same as the domain of the rhs\n");
      exit(1);
    }
    lhs.assign(id_name);
    rhs = new_rhs;
    ndims = sub_d->get_dim();
    scale_domain = sub_d;
    sub_domain = NULL;
    access_iterator = ai;
    offset = ao;
    elem_type.assign(rhs->get_data_type());
    base_elem_type.assign(elem_type);
    rhs->add_usage(this);
    expr_type = VEC_STMT;
  }

  const domain_node*  compute_domain()
  {
    ///Initialize the domain of the output with the domain of the input
    expr_domain->init_domain(rhs->compute_domain());
    ///If a sub-domain is used on the input side, intersect the sub-domain
    if( rhs->get_sub_domain() )
      expr_domain->compute_intersection(rhs->get_sub_domain());
    ///Realign the input domain
    expr_domain->realign_domain();

    ///If a scaling is used on a left hand-side scale it up
    if( scale_domain )
      expr_domain->compute_scale_up(scale_domain);
    expr_domain->realign_domain();

    ///In case a scaling is not used and an offset is used instead, compute the new-domain, do not realign
    if( sub_domain )
      expr_domain->add_offset(sub_domain);

    return expr_domain;
  }

  //Method to change the name used - Use with caution
  inline void set_name(const char* new_name){
    lhs.clear();
    lhs.assign(new_name);
  }
  
  ///Method to get the lhs_name
  inline const char* get_name() const {
    return lhs.c_str();
  }
  inline const std::string get_name_string() const {
    return lhs;
  }

  ///Method to return the rhs expr
  const vector_expr_node* get_rhs() const {
    return rhs;
  }

  const for_iterator* get_access_iterator() const{
    return access_iterator;
  }

  int get_offset() const {
    return offset;
  }

  ///Destructor
  virtual ~stmt_node(){
    for( std::deque<vector_expr_node*>::iterator it = uses.begin() ; it != uses.end() ; it++ ){
      assert((*it)->get_type() == VEC_ID );
      static_cast<vec_id_expr_node*>(*it)->remove_defn(this);
    }
    lhs.clear();
    if( rhs )
      delete rhs;
    if( scale_domain ){
      delete scale_domain;
    }
  }

  ///Virtual function to get stmt sequence associated with loops
  virtual const for_stmt_seq* get_body() const {
    assert(0);
    return NULL;
  }

  ///Return the scale_domain
  const domainfn_node* get_scale_domain() const {
    return scale_domain;
  }

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();

  // int compute_simpleC_size();
  // void print_simpleC() const;

  template <typename State> friend class ASTVisitor;
};


/** CLass to support for stmt production
 */
class for_stmt_node : public stmt_node{

private:
  for_iterator* iter_info;
  
  for_stmt_seq* body;

public:

  for_stmt_node(for_iterator* in, for_stmt_seq* sn ) :
    iter_info(in),
    body(sn)
  {
    ndims= in->iter_domain->get_dim();
    expr_type = VEC_FORSTMT;
  }

  void print_node(FILE* outfile) const {
    fprintf(outfile,"%s\n",iter_info->name.c_str());
    for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end()  ; it++ )
      (*it)->print_node(stdout);
  }

  inline const for_iterator* get_iterator() const {
    return iter_info;
  }

  inline const for_stmt_seq* get_body() const {
    return body;
  }

  int compute_pretty_print_size() ;
  void pretty_print() const;
  
  ~for_stmt_node(){
    delete iter_info;
    for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ ){
      delete (*it);
    }
    delete body;
  }
  
  template <typename State> friend class ASTVisitor;
};


/** Call to support do stmt productions
 */
class do_stmt_node : public stmt_node{

private:
  
  for_iterator* iter_info;

  for_stmt_seq* body;
  
  vector_expr_node* escape_condn;
  
public:

  do_stmt_node(for_iterator* in, for_stmt_seq* sn, vector_expr_node* terminate) :
    iter_info(in),
    body(sn),
    escape_condn(terminate) 
  { 
    ndims = in->iter_domain->get_dim();
    expr_type = VEC_DOSTMT;
  }
    
  void print_node(FILE* outfile) const {
    fprintf(outfile,"%s\n",iter_info->name.c_str());
    for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end()  ; it++ )
      (*it)->print_node(stdout);
  }
  
  int compute_pretty_print_size() ;
  void pretty_print() const;

  inline const for_iterator* get_iterator() const {
    return iter_info;
  }

  inline const for_stmt_seq* get_body() const {
    return body;
  }

  ~do_stmt_node() {
    delete iter_info;
    for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ ){
      delete (*it);
    }
    delete body;
    delete escape_condn;
  }    
  template <typename State> friend class ASTVisitor;
  
};


/** Class to support variables defined through for statements
 */
class multi_stmt_node : public vector_expr_node{
  
private:

  ///Name of the variable defined
  std::string name;

  ///List of definitions
  std::deque<std::pair<domain_node*,stmt_node*> > def_exprs;

public:

  multi_stmt_node(domain_node* curr_domain, stmt_node* def_stmt){
    if( curr_domain->get_dim() != 1 ){
      fprintf(stderr,"Currently only for loop of one-dimension supported\n");
      exit(1);
    }
    name.assign(def_stmt->get_name());
    ndims = def_stmt->get_dim();
    elem_type.assign(def_stmt->get_data_type());      
    base_elem_type.assign(elem_type);
    def_exprs.push_back(std::pair<domain_node*,stmt_node*>(curr_domain,def_stmt));
    expr_type = VEC_MULTISTMT;
  }

  void add_def_domain(domain_node* curr_domain, stmt_node* def_stmt){
    if( curr_domain->get_dim() != 1 ){
      fprintf(stderr,"[ME] : ERROR ! Currently nested for-loops unsupported\n");
      exit(1);
    }
    range_coeffs curr_range = curr_domain->range_list.front();
    assert(curr_range.lb->type == P_INT && curr_range.ub->type == P_INT );
    int curr_range_lb = static_cast<int_expr*>(curr_range.lb)->value;
    int curr_range_ub = static_cast<int_expr*>(curr_range.ub)->value;
    for( std::deque<std::pair<domain_node*,stmt_node*> >::iterator it = def_exprs.begin() ; it != def_exprs.end() ; it++ ){
      domain_node* check_domain = it->first;
      range_coeffs check_range = check_domain->range_list.front();
      assert(check_range.lb->type == P_INT && check_range.ub->type == P_INT );
      int check_range_lb = static_cast<int_expr*>(check_range.lb)->value;
      int check_range_ub = static_cast<int_expr*>(check_range.ub)->value;
      // if( ISIN(curr_range.lb,check_range.lb,check_range.ub) || ISIN(curr_range.ub,check_range.lb,check_range.ub) ){
      if( !(curr_range_lb > check_range_ub || curr_range_ub < check_range_lb) ){
	fprintf(stderr,"[ME] : ERROR ! Redefinition of symbol %s \n",name.c_str());
	exit(1);
      }
    }
    if( def_stmt->get_dim() != ndims ){
      fprintf(stderr,"[ME] : ERROR ! Elements of  Qualified variable (%s) dont have the same dimensionality\n",name.c_str());
      exit(1);
    }
    if( !data_types::is_same_type(elem_type,def_stmt->get_data_type()) ){
      fprintf(stderr,"[ME] : ERROR ! Elements of  Qualified variable (%s) dont have the same data types\n",name.c_str());
      exit(1);
    }
    def_exprs.push_back(std::pair<domain_node*,stmt_node*>(curr_domain,def_stmt));
  }

  void remove_def_domain(const stmt_node* curr_stmt){
    for( std::deque<std::pair<domain_node*,stmt_node*> >::iterator it = def_exprs.begin() ; it != def_exprs.end() ; it++ ){
      if( curr_stmt == it->second ){
	delete it->first;
	def_exprs.erase(it);
	return;
      }
    }
    fprintf(stderr,"[MW] : Warning : Removing unknown definition\n");
  }

  
  inline stmt_node* check_if_defined(int point) const{
    for( std::deque<std::pair<domain_node*,stmt_node*> >::const_iterator it = def_exprs.begin() ; it != def_exprs.end() ; it++ ){
      domain_node* check_domain = it->first;
      range_coeffs check_range = check_domain->range_list.front();
      assert((check_range.lb->type == P_INT && check_range.ub->type == P_INT ) && ("[ME]: No support for parametric loop bounds of for-loops"));
      int_expr* lb = static_cast<int_expr*>(check_range.lb);
      int_expr* ub = static_cast<int_expr*>(check_range.ub);
      if( ISIN(point,lb->value,ub->value) )
	return it->second;
    }
    return NULL;
  }

  
  inline bool check_if_defined(int lb, int ub, std::set<vector_expr_node*>& defns)const{
    for( std::deque<std::pair<domain_node*,stmt_node*> >::const_iterator it = def_exprs.begin() ; it != def_exprs.end() ; it++ ){
      domain_node* check_domain = it->first;
      range_coeffs check_range = check_domain->range_list.front();
      assert((check_range.lb->type == P_INT && check_range.ub->type == P_INT ) && ("[ME]: No support for parametric loop bounds of for-loops"));
      int_expr* curr_lb = static_cast<int_expr*>(check_range.lb);
      int_expr* curr_ub = static_cast<int_expr*>(check_range.ub);
      if( ISIN(lb,curr_lb->value,curr_ub->value) ){
	defns.insert(it->second);
	lb = curr_ub->value+1;
      }
      if( ISIN(ub,curr_lb->value,curr_ub->value) ){
	defns.insert(it->second);
	ub = curr_ub->value-1;
      }
    }
    return ( lb <= ub ? false : true );
  }
  

  ~multi_stmt_node(){
    for( std::deque<std::pair<domain_node*,stmt_node*> >::iterator it = def_exprs.begin() ; it != def_exprs.end() ; it++ )
      delete it->first;
    def_exprs.clear();
  }

  inline const char* get_name() const{
    return name.c_str();
  }

  void print_node(FILE*)const  { };
  int compute_pretty_print_size() { return 0; }
  void pretty_print() const { };
  const domain_node* compute_domain() { return NULL; };
};



#endif
