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
#include "ASTVisitor/convert_boundaries.hpp"

using namespace std;

vector_expr_node* ConvertBoundaries::Visitor::visit_fnid_expr(fnid_expr_node* curr_expr, void* state)
{
  stencilfn_defn_node* fn_defn = dynamic_cast<stencilfn_defn_node*>(curr_expr->fn_defn);
  if( fn_defn){
    printf("Stencil fn : %s\n",curr_expr->get_name());
    deque<arg_info>& curr_args = curr_expr->args;
    const deque<vector_defn_node*>& curr_params = fn_defn->get_args();
    deque<vector_defn_node*>::const_iterator jt = curr_params.begin();
    for( deque<arg_info>::iterator it = curr_args.begin(); it != curr_args.end() ; it++, jt++ ){
      ///Replace the bdy condin by a compose expression if the boundary is not none or constant(0)
      if( it->bdy_condn->type != B_NONE &&  it->bdy_condn->type != B_CONSTANT && it->bdy_condn->value != 0 ){
	const vector_defn_node* curr_defn = *jt;
	const vector_expr_node* curr_argument = it->arg_expr;

	const deque<offset_hull>& access_info = curr_defn->get_access_info();
	
	///Compute the domain of the interior of the new padded expression
	domain_node* interior_domain = new domain_node();
	interior_domain->init_domain(curr_argument->get_expr_domain());
	if( curr_argument->get_sub_domain() )
	  interior_domain->compute_intersection(curr_argument->get_sub_domain());
	
	bool create_expression = false;

	///Add the padding to the interior domain
	deque<range_coeffs>::iterator curr_dim = interior_domain->range_list.begin();
	for( deque<offset_hull>::const_iterator kt = access_info.begin() ; kt != access_info.end() ; kt++, curr_dim++ ){
	  ///Adjust the interior domain according to access extent
	  if( kt->max_negetive != 0){
	    create_expression = true;
	    curr_dim->lb = curr_dim->lb->add(-kt->max_negetive);
	    curr_dim->ub = curr_dim->ub->add(-kt->max_negetive);
	  }
	  if( kt->max_positive != 0 ){
	    create_expression = true;
	  }
	}

	if( create_expression ){
	  ///Now create the actual expression
	  compose_expr_node* new_expr = new compose_expr_node();
	  
	  int curr_dimension = 0;
	  for( deque<offset_hull>::const_iterator kt = access_info.begin() ; kt != access_info.end() ; kt++, curr_dim++ ){
	    ///Creating the domain for initializing the part accessed by negetive offsets
	    if( kt->max_negetive != 0 ){
	      domain_node* neg_offset_domain = new domain_node();
	      neg_offset_domain->init_domain(interior_domain);
	      neg_offset_domain->range_list[curr_dimension].lb = neg_offset_domain->range_list[curr_dimension].lb->add(-kt->max_negetive);
	      vector_expr_node* init_expr = create_init_expression(*it);
	      if( init_expr->get_type() == VEC_SCALAR ){
		delete  neg_offset_domain->range_list[curr_dimension].ub;
		neg_offset_domain->range_list[curr_dimension].ub = parametric_exp::copy(interior_domain->range_list[curr_dimension].lb);
		neg_offset_domain->range_list[curr_dimension].ub = neg_offset_domain->range_list[curr_dimension].ub->subtract(1);
	      }
	      else{
		delete neg_offset_domain->range_list[curr_dimension].ub;
		neg_offset_domain->range_list[curr_dimension].ub = new int_expr(DEFAULT_RANGE);
	      }
	      new_expr->add_expr(neg_offset_domain,init_expr);
	    }

	    ///Do the same for the positive offsets
	    if( kt->max_positive != 0 ){
	      domain_node* positive_offset_domain = new  domain_node();
	      positive_offset_domain->init_domain(interior_domain);
	      delete positive_offset_domain->range_list[curr_dimension].lb;
	      positive_offset_domain->range_list[curr_dimension].lb = parametric_exp::copy(interior_domain->range_list[curr_dimension].ub);
	      positive_offset_domain->range_list[curr_dimension].lb = positive_offset_domain->range_list[curr_dimension].lb->add(1);
	      
	      vector_expr_node* init_expr = create_init_expression(*it);
	      
	      if( init_expr->get_type() == VEC_SCALAR ){
		positive_offset_domain->range_list[curr_dimension].ub = positive_offset_domain->range_list[curr_dimension].ub->add(kt->max_positive );
	      }
	      else{
		delete positive_offset_domain->range_list[curr_dimension].ub;
		positive_offset_domain->range_list[curr_dimension].ub = new int_expr(DEFAULT_RANGE);
	      }
	      new_expr->add_expr(positive_offset_domain,init_expr);
	    }
	  }
	  new_expr->add_expr(interior_domain,it->arg_expr);
	  it->bdy_condn->type = B_NONE;
	  it->arg_expr = new_expr;
	}
	else{
	  delete interior_domain;
	}
      }      
    }
  }
  return NULL;
}


vector_expr_node* ConvertBoundaries::Visitor::create_init_expression(const arg_info& curr_argument)
{
  data_types elem_type = curr_argument.arg_expr->get_data_type();
  switch(curr_argument.bdy_condn->type){
  case B_CONSTANT:
    switch( elem_type.type ){
    case T_DOUBLE:
      return new value_node<double>(curr_argument.bdy_condn->value);
    case T_INT:
      return new value_node<int>(curr_argument.bdy_condn->value);
    case T_INT8:
      return new value_node<unsigned char>(curr_argument.bdy_condn->value);
    case T_INT16:
      return new value_node<short>(curr_argument.bdy_condn->value);
    case T_FLOAT:
      return new value_node<float>(curr_argument.bdy_condn->value);
    default:
      assert( 0 && "Unknown initialization for Struct");
    }
  default:
    assert(0);
    return NULL;
  }
}


