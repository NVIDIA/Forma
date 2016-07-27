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
#ifndef __FORWARD_EXPR_HPP__
#define __FORWARD_EXPR_HPP__

#include "ASTVisitor/visitor.hpp"
#include "ASTVisitor/copy_visitor.hpp"

class ForwardExpr : public ASTVisitor<void*> {
  
private :

  std::deque<stmt_node*> remove_statements;

  void visit_vectorfn(vectorfn_defn_node* curr_fn, void * state){
    ASTVisitor::visit_vectorfn(curr_fn,state);
    for( std::deque<stmt_node*>::iterator it = remove_statements.begin() ; it != remove_statements.end() ; it++ ){
      // printf("Removing statement : %s\n",(*it)->get_name());
      remove_statement(*it);
    }
    remove_statements.clear();
  }

  stmt_node* visit_single_stmt(stmt_node* curr_stmt, void* state){
    //printf("stmt : %s , number of uses : %d\n",curr_stmt->get_name(),(int)curr_stmt->get_usage().size());
    ASTVisitor::visit_single_stmt(curr_stmt,state);
    return NULL;
  }

  vector_expr_node* visit_vec_id_expr(vec_id_expr_node* curr_expr, void * state){
    ///This is valid only for references not using loop-iterator
    ///Maybe extendible to expression that use a loop-iterator but do not use an offset
    if( curr_expr->get_access_iterator() == NULL && curr_expr->get_sub_domain() == NULL ){
      const std::set<vector_expr_node*>& defns = curr_expr->get_defn();
      assert( defns.size() == 1 );
      stmt_node* curr_defn = dynamic_cast<stmt_node*>(*(defns.begin()));
      ///Ignore references to function arguments or statements which have an offset specified on LHS
      if( curr_defn && curr_defn->get_sub_domain() == NULL && curr_defn->get_scale_domain() == NULL ){
	/// Do this only if the defining statement has only one use
	if( curr_defn->get_usage().size() == 1 ){ 
	  assert((*(curr_defn->get_usage().begin()) == curr_expr) && "[ME] : Error! : Mismatch in the def-usage realtionship while forwarding exprs\n");
	  CopyVectorExpr expr_copier;
	  remove_statements.push_back(curr_defn);
	  vector_expr_node* copy_defn_rhs = expr_copier.copy(curr_defn->get_rhs());
	  return copy_defn_rhs;
	}
      }
    }
    return NULL;
  }
};





#endif
