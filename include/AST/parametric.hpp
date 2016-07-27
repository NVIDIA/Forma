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
#ifndef __PARAMTERIC_HPP__
#define __PARAMTERIC_HPP__

#include "AST/local_scalars.hpp"
#include <string>
#include <cstdio>
#include <cassert>

#define DEFAULT_RANGE -1
#define MIN(a,b) ( a < b ? a : b )
#define MAX(a,b) ( a > b ? a : b )
// #define CEIL(a,b) ( ( ( (a) % (b) == 0 ) ) ? ( (a) / (b) ) : ( ( (a) / (b) ) + 1 ) )
#define CEIL(a,b) (((a)<0)?((a)/(b)):(((a)%(b)==0)?((a)/(b)):(((a)/(b))+1)))
// #define FLOOR(a,b) ( ( ( (a) >= 0 ) || ( (a) % (b) == 0) ) ? ((a)/(b)) : ( ((a)/(b)) - 1 ) )
#define FLOOR(a,b) (((a)>=0)?((a)/(b)):(((a)%(b)==0)?((a)/(b)):(((a)/(b))-1)))


///Virtual Base class used for a node in the AST
class ast_node{ 

protected:

  ///Field used for storing the pretty print size
  int pretty_print_size;

  ///Field used for storing the C print size, might be merged with
  ///above later
  // int print_C_size;

public:
  
  ///Virtual function used for pretty printing
  virtual void print_node(FILE*) const =0;

  ///Destructor
  virtual ~ast_node() {}

  ///Functions used for pretty printing
  virtual int compute_pretty_print_size() = 0;
  virtual void pretty_print() const =0;
  inline int get_pretty_print_size(){
    return pretty_print_size;
  }
  
  // ///Functions for C codegen - to be made pure virtual
  // virtual void print_simpleC() const { }
  // virtual int compute_simpleC_size() { }
  // virtual inline int get_print_C_size() {
  //   return print_C_size;
  // }
};


enum parametric_exp_type{
  P_INT,
  P_PARAM,
  P_ADD,
  P_SUBTRACT,
  P_DIVIDE,
  P_MULTIPLY,
  P_CEIL,
  P_MAX,
  P_MIN,
  P_DEFAULT,
};


struct parametric_exp : public ast_node{

public:
  const parametric_exp_type type;

  bool is_default() const { return type == P_DEFAULT; }

  virtual parametric_exp* add(parametric_exp* rhs, bool no_copy = false) =0;
  
  virtual parametric_exp* subtract( parametric_exp* rhs, bool no_copy = false)=0;

  virtual parametric_exp* multiply( parametric_exp* rhs, bool no_copy = false)=0;

  virtual parametric_exp* divide( parametric_exp* rhs, bool no_copy = false)=0;

  virtual parametric_exp* ceil( parametric_exp* rhs, bool no_copy = false)=0;
  
  virtual parametric_exp* max( parametric_exp* rhs, bool no_copy = false)=0;

  virtual parametric_exp* min( parametric_exp* rhs, bool no_copy = false)=0;

  virtual parametric_exp* add(int) =0;
  
  virtual parametric_exp* subtract( int)=0;

  virtual parametric_exp* multiply( int)=0;

  virtual parametric_exp* divide( int)=0;

  virtual parametric_exp* ceil( int)=0;
  
  virtual parametric_exp* max( int)=0;

  virtual parametric_exp* min( int)=0;

  static bool is_equal(const parametric_exp* lhs, const parametric_exp* rhs);
  
  static parametric_exp* copy(const parametric_exp* rhs);

  parametric_exp(parametric_exp_type curr_type) : type(curr_type) { }

  virtual ~parametric_exp() { }
};


struct parameter_defn : public ast_node{

  const std::string param_id;

  const int value;
  
  parameter_defn(const char* id):
    param_id(id),
    value(DEFAULT_RANGE)
  { }

  parameter_defn(const char* id, int val):
    param_id(id),
    value(val)
  { }

  void print_node(FILE*) const;
  int compute_pretty_print_size() ;
  void pretty_print() const;
  
};

struct default_expr : public parametric_exp{

  default_expr() : parametric_exp(P_DEFAULT) { }
  
  parametric_exp* add(parametric_exp* rhs, bool no_copy = false) { assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }
  
  parametric_exp* subtract( parametric_exp* rhs, bool no_copy = false){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* multiply( parametric_exp* rhs, bool no_copy = false){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* divide( parametric_exp* rhs, bool no_copy = false){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* ceil( parametric_exp* rhs, bool no_copy = false){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }
  
  parametric_exp* max( parametric_exp* rhs, bool no_copy = false){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* min( parametric_exp* rhs, bool no_copy = false){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* add(int) { assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }
  
  parametric_exp* subtract( int){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* multiply( int){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* divide( int){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* ceil( int){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }
  
  parametric_exp* max( int){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }

  parametric_exp* min( int){ assert((0) && ("[ME]: Error! Operation on an undefined parametric expr\n")); return NULL; }


  int compute_pretty_print_size() { return 0; }
  void pretty_print() const { }
  void print_node(FILE*) const { }
  
};


struct int_expr : public parametric_exp{
  int_expr(int val) :parametric_exp(P_INT),  value(val) { }

  int value;

  parametric_exp* add(parametric_exp* rhs, bool no_copy = false) ;
  
  parametric_exp* subtract( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* multiply( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* divide( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* ceil( parametric_exp* rhs, bool no_copy = false);
  
  parametric_exp* max( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* min( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* add(int) ;
  
  parametric_exp* subtract( int);

  parametric_exp* multiply( int);

  parametric_exp* divide( int);

  parametric_exp* ceil( int);
  
  parametric_exp* max( int);

  parametric_exp* min( int);

  int compute_pretty_print_size();
  void pretty_print() const;
  void print_node(FILE*) const;
};

struct param_expr : public parametric_exp{
  param_expr(const parameter_defn* defn) :  parametric_exp(P_PARAM), param(defn){ }

  const parameter_defn* param;
  //const std::string param;

  parametric_exp* add(parametric_exp* rhs, bool no_copy = false) ;
  
  parametric_exp* subtract( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* multiply( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* divide( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* ceil( parametric_exp* rhs, bool no_copy = false);
  
  parametric_exp* max( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* min( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* add(int) ;
  
  parametric_exp* subtract( int);

  parametric_exp* multiply( int);

  parametric_exp* divide( int);

  parametric_exp* ceil( int);
  
  parametric_exp* max( int);

  parametric_exp* min( int);



  int compute_pretty_print_size();
  void pretty_print() const;
  void print_node(FILE*) const;
};

struct binary_expr : public parametric_exp{
  parametric_exp* lhs_expr;
  parametric_exp* rhs_expr;
  binary_expr( parametric_exp* lhs, parametric_exp* rhs, parametric_exp_type curr_type):
    parametric_exp(curr_type) ,
    lhs_expr(lhs),
    rhs_expr(rhs) { 
    if( curr_type == P_MULTIPLY && ( rhs_expr->type != P_INT && lhs_expr->type != P_INT )){
      fprintf(stderr,"[ME] : Error : Multiplying parametric expressions is unsupported\n");
      exit(1);
    }
  }
  
  parametric_exp* add(parametric_exp* rhs, bool no_copy = false) ;
  
  parametric_exp* subtract( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* multiply( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* divide( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* ceil( parametric_exp* rhs, bool no_copy = false);
  
  parametric_exp* max( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* min( parametric_exp* rhs, bool no_copy = false);

  parametric_exp* add(int) ;
  
  parametric_exp* subtract( int);

  parametric_exp* multiply( int);

  parametric_exp* divide( int);

  parametric_exp* ceil( int);
  
  parametric_exp* max( int);

  parametric_exp* min( int);

  ~binary_expr(){
    //printf("Deleting : %p , %p \n",lhs_expr,rhs_expr);
    delete lhs_expr;
    delete rhs_expr;
  }
  int compute_pretty_print_size();
  void pretty_print() const;
  void print_node(FILE*) const;
};


typedef symbol_table<parameter_defn*> param_table;

extern param_table* global_params;

#endif
