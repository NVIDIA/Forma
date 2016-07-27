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
#include "ASTVisitor/copy_visitor.hpp"

using namespace std;

domain_node* CopyVectorExpr::copy(const domain_node* orig_domain)
{
  domain_node* new_domain = new domain_node();
  for( deque<range_coeffs>::const_iterator it = orig_domain->range_list.begin() ; it != orig_domain->range_list.end() ; it++ ){
    new_domain->add_range(parametric_exp::copy(it->lb),parametric_exp::copy(it->ub));
  }
  return new_domain;
}

domainfn_node* CopyVectorExpr::copy(const domainfn_node* orig_domain)
{
  domainfn_node* new_domain = new domainfn_node();
  for( deque<scale_coeffs>::const_iterator it = orig_domain->scale_fn.begin() ; it != orig_domain->scale_fn.end() ; it++ ){
    new_domain->add_scale_coeffs(it->offset,it->scale);
  }
  return new_domain;
}

bdy_info* CopyVectorExpr::copy(const bdy_info* orig_bdy)
{
  return new bdy_info(orig_bdy->type,orig_bdy->value);
}


domain_desc_node* CopyVectorExpr::copy(const domain_desc_node*  orig_domain)
{
  if( dynamic_cast<const domain_node*>(orig_domain) ){
    return copy(static_cast<const domain_node*>(orig_domain));
  }
  else{
    return copy(static_cast<const domainfn_node*>(orig_domain));
  }
}

vector_expr_node* CopyVectorExpr::copy(const vector_expr_node* orig_expr)
{
  vector_expr_node* ret_expr;
  switch(orig_expr->get_type()){
  case VEC_COMPOSE:
    ret_expr =  copy(static_cast<const compose_expr_node*>(orig_expr));
    break;
  case VEC_ID:
    ret_expr =  copy(static_cast<const vec_id_expr_node*>(orig_expr));
    break;
  case VEC_MAKESTRUCT:
    ret_expr =  copy(static_cast<const make_struct_node*>(orig_expr));
    break;
  case VEC_FN:
    ret_expr =  copy(static_cast<const fnid_expr_node*>(orig_expr));
    break;
  case VEC_SCALE:
    ret_expr =  copy(static_cast<const vec_domainfn_node*>(orig_expr));
    break;
  case VEC_SCALAR:
    ret_expr =  copy(static_cast<const expr_node*>(orig_expr));
    break;
  default:
    assert(0);
    return NULL;
  }
  ret_expr->set_access_field(orig_expr->get_access_field());
  return ret_expr;
}


compose_expr_node* CopyVectorExpr::copy(const compose_expr_node* orig_expr)
{
  compose_expr_node* new_compose = new compose_expr_node();
  const deque<pair<domain_desc_node*,vector_expr_node*> > orig_list = orig_expr->get_expr_list();
  for( deque<pair<domain_desc_node*,vector_expr_node*> >::const_iterator it = orig_list.begin() ; it != orig_list.end() ; it++ ){
    domain_desc_node* new_domain = copy(it->first);
    vector_expr_node* new_expr = copy(it->second);
    if( dynamic_cast<domain_node*>(new_domain) ){
      new_compose->add_expr(static_cast<domain_node*>(new_domain),new_expr);
    }
    else{
      new_compose->add_expr(static_cast<domainfn_node*>(new_domain),new_expr);
    }
  }
  return new_compose;
}


vec_id_expr_node* CopyVectorExpr::copy(const vec_id_expr_node* orig_expr)
{
  vec_id_expr_node* new_expr = new vec_id_expr_node(orig_expr);
  return new_expr;
}

make_struct_node* CopyVectorExpr::copy(const make_struct_node* orig_expr)
{
  const deque<vector_expr_node*>& field_inputs = orig_expr->get_field_inputs();
  deque<vector_expr_node*>::const_iterator it = field_inputs.begin();
  make_struct_node* new_expr = new make_struct_node(copy(*it));
  it++;
  for(  ; it != field_inputs.end() ; it++) {
    new_expr->add_field_input(copy(*it));
  }
  new_expr->set_struct_definition(orig_expr->get_data_type());
  return new_expr;
}

fnid_expr_node* CopyVectorExpr::copy(const fnid_expr_node* orig_expr)
{
  fnid_expr_node* new_expr = new fnid_expr_node();
  const deque<arg_info>& orig_args = orig_expr->get_args();
  for( deque<arg_info>::const_iterator it = orig_args.begin() ; it != orig_args.end() ; it++ ){
    new_expr->add_arg(copy(it->arg_expr),copy(it->bdy_condn));
  }
  new_expr->find_definition(orig_expr->get_name());
  return new_expr;
}

vec_domainfn_node* CopyVectorExpr::copy(const vec_domainfn_node* orig_expr)
{
  return new vec_domainfn_node(copy(orig_expr->get_base_expr()),copy(orig_expr->get_scale_fn()));
}

expr_node* CopyVectorExpr::copy(const expr_node* orig_expr)
{
  assert((orig_expr->get_s_type() == S_VALUE) && "[ME] : Scalar expression in vector function which is not a value expr");
  switch( orig_expr->get_data_type().type ){
  case T_FLOAT:
    return new value_node<float>(static_cast<const value_node<float>*>(orig_expr)->get_value());
  case T_DOUBLE:
    return new value_node<double>(static_cast<const value_node<double>*>(orig_expr)->get_value());
  case T_INT:
    return new value_node<int>(static_cast<const value_node<int>*>(orig_expr)->get_value());
  default:
    assert(0);
    return NULL;
  }
}
