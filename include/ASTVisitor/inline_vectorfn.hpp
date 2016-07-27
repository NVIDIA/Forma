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
#ifndef __INLINE_VECTORFN_HPP__
#define __INLINE_VECTORFN_HPP__

#include "ASTVisitor/visitor.hpp"
#include <map>
#include <set>
#include <deque>

class ReplaceFnParams : public ASTVisitor<void*>{

public:

  ReplaceFnParams(std::map<const vector_defn_node*, const vector_expr_node*> curr_fn_bindings) :
    fn_bindings(curr_fn_bindings) { }		  

private:
  
  const std::map<const vector_defn_node*, const vector_expr_node*> fn_bindings;
  
  vector_expr_node* visit_vec_id_expr(vec_id_expr_node* curr_expr, void* state){
    const std::set<vector_expr_node*> defn = curr_expr->get_defn();
    if( defn.size() == 1 ){
      vector_defn_node* curr_defn = dynamic_cast<vector_defn_node*>(*(defn.begin()));
      if( curr_defn ){
	std::map<const vector_defn_node*, const vector_expr_node*>::const_iterator curr_arg = fn_bindings.find(curr_defn);
	assert((curr_arg != fn_bindings.end()) && "[ME] : Error ! Not able to find the arg for parameter\n");
	CopyVectorExpr my_copier;
	return my_copier.copy(curr_arg->second);
      }
    }
    return NULL;
  }

};



class InlineVectorfn : public ASTVisitor<void*> {

public:

  ~InlineVectorfn() { }

private:

  std::deque<std::pair<stmt_node*,stmt_node*> > new_stmts;

  std::string get_new_var(){
    static int var_num = 0;
    std::stringstream new_stream;
    new_stream << "__inline_vectorfn_" << var_num++ << "__" ;
    return new_stream.str();
  }

  void visit_vectorfn(vectorfn_defn_node* curr_function, void* state){
    ASTVisitor::visit_vectorfn(curr_function,state);
    for( std::deque<std::pair<stmt_node*,stmt_node*> >::iterator it = new_stmts.begin() ; it != new_stmts.end() ; it++ ){
      insert_before(it->first,it->second);
    }
    new_stmts.clear();
  }

  vector_expr_node* visit_fnid_expr(fnid_expr_node* curr_fn_call, void* state){
    ASTVisitor::visit_fnid_expr(curr_fn_call,state);
    
    const vectorfn_defn_node* curr_fn_defn = dynamic_cast<const vectorfn_defn_node*>(curr_fn_call->get_defn());
    if( curr_fn_defn && curr_fn_defn->get_body().size() == 0 && curr_fn_defn->get_return_expr()->get_type() == VEC_FN ){ ///The vector function is inlined only if it has no statements and return expr is a function call
	
      const std::deque<arg_info> curr_args = curr_fn_call->get_args();
      const std::deque<vector_defn_node*> curr_params = curr_fn_defn->get_args();

      std::map<const vector_defn_node*, const vector_expr_node*> fn_bindings;
	
      std::deque<arg_info>::const_iterator curr_arg_iterator = curr_args.begin();
      for( std::deque<vector_defn_node*>::const_iterator curr_params_iterator = curr_params.begin() ; 
	   curr_params_iterator != curr_params.end() ; curr_params_iterator++,curr_arg_iterator++){
	CopyVectorExpr copy_arg;
	const vector_defn_node* curr_param = *curr_params_iterator;
	const vector_expr_node* curr_arg = curr_arg_iterator->arg_expr;

	///If the argument is used multiple times in the vector function,
	///precompute this variable
	if( curr_param->get_type() != VEC_DEFN && curr_param->get_usage().size() > 1 ){ 
	  std::string new_variable = get_new_var();
	  stmt_node* new_stmt = new stmt_node(new_variable.c_str(),copy_arg.copy(curr_arg));
	  curr_fn_symbols->add_local_symbol(new_variable.c_str(),new_stmt);
	  vec_id_expr_node* new_var_ref = new vec_id_expr_node(new_variable.c_str(),curr_fn_symbols);
	  fn_bindings.insert(std::pair<const vector_defn_node*,const vector_expr_node*>(curr_param,new_var_ref));
	  new_stmts.push_back(std::pair<stmt_node*,stmt_node*>(curr_visiting_stmt,new_stmt));
	}
	else{
	  fn_bindings.insert(std::pair<const vector_defn_node*, const vector_expr_node*>(curr_param,copy_arg.copy(curr_arg)));
	}
      }
	
      CopyVectorExpr my_copier;
      vector_expr_node* new_expr = my_copier.copy(curr_fn_defn->get_return_expr());

      ReplaceFnParams replace_params(fn_bindings);
      replace_params.visit_vector_expr(new_expr,NULL);

      for( std::map<const vector_defn_node*, const vector_expr_node*>::iterator it = fn_bindings.begin() ; it != fn_bindings.end() ; it++ ){
	delete it->second;
      }

      return new_expr;
    }
    else
      return NULL;
  }

};


#endif
