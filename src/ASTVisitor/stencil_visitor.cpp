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
#include "ASTVisitor/stencil_visitor.hpp"

void StencilExprVisitor::visit_stmt(pt_stmt_node* curr_stmt)
{
  expr_node* rhs_expr = visit(curr_stmt->rhs_expr);
  if( rhs_expr ){
    delete curr_stmt->rhs_expr;
    curr_stmt->rhs_expr = rhs_expr;
  }
}



expr_node* StencilExprVisitor::visit(expr_node* curr_expr)
{
  switch(curr_expr->get_s_type()){
  case S_VALUE:
    return NULL;
  case S_UNARYNEG:
    return visit_unary_expr(static_cast<unary_neg_expr_node*>(curr_expr));
  case S_ID:
    return visit_id(static_cast<id_expr_node*>(curr_expr));
  case S_MATHFN:
    return visit_math_fn(static_cast<math_fn_expr_node*>(curr_expr));
  case S_TERNARY:
    return visit_ternary(static_cast<ternary_expr_node*>(curr_expr));
  case S_BINARYOP:
    return visit_expr_op(static_cast<expr_op_node*>(curr_expr));
  case S_STENCILOP:
    return visit_stencil_op(static_cast<stencil_op_node*>(curr_expr));
  case S_STRUCT:
    return visit_pt_struct(static_cast<pt_struct_node*>(curr_expr));
  case S_ARRAYACCESS:
    return visit_array_access(static_cast<array_access_node*>(curr_expr));
  default:
    assert(0);
  }
  return NULL;
}



expr_node* StencilExprVisitor::visit_unary_expr(unary_neg_expr_node* curr_expr)
{
  expr_node* new_expr = visit(curr_expr->base_expr);
  if( new_expr ){
    delete curr_expr->base_expr;
    curr_expr->base_expr = new_expr;
  }
  return NULL;
}



expr_node* StencilExprVisitor::visit_math_fn(math_fn_expr_node* curr_expr)
{
  std::deque<expr_node*>& args = curr_expr->fn_args;
  for( std::deque<expr_node*>::iterator it = args.begin() ; it != args.end() ; it++ ){
    expr_node* new_expr = visit(*it);
    if( new_expr ){
      delete (*it);
      (*it) = new_expr;
    }
  }
  return NULL;
}


expr_node* StencilExprVisitor::visit_ternary(ternary_expr_node* curr_expr)
{
  expr_node* new_bool_expr = visit(curr_expr->bool_expr);
  if( new_bool_expr ){
    delete curr_expr->bool_expr;
    curr_expr->bool_expr = new_bool_expr;
  }
  expr_node* new_true_expr = visit(curr_expr->true_expr);
  if( new_true_expr ){
    delete curr_expr->true_expr;
    curr_expr->true_expr = new_true_expr;
  }
  expr_node* new_false_expr = visit(curr_expr->false_expr);
  if( new_false_expr ){
    delete curr_expr->false_expr;
    curr_expr->false_expr = new_false_expr;
  }
  return NULL;
}


expr_node* StencilExprVisitor::visit_expr_op(expr_op_node* curr_expr)
{
  expr_node* new_lhs_expr = visit(curr_expr->lhs_expr);
  if( new_lhs_expr ){
    delete curr_expr->lhs_expr;
    curr_expr->lhs_expr = new_lhs_expr;
  }
  expr_node* new_rhs_expr = visit(curr_expr->rhs_expr);
  if( new_rhs_expr ){
    delete curr_expr->rhs_expr;
    curr_expr->rhs_expr = new_rhs_expr;
  }
  return NULL;
}


expr_node* StencilExprVisitor::visit_pt_struct(pt_struct_node* curr_expr)
{
  for( std::deque<expr_node*>::iterator it = curr_expr->field_expr.begin() ; it != curr_expr->field_expr.end() ; it++){
    expr_node* new_expr = visit(*it);
    if( new_expr ){
      delete (*it);
      (*it) = new_expr;
    }
  }
  return NULL;
}

expr_node* StencilExprVisitor::visit_array_access(array_access_node* curr_expr)
{
  for( std::deque<expr_node*>::iterator it = curr_expr->index_exprs.begin() ;it != curr_expr->index_exprs.end() ; it++){
    expr_node* new_expr = visit(*it);
    if( new_expr ){
      delete (*it);
      (*it) = new_expr;
    }
  }
  return NULL;
}
