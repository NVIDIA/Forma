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
#ifndef __VECTOR_DEFN_HPP__
#define __VECTOR_DEFN_HPP__

#include "AST/data_types.hpp"
#include "AST/vector_expr.hpp"

#include <climits>

struct offset_hull{
  int max_negetive;
  int max_positive;
  int scale;
  offset_hull(int a, int b, int c = 1) : max_negetive(a), max_positive(b), scale(c) { }
};


/** Class to represent vector definitions
    vectordefn ::= vector#INT TYPE ID '[' INT+ ']';    
*/
class vector_defn_node : public vector_expr_node{

private:

  ///Type of the elements of the vector
  // data_types elem_type;
  
  ///Vector name
  std::string name;

  ///Description of maximum offsets needed for each input argument of
  ///a stencil function. Used to account for boundary conditions
  std::deque<offset_hull> stencil_access_info;
  bool first_access;
  
  ///Boolean set to true if there is an direct access to an array
  bool has_direct_access;

public:

  ///Constructor 
  vector_defn_node(const data_types* cd, int nd, const char* buf){    
    elem_type.assign(*cd);
    base_elem_type.assign(elem_type);
    ndims = nd;
    name.assign(buf);
    expr_type = VEC_DEFN;
    for( int i = 0 ; i < ndims ; i++ ){
      stencil_access_info.push_back(offset_hull(0,0));
    }
    first_access = true;
    has_direct_access = false;
  }

  ///Constructor with domain info, these are not fn. args to stencil
  ///fns, so no stencil_access_info needed
  vector_defn_node(const data_types* cd, int nd, const char* buf, domain_node* inp_domain){
    elem_type.assign(*cd);
    base_elem_type.assign(elem_type);
    ndims = nd;
    name.assign(buf);
    expr_domain->init_domain(inp_domain);
    expr_type = VEC_DEFN;
    first_access = true;
    has_direct_access = false;
  }

  ///Function to return dimension of the vector defined.
  inline int get_dim() const { 
    return ndims;
  }

  ///Method to compute the domain of the expression
  inline const domain_node* compute_domain(){
    // expr_domain->print_node(stdout);
    // printf("\n");
    return expr_domain;
  }

  ///Method to init the domain
  inline void init_domain(const domain_node* arg_domain){
    expr_domain->init_domain(arg_domain);
  }

  ///Method to get the name of the vector 
  inline const std::string& get_name() const{
    return name;
  }

  ///Method to update the stencil access info
  inline void update_stencil_access_info(const domainfn_node* na){
    const std::deque<scale_coeffs>& scale_fn = na->scale_fn;
    assert(((int)(scale_fn.size()) == ndims) && "[ME]: Parsing Error, scale_fn specified not the same dimension as the fn_parameter, must have been checked before");
    if( first_access ){
      std::deque<offset_hull>::iterator jt = stencil_access_info.begin();
      for( std::deque<scale_coeffs>::const_iterator it = scale_fn.begin() ; it != scale_fn.end() ; it++, jt++ ){
	jt->scale = it->scale;
	if( it->offset < 0 )
	  jt->max_negetive = MIN(jt->max_negetive,it->offset);
	else
	  jt->max_positive = MAX(jt->max_positive,it->offset);
      }
      first_access = false;
    }
    else{
      std::deque<offset_hull>::iterator jt = stencil_access_info.begin();
      for( std::deque<scale_coeffs>::const_iterator it = scale_fn.begin() ; it != scale_fn.end() ; it++, jt++ ){
	if( it->scale != jt->scale ){
	  fprintf(stderr,"[ME] : Currently cannot support using a mixed scale-coefficient within a stencil function, Consider using it outside stencil functions instead\n");
	  exit(1);
	}
	else{
	  jt->scale = it->scale;
	}
	if( it->offset < 0 )
	  jt->max_negetive = MIN(jt->max_negetive,it->offset);
	else
	  jt->max_positive = MAX(jt->max_positive,it->offset);
      }
    }
  }

  ///Method to get the access info
  const std::deque<offset_hull>& get_access_info() const {
    return stencil_access_info;
  }

  ///Set boolean to indicate direct access
  inline void set_direct_access() {
    has_direct_access = true;
  }
  inline bool get_direct_access() const{
    return has_direct_access;
  }
  
  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();

  // int compute_simpleC_size();
  // void print_simpleC() const;
  
  ///Destructor
  ~vector_defn_node(){
    name.clear();
  }

  friend class vectorfn_defn_node;
  friend class stencilfn_defn_node;
  friend class VectorExprVisitor;
};








#endif
