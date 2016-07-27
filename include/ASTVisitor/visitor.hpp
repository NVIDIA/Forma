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
#ifndef __VISITOR_HPP__
#define __VISITOR_HPP__

#include "AST/parser.hpp"

template <typename State> 
class ASTVisitor
{
public: 

  ASTVisitor() { 
    curr_fn_symbols = NULL;
    curr_fn = NULL;
    curr_visiting_stmt = NULL;
  }

  virtual ~ASTVisitor() { }

  virtual void visit(program_node*, State state);

  virtual vector_expr_node* visit_vector_expr(vector_expr_node* expr, State state);

protected:

  local_symbols* curr_fn_symbols;

  vectorfn_defn_node* curr_fn;

  stmt_node* curr_visiting_stmt;

  void insert_before(stmt_node*,stmt_node*);
  
  void remove_statement(stmt_node*);

  virtual void visit_vectorfn(vectorfn_defn_node*, State state);
  
  virtual vector_expr_node* visit_compose_expr(compose_expr_node*, State);

  virtual vector_expr_node* visit_vec_id_expr(vec_id_expr_node* , State ) { return NULL; }

  virtual vector_expr_node* visit_make_struct(make_struct_node*, State);

  virtual vector_expr_node* visit_fnid_expr(fnid_expr_node*, State);

  virtual vector_expr_node* visit_domainfn(vec_domainfn_node*, State);

  virtual stmt_node* visit_stmt(stmt_node*, State);

  virtual stmt_node* visit_single_stmt(stmt_node*, State);

  virtual stmt_node* visit_for_stmt(for_stmt_node*, State);

  virtual stmt_node* visit_do_stmt(do_stmt_node*,State);

  virtual vector_expr_node* visit_vector_defn(vector_defn_node*, State) { return NULL; }

  virtual expr_node* visit_value_expr(expr_node*, State) { return NULL; }
};

template<typename State>
void ASTVisitor<State>::visit(program_node* program, State state)
{
  std::deque<std::pair <std::string,fn_defn_node* > > curr_symbols;
  for( std::deque<std::pair<std::string,fn_defn_node*> >::iterator it = fn_defs->symbols.begin() ; it != fn_defs->symbols.end() ; it++ )
    curr_symbols.push_back(std::pair<std::string,fn_defn_node*>(it->first,it->second));
  for( std::deque<std::pair<std::string,fn_defn_node*> >::iterator it = curr_symbols.begin() ; it != curr_symbols.end() ; it++ ){
    if( dynamic_cast<vectorfn_defn_node*>(it->second) )
      visit_vectorfn(static_cast<vectorfn_defn_node*>(it->second),state);
  }
  visit_vectorfn(program->program_body,state);
}

template<typename State>
void ASTVisitor<State>::visit_vectorfn(vectorfn_defn_node* function, State state)
{
  curr_fn = function;
  curr_fn_symbols = function->fn_symbols;
  
  for( std::deque<vector_defn_node*>::iterator it = function->fn_args.begin() ; it != function->fn_args.end() ; it++){
    visit_vector_expr((*it),state);
  }
  for( std::deque<stmt_node*>::iterator it = function->fn_body.begin() ; it != function->fn_body.end() ; it++ ){
    curr_visiting_stmt = *it;
    stmt_node* new_stmt = visit_stmt((*it),state);
    if( new_stmt ){
      assert(("[ME] : Error! During traversal, replaced expression is not a stmt\n",new_stmt->get_type() == VEC_STMT));
      curr_fn_symbols->remove_local_symbol(*it);
      delete (*it);
      (*it) = static_cast<stmt_node*>(new_stmt);
      curr_fn_symbols->add_local_symbol(new_stmt->get_name(),new_stmt);
    }
  }
  curr_visiting_stmt = NULL;
  
  vector_expr_node* new_ret_expr = visit_vector_expr(function->return_expr,state);
  if( new_ret_expr ){
    delete function->return_expr;
    function->return_expr = new_ret_expr;
  }
}

template<typename State>
void ASTVisitor<State>::insert_before(stmt_node* target_stmt, stmt_node* new_stmt)
{
  assert(("In ASTVisitor, trying to insert statement without setting function scope\n",curr_fn));
  for( std::deque<stmt_node*>::iterator it = curr_fn->fn_body.begin() ; it != curr_fn->fn_body.end() ; it++ ){
    if( *it == target_stmt ){
      curr_fn->fn_body.insert(it,new_stmt);
      return;
    }
  }
  if( target_stmt == NULL || target_stmt == 0 ){ ///If the target_stmt is NULL, just insert at the end of the stmt list
    curr_fn->fn_body.push_back(new_stmt);
    return ;
  }
  assert(("[ME] : Error : Couldnt find target stmt to insert new statement",0));
}


template<typename State>
void ASTVisitor<State>::remove_statement(stmt_node* target_stmt)
{
  assert(("In ASTVisitor, trying to remove statement without setting function scope\n",curr_fn));
  for( std::deque<stmt_node*>::iterator it = curr_fn->fn_body.begin() ; it != curr_fn->fn_body.end() ; it++ ){
    if( *it == target_stmt ){
      curr_fn_symbols->remove_local_symbol(*it);
      delete *it;
      curr_fn->fn_body.erase(it);
      return;
    }
  }
  assert(("[ME] : Error : Couldnt find target stmt to remove",0));
}



template<typename State>
vector_expr_node* ASTVisitor<State>::visit_vector_expr(vector_expr_node* expr, State state)
{
  switch(expr->get_type()){
  case VEC_COMPOSE:
    return visit_compose_expr(static_cast<compose_expr_node*>(expr),state);
  case VEC_ID:
    return visit_vec_id_expr(static_cast<vec_id_expr_node*>(expr),state);
  case VEC_MAKESTRUCT:
    return visit_make_struct(static_cast<make_struct_node*>(expr),state);
  case VEC_FN:
    return visit_fnid_expr(static_cast<fnid_expr_node*>(expr), state);
  case VEC_SCALE:
    return visit_domainfn(static_cast<vec_domainfn_node*>(expr), state);
  case VEC_DEFN:
    return visit_vector_defn(static_cast<vector_defn_node*>(expr), state);
  case VEC_FORSTMT:
  case VEC_DOSTMT:
  case VEC_STMT:
    return visit_stmt(static_cast<stmt_node*>(expr),state);
  case VEC_SCALAR: 
    return visit_value_expr(static_cast<expr_node*>(expr),state);
  default:
    assert(false && "Unhandled vector_expr during traversal");
    return NULL;
  }
}

template<typename State>
stmt_node* ASTVisitor<State>::visit_stmt(stmt_node* curr_stmt, State state)
{
  switch( curr_stmt->get_type() ){
  case VEC_STMT: {
    return visit_single_stmt(curr_stmt,state);
  }
  case VEC_FORSTMT:
    return visit_for_stmt(static_cast<for_stmt_node*>(curr_stmt),state);
  case VEC_DOSTMT:
    return visit_do_stmt(static_cast<do_stmt_node*>(curr_stmt),state);
  default:
    assert(false && "Unhandled stmt during traversal");
    return NULL;
  }
}

template<typename State>
stmt_node* ASTVisitor<State>::visit_single_stmt(stmt_node* curr_stmt, State state)
{
  vector_expr_node* new_rhs = visit_vector_expr(curr_stmt->rhs,state);
  if( new_rhs ){
    delete curr_stmt->rhs;
    curr_stmt->rhs = new_rhs;
    //new_rhs->add_usage(curr_stmt);
  }
  return NULL;
}

template<typename State>
stmt_node* ASTVisitor<State>::visit_for_stmt(for_stmt_node* curr_stmt, State state)
{
  for( std::deque<stmt_node*>::iterator it = curr_stmt->body->stmt_list.begin() ; it != curr_stmt->body->stmt_list.end() ; it++ ){
    stmt_node* new_stmt = visit_stmt(*it,state);
    if( new_stmt ){
      assert(("[ME] : Error! : During traversal replacement of a stmt is not a stmt\n",new_stmt->get_type() == VEC_STMT || new_stmt->get_type() == VEC_FORSTMT || new_stmt->get_type() == VEC_DOSTMT));
      curr_fn_symbols->remove_local_symbol(*it);
      delete (*it);
      (*it) = static_cast<stmt_node*>(new_stmt);
      curr_fn_symbols->add_local_symbol(new_stmt->get_name(),new_stmt);
    }
  }
  return NULL;
}

template<typename State>
stmt_node* ASTVisitor<State>::visit_do_stmt(do_stmt_node* curr_stmt, State state)
{
  for( std::deque<stmt_node*>::iterator it = curr_stmt->body->stmt_list.begin() ; it != curr_stmt->body->stmt_list.end() ; it++ ){
    stmt_node* new_stmt = visit_stmt(*it,state);
    if( new_stmt ){
      assert(("[ME] : Error! : During traversal replacement of a stmt is not a stmt\n",new_stmt->get_type() == VEC_STMT || new_stmt->get_type() == VEC_FORSTMT || new_stmt->get_type() == VEC_DOSTMT));
      curr_fn_symbols->remove_local_symbol(*it);
      delete (*it);
      (*it) = static_cast<stmt_node*>(new_stmt);
      curr_fn_symbols->add_local_symbol(new_stmt->get_name(),new_stmt);
    }
  }
  vector_expr_node* new_condn = visit_vector_expr(curr_stmt->escape_condn,state);
  if( new_condn ){
    delete curr_stmt->escape_condn;
    curr_stmt->escape_condn = new_condn;
  }
  return NULL;
}


template<typename State>
vector_expr_node* ASTVisitor<State>::visit_compose_expr(compose_expr_node* expr, State state)
{
  int num = 0;
  for( std::deque<std::pair<domain_desc_node*,vector_expr_node*> >::iterator it = expr->output_domains.begin() ; it != expr->output_domains.end() ; it++ , num++ ){
    vector_expr_node* new_expr = visit_vector_expr(it->second,state);
    if( new_expr){
      delete it->second;
      it->second = new_expr;
      //new_expr->add_usage(expr);
    }
  }
  return NULL;
}


template<typename State>
vector_expr_node* ASTVisitor<State>::visit_make_struct(make_struct_node* expr,State state)
{
  int num = 0;
  for( std::deque<vector_expr_node*>::iterator it = expr->field_inputs.begin() ; it != expr->field_inputs.end() ; it++, num++ ){
    vector_expr_node* new_expr = visit_vector_expr(*it,state);
    if( new_expr ){
      delete (*it);
      (*it) = new_expr;
      //new_expr->add_usage(expr);
    }
  }
  return NULL;
}


template<typename State>
vector_expr_node* ASTVisitor<State>::visit_fnid_expr(fnid_expr_node* expr, State state)
{
  int num = 0;
  for( std::deque<arg_info>::iterator it = expr->args.begin() ; it != expr->args.end() ; it++, num++ ){
    vector_expr_node* new_expr =visit_vector_expr(it->arg_expr,state);
    if( new_expr ){
      delete it->arg_expr;
      it->arg_expr = new_expr;
      //new_expr->add_usage(expr);
    }
  }
  return NULL;
}

template<typename State>
vector_expr_node* ASTVisitor<State>::visit_domainfn(vec_domainfn_node* expr, State state)
{
  vector_expr_node* new_expr = visit_vector_expr(expr->base_vec_expr,state);
  if( new_expr ){
    delete expr->base_vec_expr;
    expr->base_vec_expr = new_expr;
    //new_expr->add_usage(expr);
  }
  return NULL;
}


#endif


