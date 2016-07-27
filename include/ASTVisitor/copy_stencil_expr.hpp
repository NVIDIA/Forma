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
#ifndef __COPY_STENCIL_EXPR_HPP__
#define __COPY_STENCIL_EXPR_HPP__

class CopyStencilExpr{

protected:

  const local_symbols* fn_args;
  
  virtual unary_neg_expr_node* copy(const unary_neg_expr_node* curr_expr){
    return new unary_neg_expr_node(copy(curr_expr->get_base_expr()));
  }

  virtual math_fn_expr_node* copy(const math_fn_expr_node* curr_expr){
    const std::deque<expr_node*>& curr_args = curr_expr->get_args();
    math_fn_expr_node* ret_expr = new math_fn_expr_node();
    for( int i = 0 ; i < (int)curr_args.size() ; i++ ){
      ret_expr->add_arg(copy(curr_args[i]));
    }
    ret_expr->set_name(curr_expr->get_name().c_str());
    return ret_expr;
  }

  virtual expr_node* copy_value(const expr_node* curr_value_expr){
    switch(curr_value_expr->get_data_type().type){
    case T_FLOAT:
      return new value_node<float>(static_cast<const value_node<float>*>(curr_value_expr)->get_value());
    case T_DOUBLE:
      return new value_node<double>(static_cast<const value_node<double>*>(curr_value_expr)->get_value());
    case T_INT:
      return new value_node<int>(static_cast<const value_node<int>*>(curr_value_expr)->get_value());
    case T_INT8:
      return new value_node<unsigned char>(static_cast<const value_node<unsigned char>*>(curr_value_expr)->get_value());
    case T_INT16:
      return new value_node<short>(static_cast<const value_node<short>*>(curr_value_expr)->get_value());
    default:
      assert((0) && ("[ME]: Error! Copy of unknown data type"));
      return NULL;
    }    
  }

  virtual id_expr_node* copy(const id_expr_node* curr_id){
    return new id_expr_node(curr_id);
  }

  virtual ternary_expr_node* copy(const ternary_expr_node* curr_expr){
    return new ternary_expr_node(copy(curr_expr->get_bool_expr()),
				 copy(curr_expr->get_true_expr()),
				 copy(curr_expr->get_false_expr()));
  }


  virtual expr_op_node* copy(const expr_op_node* curr_expr){
    return new expr_op_node(copy(curr_expr->get_lhs_expr()),
			    copy(curr_expr->get_rhs_expr()),
			    curr_expr->get_op());
  }


  virtual pt_struct_node* copy(const pt_struct_node* curr_expr){
    pt_struct_node* new_struct_expr = NULL;
    const std::deque<expr_node*>& field_exprs = curr_expr->get_field_exprs();
    for( int i = 0 ; i < (int)field_exprs.size() ; i++ ){
      if( i == 0 ){
	new_struct_expr = new pt_struct_node(copy(field_exprs[i]));
      }
      else{
	new_struct_expr->add_field(copy(field_exprs[i]));
      }
    }
    new_struct_expr->find_struct_definition(curr_expr->get_struct_name().c_str());
    return new_struct_expr;
  }

  virtual stencil_op_node* copy(const stencil_op_node* curr_expr){
    return new stencil_op_node(curr_expr->get_name().c_str(),copy(curr_expr->get_scale_fn()),fn_args);
  }

  virtual array_access_node* copy( const array_access_node* curr_expr){
    std::deque<expr_node*>::const_iterator curr_index_expr = curr_expr->get_index_exprs().begin();
    array_access_node* new_access = new array_access_node(copy(*curr_index_expr));
    curr_index_expr++;
    for( ; curr_index_expr != curr_expr->get_index_exprs().end() ; curr_index_expr++ )
      new_access->add_index(copy(*curr_index_expr));
    new_access->set_name(curr_expr->get_name().c_str(),fn_args);
    return new_access;
  }

  
public:
  
  CopyStencilExpr(const local_symbols* curr_fn_args) :
    fn_args(curr_fn_args) 
  { }

  virtual ~CopyStencilExpr() { }
  
  virtual expr_node* copy(const expr_node* curr_expr){
    expr_node* ret_expr;
    switch(curr_expr->get_s_type()){
    case S_VALUE:
      ret_expr = copy_value(curr_expr);
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_UNARYNEG:
      ret_expr = copy(static_cast<const unary_neg_expr_node*>(curr_expr));
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_ID:
      ret_expr = copy(static_cast<const id_expr_node*>(curr_expr));
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_MATHFN:
      ret_expr = copy(static_cast<const math_fn_expr_node*>(curr_expr));
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_TERNARY:
      ret_expr = copy(static_cast<const ternary_expr_node*>(curr_expr));
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_BINARYOP:
      ret_expr = copy(static_cast<const expr_op_node*>(curr_expr));
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_STENCILOP:
      ret_expr = copy(static_cast<const stencil_op_node*>(curr_expr));
      if( curr_expr->get_base_type().type == T_STRUCT && curr_expr->get_access_field() != -1) 
	ret_expr->add_access_field(curr_expr->get_field_name().c_str());
      ret_expr->cast_to_type(curr_expr->get_data_type().type);			   
      break;
    case S_STRUCT:
      ret_expr = copy(static_cast<const pt_struct_node*>(curr_expr));
      break;
    case S_ARRAYACCESS:
      ret_expr = copy(static_cast<const array_access_node*>(curr_expr));
      break;
    default:
      assert(0);
      ret_expr = NULL;
    }
    return ret_expr;
  }

  virtual domainfn_node* copy(const domainfn_node* curr_scale_fn){
    domainfn_node* new_domainfn = new domainfn_node();
    const std::deque<scale_coeffs>& scale_fn = curr_scale_fn->scale_fn;
    for( int i = 0 ; i < (int)scale_fn.size() ; i++ ){
      new_domainfn->add_scale_coeffs(scale_fn[i].offset,scale_fn[i].scale);
    }
    return new_domainfn;
  }
  
};

#endif
