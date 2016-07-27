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
#include "AST/parser.hpp"

using namespace std;

string get_op_string(operator_type op){
  stringstream op_string;
  switch(op){
  case O_PLUS:
    op_string << "+" ;
    break;
  case O_MULT:
    op_string << "*" ;
    break;
  case O_MINUS:
    op_string << "-" ;
    break;
  case O_DIV:
    op_string << "/" ;
    break;
  case O_EXP:
    op_string << "^" ;
    break;
  case O_LT:
    op_string << "<" ;
    break;
  case O_GT:
    op_string << ">" ;
    break;
  case O_LE:
    op_string << "<=" ;
    break;
  case O_GE:
    op_string << ">=" ;
    break;
  case O_EQ:
    op_string << "==" ;
    break;
  case O_NE:
    op_string << "!=" ;
    break;
  default:
    ;
  }
  return op_string.str();
}


int get_op_string_size(operator_type op){
  return get_op_string(op).size();
}

bool is_relation_op(operator_type op){
  bool ret_val;
  switch(op){
  case O_LT:
  case O_GT:
  case O_LE:
  case O_GE:
  case O_EQ:
  case O_NE:
    ret_val = true;
    break;
  default:
    ret_val = false;
  }
  return ret_val;
}


fn_defn_node::fn_defn_node(local_symbols* curr_symbols)
{
  fn_symbols = curr_symbols;
  for( deque<pair<string,vector_expr_node*> >::iterator it = fn_symbols->symbols.begin() ; it != fn_symbols->symbols.end() ; it++ ){
    vector_defn_node* curr_arg = dynamic_cast<vector_defn_node*>(it->second);
    assert(curr_arg);
    fn_args.push_back(curr_arg);
  }
  expr_domain = new domain_node();
}

stencilfn_defn_node::stencilfn_defn_node(local_symbols* curr_local_symbols, local_scalar_symbols* curr_local_scalars):
  fn_defn_node(curr_local_symbols),
  local_scalars(curr_local_scalars)
{
  ///The following is a hack for now. It checks that all arguments
  ///passed to the stencil function definitions are the same dimensions or are scalar (0-dimension)
  int base_dim = 0;
  int curr_dim;
  for( deque<vector_defn_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    if( (curr_dim  = (*it)->get_dim()) != 0 ){
      if( !base_dim )
	base_dim = curr_dim;
      else if( base_dim != curr_dim ){
	fprintf(stderr,"[ME] : ERROR : In definition of stencil fn %s, vector arguments have different dimensions, currently not supported",fn_name.c_str());
	exit(1);
      }
    }
  }
  return_dim = base_dim;
  return_expr = NULL;
}


const domain_node* stencilfn_defn_node::compute_domain(const deque<const domain_node*>& arg_domains )
{
  assert(arg_domains.size() == fn_args.size());
  deque<vector_defn_node*>::iterator jt = fn_args.begin() ;
  domain_node* temp_domain = new domain_node();
  bool first = true;
  for( deque<const domain_node*>::const_iterator it = arg_domains.begin() ; it != arg_domains.end() ; it++ , jt++ ){
    ///Check if the passed argument is a scalar
    if( (*jt)->get_dim() != 0 && !(*jt)->get_direct_access() ){
      
      assert((*jt)->get_dim() == (*it)->get_dim() && "Mismatch in input arg domain dimension and specified arg dimension" );
      
      ///Initialize the domain of the fn_param with the domain of the passed argument. It is not realigned.
      (*jt)->init_domain(*it);
      
      ///Compute the domain of the function call as an intersection of the domain of the arguments
      temp_domain->init_domain(*it);
      const deque<offset_hull>& access_info = (*jt)->get_access_info();
      deque<range_coeffs>::iterator lt = temp_domain->range_list.begin() ; 
      for( deque<offset_hull>::const_iterator kt = access_info.begin() ; kt != access_info.end() ; kt++, lt++ ){
	if( kt->scale != 1 ){
	  lt->lb = lt->lb->divide(kt->scale);
	  lt->ub = lt->ub->add(1)->divide(kt->scale)->subtract(1);
	}
      }
      
      if( first ){
	expr_domain->init_domain(temp_domain);
      }
      else{
	expr_domain->compute_intersection(temp_domain);
      }
    }
  }
  delete temp_domain;

  ///Realign the domain of the call expression
  expr_domain->realign_domain();

  return expr_domain;
}


int vectorfn_defn_node::get_return_dim() const
{
  return return_expr->get_dim();
}  

data_types vectorfn_defn_node::get_data_type() const
{
  if( return_expr == NULL )
    assert((0) && ("Accessing data_type of function with undefined return expr"));
  return return_expr->get_data_type();

}

const domain_node* vectorfn_defn_node::compute_domain()
{
  ///This function is called only on the outermost function
  ///Compute the domain of all the statements
  for( deque<stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    (*it)->compute_domain();
  
  ///Compute the domain of the return expression
  expr_domain->init_domain(return_expr->compute_domain());
  ///if the return expression has a sub-domain, compute the intersection
  if( return_expr->get_sub_domain() )
    expr_domain->compute_intersection(return_expr->get_sub_domain());
  ///Realign the domain of the return expression
  expr_domain->realign_domain();

  return expr_domain;
}


const domain_node* vectorfn_defn_node::compute_domain(const deque<const domain_node*>& arg_domains )
{
  assert(arg_domains.size() == fn_args.size());
  deque<vector_defn_node*>::iterator jt = fn_args.begin();
  for( deque<const domain_node*>::const_iterator it = arg_domains.begin() ; it != arg_domains.end() ; it++ , jt++ ){
    ///Initialize the domain of the fn_param with the domain of the passed argument. It is not realigned.
    (*jt)->expr_domain->init_domain(*it);
  }

  ///Compute the domains of all the statements in the function
  for( deque<stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    (*it)->compute_domain();

  ///Compute the domain of the return expression
  expr_domain->init_domain(return_expr->compute_domain());
  if( return_expr->get_sub_domain() )
    expr_domain->compute_intersection(return_expr->get_sub_domain());

  ///Realign the domain of the final return 
  expr_domain->realign_domain();

  return expr_domain;
}


vectorfn_defn_node::~vectorfn_defn_node()
{
  if( fn_symbols )
    delete fn_symbols;
  for( deque<stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    delete (*it);
  }
  fn_body.clear();
  if( return_expr ){
    delete return_expr;
  }
  for( deque<vector_defn_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    delete (*it);
  }
  fn_args.clear();
  fn_name.clear();
}

data_types stencilfn_defn_node::get_data_type() const{
  return return_expr->get_data_type();
}


stencilfn_defn_node::~stencilfn_defn_node(){
  if( fn_symbols )
    delete fn_symbols;
  for( deque<pt_stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    delete (*it);
  fn_body.clear();
  if( return_expr ){
    //printf("Deleting : %p\n",return_expr);
    delete return_expr;
  }
  for( deque<vector_defn_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    delete (*it);
  }
  fn_args.clear();
  fn_name.clear();
  delete local_scalars;
}
