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
#ifndef __DOMAIN_HPP__
#define __DOMAIN_HPP__

#include <deque>
#include <cstdio>
#include <cstdlib>
#include "AST/parametric.hpp"

#define DEFAULT_OFFSET 0

struct scale_coeffs{
  int offset;
  int scale;
  scale_coeffs(int a, int b) : offset(a), scale(b) { }
};

struct range_coeffs{
  // int lb;
  // int ub;
  // range_coeffs(int a, int b) : lb(a), ub(b) { }
  parametric_exp* lb;
  parametric_exp* ub;
  range_coeffs(parametric_exp* a, parametric_exp* b) : lb(a), ub(b) { }
  ~range_coeffs(){
    // delete lb; 
    // delete ub; 
  }
};


/**Base class to represent a domain descriptor
 */
struct domain_desc_node : public ast_node{

  ///Virtual function for printing, overloaded by inherited classes
  virtual void print_node(FILE*) const=0;

  ///Virtual destructor
  virtual ~domain_desc_node() {}

  ///Virtual function to get the domain dimenstionality
  virtual int get_dim() const=0;

};


/**Class to represent domain functions for interpolations and extrapolations
   domainfn ::= '@[' offset+ ']'
   offset::= '(' int ',' int ')' | int
*/
struct domainfn_node: public domain_desc_node{

  ///The scale co-efficients to be used for each dimension, <a,b> = a + b * i
  std::deque<scale_coeffs> scale_fn;

  domainfn_node() { };

  domainfn_node(int size){
    for( int i = 0 ; i < size ; i++ ){
      scale_fn.push_back(scale_coeffs(0,1));
    }
  }
  
  ///Method to add scale co-effs when b > 1, used by the parser
  inline void add_scale_coeffs(int a, int b){
    if( b <= 0 ){
      fprintf(stderr,"[ME] : Error! Scale co-efficient cannot be less than or equal to 0 in (%d,%d)\n",a,b);
      exit(1);
    }
    // if( a > b ){
    //   fprintf(stderr,"[ME] : Error! Offset cannot be more than or equal to the scale-coefficient when specified as '(' ')' in (%d,%d)\n",a,b);
    //   exit(1);
    // }
    
    scale_fn.push_back(scale_coeffs(a,b));  // a + b * i
  }

  ///Method to add scale coeffs when b == 1 , used by the parser
  inline void add_scale_coeffs(int a){
    scale_fn.push_back(scale_coeffs(a,1));
  }

  inline int get_dim() const { 
    return scale_fn.size();
  }
  
  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();

  ///Destructor
  ~domainfn_node(){
    scale_fn.clear();
  }
};


/**Class to represent the domain productions, for contiguous domains
   domain ::= '[' range+ ']'
   ranglist ::= '(' int ',' int ')' | int 
*/
struct domain_node : public domain_desc_node{

  ///The range for each dimension 
  std::deque<range_coeffs> range_list;

  ///Constructor
  domain_node(){ }
  domain_node(parametric_exp* a){
    add_range(a);
  } 
  domain_node(parametric_exp* a, parametric_exp* b){
    add_range(a,b);
  }
  domain_node(const domain_node* copy_from){
    init_domain(copy_from);
  }
  
  ///Helper function to add range for a dimension, used by the parser
  inline void add_range(parametric_exp* a ,parametric_exp* b){
    // if( a < 0 || b < a ){
    //   fprintf(stderr,"[ME] : Range specified for domain cant be negetive\n");
    //   exit(1);
    // }
    range_list.push_back(range_coeffs(a,b));
  }

  ///Helper function to add range as a single point, used by the parser
  inline void add_range(parametric_exp* a){
    // if( a < 0 ){
    //   fprintf(stderr,"[ME] : Error! Value specified in domain cant be negetive\n");
    //   exit(1);
    // }
    range_list.push_back(range_coeffs(a,parametric_exp::copy(a)));
  }

  ///Helper function to add size for a dimension, used by the parser , the start is assumed to be from 0
  inline void add_size(parametric_exp* a){
    range_list.push_back(range_coeffs(new int_expr(0),a->subtract(1)));
  }

  ///Inherited function to get dimensionality of the specified domain fn
  inline int get_dim()const{
    return range_list.size();
  }

  ///Returns the domains ranges
  inline const std::deque<range_coeffs>& get_domain() const {
    return range_list;
  }

  void init_domain(const domain_node*);
  //void init_domain(int);
  void realign_domain();
  void compute_intersection(const domain_node*);
  void compute_union(const domain_node*);
  void compute_scale_up(const domainfn_node*);
  void compute_scale_down(const domainfn_node*);
  void add_offset(const domain_node*);
  // void compute_stencil_domain(const std::deque<const domainfn_node*>&);
  int check_if_offset()const;
  bool check_defined() const;

  ///Overloaded method used for pretty printing
  void print_node(FILE*) const;

  void pretty_print() const;
  int compute_pretty_print_size();
  
  // int compute_simpleC_size();
  // void print_simpleC() const;

  ///Destructor
  ~domain_node(){
    for( std::deque<range_coeffs>::iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
      //printf("Deleting : %p , %p \n",it->lb,it->ub);
      delete it->lb;
      delete it->ub;
    }
    range_list.clear();
  }
};




#endif
