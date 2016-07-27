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
#include "AST/domain.hpp"
#include <cassert>

using namespace std;


void domain_node::init_domain(const domain_node* inp_domain)
{
  for( deque<range_coeffs>::iterator it = range_list.begin(); it != range_list.end() ; it++ ){
    delete it->lb;
    delete it->ub;
  }
  range_list.clear();
  for( deque<range_coeffs>::const_iterator it = inp_domain->range_list.begin() ; it != inp_domain->range_list.end() ; it++ ){
    //assert(("[ME] : ERROR! : While initializing, base domain is undefined\n", !it->lb->is_default() && ! it->ub->is_default()));
    range_list.push_back(range_coeffs(parametric_exp::copy(it->lb),parametric_exp::copy(it->ub)));
    //printf("Allocated : %p , %p\n",range_list.back().lb,range_list.back().ub);
  }
}

bool domain_node::check_defined() const
{
  for( deque<range_coeffs>::const_iterator it = range_list.begin(); it != range_list.end() ; it++ ){
    if( it->lb->is_default() || it->ub->is_default() ){
      return false;
    }
  }
  return true;
}


void domain_node::realign_domain()
{
  for( deque<range_coeffs>::iterator it = range_list.begin(); it != range_list.end() ; it++){
    it->ub = it->ub->subtract(it->lb);
    delete it->lb;
    it->lb = new int_expr(0);
  }
}

void domain_node::compute_intersection(const domain_node* sub_domain)
{
  assert(sub_domain->get_dim() == (int)range_list.size());
  ///Range in sub_domain is a..b 
  deque<range_coeffs>::const_iterator it = sub_domain->range_list.begin();
  ///Range in expr_domain is c..d ,
  deque<range_coeffs>::iterator jt = range_list.begin();
  int dim_num = 0;
  for( ; it != sub_domain->range_list.end() ; it++, jt++, dim_num++){
    assert( (!jt->lb->is_default() && ! jt->ub->is_default()) && "[ME] : ERROR! : While computing intersection, base domain is undefined\n");
    parametric_exp *low,*high;
    low = ( it->lb->is_default() ? jt->lb : jt->lb->max(it->lb) );
    high = ( it->ub->is_default() ? jt->ub : jt->ub->min(it->ub) );
    // jt->lb = new int_expr(0);
    // jt->ub = high->subtract(low,true);
    jt->lb = low;
    jt->ub = high;
  }
}


void domain_node::compute_scale_down(const domainfn_node* scale_fn)
{
  assert(scale_fn->get_dim() == (int)range_list.size());
  ///Scale fn is (c,d)
  deque<scale_coeffs>::const_iterator it = scale_fn->scale_fn.begin();
  ///Range in sub_domain is a..b
  deque<range_coeffs>::iterator jt = range_list.begin();
  int dim_num = 0;
  for( ; it != scale_fn->scale_fn.end() ; it++,jt++,dim_num++){
    assert(( !jt->lb->is_default() && ! jt->ub->is_default()) && "[ME] : ERROR! : While computing scale_down, base domain is undefined\n");
    assert( (jt->lb->type == P_INT && static_cast<const int_expr*>(jt->lb)->value == 0 ) && "[ME] : Failed assumption that lower bound of scaled down domain is always 0");
    parametric_exp *low,*high;
    if( it->scale == 1 ){
      low = (jt->lb);
      high = (jt->ub);
    }
    else{
      // low = jt->lb->subtract(it->offset)->ceil(it->scale);   //CEIL(jt->lb - it->offset,it->scale);
      // high = jt->ub->subtract(it->offset)->divide(it->scale); // (jt->ub - it->offset) / it->scale;	
      low = jt->lb;
      high = jt->ub->add(1)->divide(it->scale)->subtract(1);
    }
    // jt->lb = new int_expr(0);
    // jt->ub = high->subtract(low,true);
    jt->lb = low;
    jt->ub = high;
  }
}


void domain_node::compute_scale_up(const domainfn_node* scale_fn)
{
  assert(scale_fn->get_dim() == (int)range_list.size());
  ///Scale fn is (c,d)
  deque<scale_coeffs>::const_iterator it = scale_fn->scale_fn.begin();
  ///Range in sub_domain is a..b
  deque<range_coeffs>::iterator jt = range_list.begin();
  int dim_num = 0;
  for( ; it != scale_fn->scale_fn.end() ; it++,jt++,dim_num++){
    assert((!jt->lb->is_default() && ! jt->ub->is_default()) && "[ME] : ERROR! : While computing scale_up, base domain is undefined\n");
    assert( (jt->lb->type == P_INT && static_cast<const int_expr*>(jt->lb)->value == 0 ) && "[ME] : Failed assumption that lower bound of scaled up domain is always 0");
    ///For now, the output domain, always starts at 0, and ends at c+b*d
    parametric_exp *low,*high;
    if( it->scale == 1 ){
      low = jt->lb;
      high = jt->ub;
    }
    else{
      // int min_input = CEIL( 0 - it->offset , it->scale );
      // low = MIN(0,it->offset + it->scale * min_input);
      delete jt->lb;
      low = new int_expr(0); // (it->offset < 0 ? new int_expr(it->scale + it->offset) : new int_expr(it->offset) );
      // high = jt->ub->multiply(it->scale)->add(it->offset);        /// it->offset + jt->ub * it->scale;
      high = jt->ub->add(1)->multiply(it->scale)->subtract(1);
    }
    // jt->lb = new int_expr(0);
    // jt->ub = high->subtract(low,true);
    jt->lb = low;
    jt->ub = high;
  }
}


void domain_node::compute_union(const domain_node* union_domain)
{
  assert(range_list.size() == union_domain->range_list.size());
  ///The range of the input domain is c..d
  deque<range_coeffs>::const_iterator it = union_domain->range_list.begin();
  ///The range of the current domain is a..b
  deque<range_coeffs>::iterator jt = range_list.begin();
  int dim_num = 0;
  for( ; it != union_domain->range_list.end() ; it++, jt++, dim_num++){
    assert((!jt->lb->is_default() && ! jt->ub->is_default()) && "[ME] : ERROR! : While computing union, base domain is undefined\n");

    parametric_exp* low = ( it->lb->is_default() ? jt->lb : jt->lb->min(it->lb));
    parametric_exp* high = ( it->ub->is_default() ? jt->ub : jt->ub->max(it->ub));
    // jt->lb = new int_expr(0);
    // jt->ub = high->subtract(low,true);
    jt->lb = low;
    jt->ub = high;
  }
}


int domain_node::check_if_offset() const
{
  for( deque<range_coeffs>::const_iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
    if( !it->ub->is_default() ){
      return 0;
    }
  }
  return 1;
}

void domain_node::add_offset(const domain_node* offset_info)
{
  assert(offset_info->get_dim() >= (int)range_list.size()) ;
  deque<range_coeffs>::const_reverse_iterator it = offset_info->range_list.rbegin() ; 
  deque<range_coeffs>::reverse_iterator jt = range_list.rbegin(); 
  int dim_num = 0;
  for( ; jt != range_list.rend() ; it++, jt++, dim_num++ ){
    //assert(it->lb == it->ub && it->lb != DEFAULT_RANGE);
    if (! it->lb->is_default() ){
      jt->ub = jt->ub->add(it->lb);
    }
  }
  for( ; it != offset_info->range_list.rend() ; it++ ){
    range_list.insert(range_list.begin(),range_coeffs(new int_expr(0),parametric_exp::copy(it->lb)));
  }
  // range_list.push_back(range_coeffs(0,it->lb));
}


// void domain_node::compute_stencil_domain(const deque<const domainfn_node*>& stencil_accesses)
// {
//   assert(stencil_bounds.size() == range_list.size() );
//   deque<range_coeffs>::iterator it = range_list.begin();
//   deque<stencil_access_info>::const_iterator jt = stencil_bounds.begin();
//   int dim_num = 0;
//   for( ; jt != stencil_bounds.end() ; jt++ ,it++, dim_num++ ){
//     assert(it->lb != DEFAULT_RANGE && it->ub != DEFAULT_RANGE) ;
//     if( jt->min_scale == 1 ){
//       int low = it->lb - jt->min_offset;
//       int high = it->ub - jt->max_offset;
//       if( low > high ) {
// 	fprintf(stderr,"[ME] : ERROR!! : At dim %d, Computed range of output of stencil experssion [%d,%d] has no points ",dim_num, low,high);
// 	exit(1);
//       }
//       it->lb = 0;
//       it->ub = high - low;
//     }
//     else{
//       int npoints = ( it->ub / jt->min_scale ) - ( it->lb / jt->scale ) + 1;
//       if( npoints <= 0 ){
// 	fprintf(stderr,"[ME] : ERROR!! : At dim %d, the number of points in output domain is 0",dim_num);
// 	exit(1);
//       } 
//     }
//   }
// }
