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
#include "AST/vector_defn.hpp"
#include "AST/vector_stmt.hpp"
#include "AST/stencil_expr.hpp"

using namespace std;


void vector_expr_node::add_access_field(const char* field_name)
{
  if( elem_type.type != T_STRUCT ){
    fprintf(stderr,"[ME] : ERROR! Accessing struct for expression which is not a structure\n");
    exit(1);
  }
  assert(elem_type.struct_info);
  field_num = -1;
  for( deque<defined_fields>::iterator it = elem_type.struct_info->fields.begin() ; it != elem_type.struct_info->fields.end() ; it++ ){
    if( it->field_name.compare(field_name) == 0 )
      break;
    field_num++;
  }
  field_num++;
  if( field_num == (int)elem_type.struct_info->fields.size() ){
    fprintf(stderr,"[ME] : ERROR! Vector expression has no field %s\n",field_name);
    exit(1);
  }
  elem_type.assign(elem_type.struct_info->fields[field_num].field_type);
}

string vector_expr_node::get_field_name() const
{
  if( base_elem_type.type != T_STRUCT || base_elem_type.struct_info == NULL ){
    fprintf(stderr,"[ME] : ERROR! Accessing struct for expression which is not a structure\n");
    exit(1);
  }
  if( field_num == -1 ){
    return "";
  }
  else{
    assert(field_num < (int)base_elem_type.struct_info->fields.size());
    return base_elem_type.struct_info->fields[field_num].field_name;
  }
}


vec_id_expr_node::vec_id_expr_node(const  char* id_name , local_symbols* curr_vector_defs ) :
  access_iterator(NULL)
{
  vector_expr_node* curr_defn = curr_vector_defs->find_local_symbol(id_name);
  if( dynamic_cast<multi_stmt_node*>(curr_defn) ){
    fprintf(stderr,"[ME] : ERROR! Accessing previously qualified variable : %s,  without qualifying it\n",id_name);
    exit(1);
  }
  defn.insert(curr_defn);
  ndims = curr_defn->get_dim();
  elem_type.assign(curr_defn->get_data_type());
  base_elem_type.assign(elem_type);
  expr_type = VEC_ID;
  access_iterator = NULL;
  offset = -1;
  name.assign(id_name);
  curr_defn->add_usage(this);
}

vec_id_expr_node::vec_id_expr_node(const char* id_name, local_symbols* curr_vector_defs, int curr_offset, for_iterator* used_iterator) :
  access_iterator(used_iterator)
{
  multi_stmt_node* curr_symbol = dynamic_cast<multi_stmt_node*>(curr_vector_defs->find_local_symbol(id_name));
  if( curr_symbol == NULL ){
    fprintf(stderr,"[ME] : ERROR ! Undefined symbol : %s\n",id_name);
    exit(1);
  }
  if( used_iterator != NULL ){   ///Check if there is a surrounding iterator
    if( curr_offset != DEFAULT_RANGE ){     /// Check if there is an offset to the surrounding iterator val
      assert(used_iterator->iter_domain->range_list.front().lb->type == P_INT );
      if( used_iterator->is_positive ){ ///Check if the iterator is positive increment or negetive
	int lb = static_cast<int_expr*>(used_iterator->iter_domain->range_list.front().lb)->value; ///ASsumes that the surrounding iterator has integer bounds

	///The first intance of this variable is to be used, it is assumed that other instances are defined within the loop
	stmt_node* curr_defn = curr_symbol->check_if_defined( lb  - curr_offset ); 
	if( curr_defn == NULL ){
	  fprintf(stderr,"[ME] : Error ! %s<%d> is not defined in %s<%s-%d>\n",id_name,lb - curr_offset,id_name,used_iterator->name.c_str(),curr_offset);
	  exit(1);
	}
	defn.insert(curr_defn);
	curr_defn->add_usage(this);
      }
      else{
	int ub = static_cast<int_expr*>(used_iterator->iter_domain->range_list.front().ub)->value; ///ASsumes that the surrounding iterator has integer bounds

	///Check that the use for the first iteration of the loop is defined, it is assumed that the other instances will be defined. 
	stmt_node* curr_defn = curr_symbol->check_if_defined( ub + curr_offset );  
	if( curr_defn == NULL ){
	  fprintf(stderr,"[ME] : Error ! %s<%d> is not defined in %s<%s+%d>\n",id_name,ub + curr_offset,id_name,used_iterator->name.c_str(),curr_offset);
	  exit(1);
	}
	defn.insert(curr_defn);
	curr_defn->add_usage(this);
      }
    }
    else{
      ///If no offset is used, then all instances of the qualified variables have to be defined previously.
      assert(used_iterator->iter_domain->range_list.front().lb->type == P_INT && used_iterator->iter_domain->range_list.front().ub->type == P_INT );
      int lb = static_cast<int_expr*>(used_iterator->iter_domain->range_list.front().lb)->value;
      int ub = static_cast<int_expr*>(used_iterator->iter_domain->range_list.front().ub)->value;
      bool all_defined = curr_symbol->check_if_defined(lb, ub , defn );
      if( !all_defined ){
	fprintf(stderr,"[ME] : Error ! %s is not defined at all points in range %d..%d\n",id_name,lb,ub);
	exit(1);
      }
      for( set<vector_expr_node*>::iterator it = defn.begin() ; it != defn.end() ; it++ ){
      	(*it)->add_usage(this);
      }
    }
  }
  else{
    ///If it doesnt have an iterator, then we are defining a particular instance of the qualified variable
    vector_expr_node* curr_defn = curr_symbol->check_if_defined(curr_offset);
    if( curr_defn == NULL ){
      fprintf(stderr,"[ME] : Error ! %s<%d> is not defined\n",id_name,curr_offset);
      exit(1);
    }
    defn.insert(curr_defn);
    curr_defn->add_usage(this);
  }
  ndims = (*defn.begin())->get_dim();
  elem_type.assign((*defn.begin())->get_data_type());
  base_elem_type.assign(elem_type);
  expr_type = VEC_ID;
  name.assign(id_name);
  offset = curr_offset;
}

vec_id_expr_node::vec_id_expr_node(const vec_id_expr_node* orig_expr) :
  access_iterator(NULL)
{
  name.assign(orig_expr->name.c_str());
  ndims = orig_expr->get_dim();
  elem_type.assign(orig_expr->get_data_type());
  base_elem_type.assign(orig_expr->get_base_type());
  expr_type = VEC_ID;
  access_iterator = orig_expr->get_access_iterator();
  offset = orig_expr->get_offset();
  if( orig_expr->get_sub_domain() != NULL )
    sub_domain = new domain_node(orig_expr->get_sub_domain());
  for( set<vector_expr_node*>::const_iterator it = orig_expr->defn.begin() ; it != orig_expr->defn.end() ; it++ ){
    defn.insert(*it);
    (*it)->add_usage(this); ///If the expression is copied, there is now a new use of the definitions
  }
}


vec_id_expr_node::~vec_id_expr_node()
{
  name.clear();
  for( set<vector_expr_node*>::iterator it = defn.begin() ; it != defn.end() ; it++ ){
    //if( (*it)->get_type() == VEC_STMT ){
    (*it)->remove_usage(this);
      //}
  }
}

const domain_node* compose_expr_node::compute_domain()
{
  domain_node* temp_domain = new domain_node();
  for( deque<pair<domain_desc_node*,vector_expr_node*> >::iterator it = output_domains.begin(); it != output_domains.end() ; it++ ){
    
    /// if the rhs is  a scalar value, use the domain of the left-hand side
    if( it->second->get_type() == VEC_SCALAR ){
      assert( dynamic_cast<domain_node*>(it->first) &&  dynamic_cast<domain_node*>(it->first)->check_defined());
      temp_domain->init_domain(static_cast<domain_node*>(it->first));
    }
    else{
      ///Get the domain of the right hand side expr
      temp_domain->init_domain(it->second->compute_domain());
      ///Compute the sub-domain accessed
      if( it->second->get_sub_domain() )
	temp_domain->compute_intersection(it->second->get_sub_domain());
   
      temp_domain->realign_domain();
      ///Check if the lhs domain is an offset or a scaling domain
      domain_node* offset_domain = dynamic_cast<domain_node*>(it->first);
      if( offset_domain ){
	temp_domain->add_offset(offset_domain);
      }
      else{
	domainfn_node* scaleup_domain = dynamic_cast<domainfn_node*>(it->first);
	temp_domain->compute_scale_up(scaleup_domain);
      }
    }
    
    ///Compute the union of the current domain of the expression and just computed domain
    if( it == output_domains.begin() )
      expr_domain->init_domain(temp_domain);
    else
      expr_domain->compute_union(temp_domain);
  }
  ///Relaign the output domain
  expr_domain->realign_domain();
  delete temp_domain;
  return expr_domain;
}


void fnid_expr_node::add_arg(expr_node* curr_scalar_expr)
{
  args.push_back(arg_info(curr_scalar_expr,new bdy_info(B_NONE)));
  curr_scalar_expr->add_usage(this);
}


void fnid_expr_node::find_definition(const char* st) {
  fn_defn = fn_defs->find_fn_defn(st);
  const deque<vector_defn_node*>& curr_params = fn_defn->get_args();
  if( args.size() != curr_params.size() ){
    fprintf(stderr,"[ME] : IN function call %s, mismatch in number of arguments\n",st);
    exit(1);
  }
  vectorfn_defn_node* is_vector_fn = dynamic_cast<vectorfn_defn_node*>(fn_defn);
  deque<vector_defn_node*>::const_iterator it = curr_params.begin();
  int dim = 0;
  
  for( deque<arg_info>::iterator jt = args.begin() ; jt != args.end() ; it++,jt++,dim++ ){
    if( (*it)->get_dim() != jt->arg_expr->get_dim() ){
      fprintf(stderr,"[ME] : In function call to %s, argument %d, mismatch in the dimensionality of the argument\n",st,dim);
      exit(1);
    }
    if( is_vector_fn ){
      if( jt->bdy_condn->type != B_NONE) {
	fprintf(stderr,"[ME] : In function call to %s, vector function argument defined with a boundary condn, not supported\n",st);
	exit(1);
      }
    }
    if( (*it)->get_data_type().type != (*it)->get_data_type().type ){
      fprintf(stderr,"[ME] : In function call to %s, argument %d, mismatch in type of argument\n",st,dim);
      exit(1);
    }
    else if( (*it)->get_data_type().type == T_STRUCT ){
      if( (*it)->get_data_type().struct_info != jt->arg_expr->get_data_type().struct_info ){
	fprintf(stderr,"[ME] : In function call to %s, argument %d, mismatch in type of argument\n",st,dim);
	exit(1);
      }
    }

  }
  ndims = fn_defn->get_return_dim();
  elem_type.assign(fn_defn->get_data_type());
  base_elem_type.assign(elem_type);
  id_name.assign(st);
}
	 

const domain_node* fnid_expr_node::compute_domain()
{
  deque<const domain_node*> arg_domains;
  for( deque<arg_info>::iterator it = args.begin(); it != args.end() ; it++ ){
    ///Comptue the domains of each of the arguments
    domain_node* temp_domain = new domain_node(it->arg_expr->compute_domain());
    if( it->arg_expr->get_sub_domain() )
      temp_domain->compute_intersection(it->arg_expr->get_sub_domain());
    arg_domains.push_back(temp_domain);
  }  

  ///Call the compute domain function of fn_defn, it returns a realigned domain by default
  expr_domain->init_domain(fn_defn->compute_domain(arg_domains));
  
  for( deque<const domain_node*>::iterator it = arg_domains.begin() ; it != arg_domains.end() ; it++ ){
    delete (*it);
  }

  return expr_domain;
}
