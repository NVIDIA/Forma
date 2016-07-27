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

scalar_sym_info::scalar_sym_info(data_types dt) :
  data_type(dt) 
{ }

local_symbols::~local_symbols()
{
  for( deque<pair<string,vector_expr_node*> >::iterator it = symbols.begin() ; it != symbols.end() ; it++ ){
    multi_stmt_node*  qualified_symbol = dynamic_cast<multi_stmt_node*>(it->second);
    if( qualified_symbol )
      delete qualified_symbol;
  }
  symbols.clear(); 
}



void local_scalar_symbols::add_local_scalar(const char* id_name, const expr_node* curr_expr){
  scalar_sym_info* curr_defn = find_symbol(id_name);
  if( curr_defn == 0 ){
    string new_id(id_name);
    symbols.push_back(pair<string,scalar_sym_info*>(new_id,new scalar_sym_info(curr_expr->get_data_type())));
  }
  else{
    curr_defn->data_type = curr_expr->get_data_type();
  }
}


void local_symbols::remove_local_symbol(stmt_node* curr_stmt)
{
  switch(curr_stmt->get_type() ){
  case VEC_STMT:{
    vector_expr_node* curr_defn = find_symbol(curr_stmt->get_name());
    if( dynamic_cast<multi_stmt_node*>(curr_defn) ){
      static_cast<multi_stmt_node*>(curr_defn)->remove_def_domain(curr_stmt);
    }
    else{
      bool is_removed = remove_symbol(curr_stmt->get_name());
      if( !is_removed ){
	assert((0) && ("[ME] : Error! : Attempting to remove non-existent symbol"));
      }
    }
    return;
  }
  case VEC_FORSTMT: {
    const for_stmt_seq* body = static_cast<for_stmt_node*>(curr_stmt)->get_body();
    for( deque<stmt_node*>::const_iterator jt = body->stmt_list.begin() ; jt != body->stmt_list.end() ; jt++ ){
      vector_expr_node* curr_symbol = find_local_symbol((*jt)->get_name());
      assert(dynamic_cast<multi_stmt_node*>(curr_symbol));
      static_cast<multi_stmt_node*>(curr_symbol)->remove_def_domain(*jt);
    }
    return;
  }
  case VEC_DOSTMT: {
    const for_stmt_seq* body = static_cast<do_stmt_node*>(curr_stmt)->get_body();
    for( deque<stmt_node*>::const_iterator jt = body->stmt_list.begin() ; jt != body->stmt_list.end() ; jt++ ){
      vector_expr_node* curr_symbol = find_local_symbol((*jt)->get_name());
      assert(dynamic_cast<multi_stmt_node*>(curr_symbol));
      static_cast<multi_stmt_node*>(curr_symbol)->remove_def_domain(*jt);
    }
    return;
  }
  default:
    assert(0);
  }
}
