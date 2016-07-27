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
#ifndef __UNROLL_HPP__
#define __UNROLL_HPP__

#include "ASTVisitor/visitor.hpp"
#include "ASTVisitor/copy_visitor.hpp"

class LoopUnroll : public ASTVisitor<int>{

private:

  std::deque<std::pair<for_stmt_node*,for_stmt_seq*> > replacements;

  void visit_vectorfn(vectorfn_defn_node* curr_fn, int state){
    ASTVisitor::visit_vectorfn(curr_fn,state);
    for( std::deque<std::pair<for_stmt_node*,for_stmt_seq*> >::iterator it = replacements.begin() ; it != replacements.end() ; it++ ){
      for( std::deque<stmt_node*>::iterator jt = it->second->stmt_list.begin() ; jt != it->second->stmt_list.end() ; jt++ ){
    	insert_before(it->first,*jt);
      }
      remove_statement(it->first);
      delete it->second;
    }
    replacements.clear();
  }

  stmt_node* visit_for_stmt(for_stmt_node* curr_stmt, int state){
    //printf("Found For Stmt\n");
    const std::deque<range_coeffs>& range_list = curr_stmt->get_iterator()->iter_domain->range_list;
    assert((range_list.size() == 1 ) && "[ME] : Error : Currently can handle only loops on single-depth\n");
    replacements.push_back(std::pair<for_stmt_node*,for_stmt_seq*>(curr_stmt,new for_stmt_seq()));
    for( std::deque<range_coeffs>::const_iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
      assert((it->lb->type == P_INT && it->ub->type == P_INT) && ("[ME] : Error : Cant hand parameteric for-loops\n"));
      int lb = static_cast<const int_expr*>(it->lb)->value;
      int ub = static_cast<const int_expr*>(it->ub)->value;
      if( curr_stmt->get_iterator()->is_positive )
	for( int i = lb  ; i <= ub ; i++ ){
	  ASTVisitor::visit_for_stmt(curr_stmt,i);
	}
      else{
	for( int i = ub ; i >= lb ; i-- ){
	  ASTVisitor::visit_for_stmt(curr_stmt,i);
	}
      }
    }
    return NULL;
  }

  stmt_node* visit_single_stmt(stmt_node* curr_stmt, int state){
    if( curr_stmt->get_access_iterator() ){
      //printf("Stmt: %p, state : %d\n",curr_stmt,state);
      std::stringstream new_name;
      
      new_name << "__unroll_" <<  curr_stmt->get_name() << "_" << state << "__";
      CopyVectorExpr expr_copier;
      vector_expr_node* new_rhs = expr_copier.copy(curr_stmt->get_rhs());
      vector_expr_node* modified_new_rhs = visit_vector_expr(new_rhs,state);
      if( modified_new_rhs ){
      	delete new_rhs;
      	new_rhs = modified_new_rhs;
      }
      stmt_node* new_stmt;
      if( curr_stmt->get_sub_domain() )
      	new_stmt = new stmt_node(new_name.str().c_str(),new_rhs,expr_copier.copy(curr_stmt->get_sub_domain()));
      else if( curr_stmt->get_scale_domain() )
      	new_stmt = new stmt_node(new_name.str().c_str(),new_rhs,expr_copier.copy(curr_stmt->get_scale_domain()));
      else
      	new_stmt = new stmt_node(new_name.str().c_str(),new_rhs);
      curr_fn_symbols->add_local_symbol(new_name.str().c_str(),new_stmt);
      replacements.rbegin()->second->stmt_list.push_back(new_stmt);
      //new_rhs->add_usage(new_stmt);
      return NULL;
    }
    else if(curr_stmt->get_offset() != DEFAULT_RANGE){
      //printf("Stmt: %p, state : %d\n",curr_stmt,state);
      std::stringstream new_name;

      new_name << "__unroll_" <<  curr_stmt->get_name() << "_" << curr_stmt->get_offset() << "__";
      CopyVectorExpr expr_copier;
      vector_expr_node* new_rhs = expr_copier.copy(curr_stmt->get_rhs());
      vector_expr_node* modified_new_rhs = visit_vector_expr(new_rhs,state);
      if( modified_new_rhs ){
	delete new_rhs;
	new_rhs = modified_new_rhs;
      }
      stmt_node* new_stmt;
      if( curr_stmt->get_sub_domain() )
	new_stmt = new stmt_node(new_name.str().c_str(),new_rhs,expr_copier.copy(curr_stmt->get_sub_domain()));
      else if( curr_stmt->get_scale_domain() )
	new_stmt = new stmt_node(new_name.str().c_str(),new_rhs,expr_copier.copy(curr_stmt->get_scale_domain()));
      else
	new_stmt = new stmt_node(new_name.str().c_str(),new_rhs);
      //new_rhs->add_usage(new_stmt);
      return new_stmt;
    }
    else{
      return ASTVisitor::visit_single_stmt(curr_stmt,state);
    }
  }
  

  vector_expr_node* visit_vec_id_expr(vec_id_expr_node* curr_expr, int state){
    const for_iterator* curr_access_iterator =  curr_expr->get_access_iterator();
    if( curr_access_iterator ){
      int value = state;
      if( curr_expr->get_offset() != DEFAULT_RANGE ){
    	if( curr_access_iterator->is_positive )
    	  value -= curr_expr->get_offset();
    	else
    	  value += curr_expr->get_offset();
      }
      std::stringstream new_string;
      new_string << "__unroll_" << curr_expr->get_name() << "_" << value << "__" ;
      return new vec_id_expr_node(new_string.str().c_str(),curr_fn_symbols);
    }
    else if( curr_expr->get_offset() != DEFAULT_RANGE) {
      std::stringstream new_string;
      new_string << "__unroll_" << curr_expr->get_name() << "_" << curr_expr->get_offset() << "__" ;
      return new vec_id_expr_node(new_string.str().c_str(),curr_fn_symbols);
    }
    else{
      return ASTVisitor::visit_vec_id_expr(curr_expr,state);
    }
  }


};


#endif
