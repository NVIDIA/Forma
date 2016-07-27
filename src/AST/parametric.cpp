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
#include "AST/parametric.hpp"
#include <cassert>

using namespace std;

///-----Addition operations Start--------------////////

parametric_exp* int_expr::add(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_INT ){
    value += static_cast<int_expr*>(rhs)->value;
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_ADD);
    return new_exp;
  }
}

parametric_exp* int_expr::add(int val){
  value += val;
  return this;
}


parametric_exp* param_expr::add(parametric_exp* rhs, bool no_copy)
{
  if( is_equal(this,rhs) ){
    if( no_copy ){
      delete rhs;
    }
    return new binary_expr(this,new int_expr(2),P_MULTIPLY);
  }
  else if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 0 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_ADD);
  }
}

parametric_exp* param_expr::add(int val){
  if( val == 0 ){
    return this;
  }
  else{
    return new binary_expr(this,new int_expr(val),P_ADD);
  }
}

parametric_exp* binary_expr::add(parametric_exp* rhs, bool no_copy)
{
  parametric_exp* return_exp = NULL;
  if( rhs->type == P_INT ){
    return_exp = add(static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
  }
  if( return_exp == NULL ){
    return_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_ADD);
  }
  return return_exp;
}


parametric_exp* binary_expr::add(int val){
  parametric_exp* return_exp = NULL;
  if( val == 0 ){
    return_exp = this;
  }
  else if( rhs_expr->type == P_INT ){
    if( type == P_ADD ){
      rhs_expr->add(val);
      return_exp = this;
    }
    else if( type == P_SUBTRACT ){
      rhs_expr->subtract(val);
      return_exp = this;
    }
  }
  else if( lhs_expr->type == P_INT) {
    if( type == P_ADD || type == P_SUBTRACT ){
      lhs_expr->add(val);
      return_exp = this;
    }
  }
  if( return_exp == NULL )
    return_exp = new binary_expr(this,new int_expr(val), P_ADD);
  return return_exp;
}

///-----Addition operations End--------------////////


///-----Subtraction operations Start--------------////////

parametric_exp* int_expr::subtract(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_INT ){
    value -= static_cast<int_expr*>(rhs)->value;
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_SUBTRACT);
    return new_exp;
  }
}

parametric_exp* int_expr::subtract(int val){
  value -= val;
  return this;
}


parametric_exp* param_expr::subtract(parametric_exp* rhs, bool no_copy)
{
  if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 0 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_SUBTRACT);
  }
}

parametric_exp* param_expr::subtract(int val){
  if( val == 0 )
    return this;
  else
    return new binary_expr(this,new int_expr(val),P_SUBTRACT);
}

parametric_exp* binary_expr::subtract(parametric_exp* rhs, bool no_copy)
{
  parametric_exp* return_exp = NULL;
  if( rhs->type == P_INT ){
    return_exp = subtract(static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
  }
  if( return_exp == NULL ){
    return_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_SUBTRACT);
  }
  return return_exp;
}


parametric_exp* binary_expr::subtract(int val){
  parametric_exp* return_exp = NULL;
  if( val == 0 ){
    return_exp = this;
  }
  else if( rhs_expr->type == P_INT ){
    if( type == P_ADD ){
      rhs_expr->subtract(val);
      return_exp = this;
    }
    else if( type == P_SUBTRACT ){
      rhs_expr->add(val);
      return_exp = this;
    }
  }
  else if( lhs_expr->type == P_INT) {
    if( type == P_ADD || type == P_SUBTRACT ){
      lhs_expr->subtract(val);
      return_exp = this;
    }
  }
  if( return_exp == NULL )
    return_exp = new binary_expr(this,new int_expr(val), P_SUBTRACT);
  return return_exp;
}

///-----Subtraction operations End--------------////////




///-----Multiply operations Start--------------////////


parametric_exp* int_expr::multiply(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_INT ){
    value *= static_cast<int_expr*>(rhs)->value;
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_MULTIPLY);
    return new_exp;
  }
}

parametric_exp* int_expr::multiply(int val){
  value *= val;
  return this;
}


parametric_exp* param_expr::multiply(parametric_exp* rhs, bool no_copy)
{
  if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 1 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_MULTIPLY);
  }
}

parametric_exp* param_expr::multiply(int val){
  if( val == 1 ){
    return this;
  }
  else{
    return new binary_expr(this,new int_expr(val),P_MULTIPLY);
  }
}

parametric_exp* binary_expr::multiply(parametric_exp* rhs, bool no_copy)
{
  parametric_exp* return_exp = NULL;
  if( rhs->type == P_INT ){
    return_exp = multiply(static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
  }
  if( return_exp == NULL ){
    return_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_MULTIPLY);
  }
  return return_exp;
}


parametric_exp* binary_expr::multiply(int val){
  parametric_exp* return_exp = NULL;
  if( val == 1 ){
    return_exp = this;
  }
  else if( rhs_expr->type == P_INT ){
    if( type == P_MULTIPLY ){
      rhs_expr->multiply(val);
      return_exp = this;
    }
  }
  else if( lhs_expr->type == P_INT) {
    if( type == P_MULTIPLY ){
      lhs_expr->multiply(val);
      return_exp = this;
    }
  }
  if( return_exp == NULL )
    return_exp = new binary_expr(this,new int_expr(val), P_MULTIPLY);
  return return_exp;
}

///-----Multiply operations End--------------////////


////--------Divide Operations Start -------------- ////


parametric_exp* int_expr::divide(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_INT ){
    value /= static_cast<int_expr*>(rhs)->value;
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_DIVIDE);
    return new_exp;
  }
}

parametric_exp* int_expr::divide(int val){
  value /= val;
  return this;
}

parametric_exp* param_expr::divide(parametric_exp* rhs, bool no_copy)
{
  if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 1 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_DIVIDE);
  }
}

parametric_exp* param_expr::divide(int val){
  if( val == 1 ){
    return this;
  }
  else{
    return new binary_expr(this,new int_expr(val),P_DIVIDE);
  }
}

parametric_exp* binary_expr::divide(parametric_exp* rhs, bool no_copy)
{
  if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 1 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_DIVIDE);
  }
}


parametric_exp* binary_expr::divide(int val){
  if( val == 1 ){
    return this;
  }
  else{
    return new binary_expr(this,new int_expr(val), P_DIVIDE);
  }
}



////--------Divide Operations End -------------- ////


////--------CEIL Operations Start -------------- ////


parametric_exp* int_expr::ceil(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_CEIL ){
    value = CEIL(value,static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_CEIL);
    return new_exp;
  }
}

parametric_exp* int_expr::ceil(int val){
  value = CEIL(value,val);
  return this;
}

parametric_exp* param_expr::ceil(parametric_exp* rhs, bool no_copy)
{
  if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 1 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_CEIL);
  }
}

parametric_exp* param_expr::ceil(int val){
  if( val == 1 ){
    return this;
  }
  else{
    return new binary_expr(this,new int_expr(val),P_CEIL);
  }
}

parametric_exp* binary_expr::ceil(parametric_exp* rhs, bool no_copy)
{
  if( rhs->type == P_INT && static_cast<int_expr*>(rhs)->value == 1 ){
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    return new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_CEIL);
  }
}


parametric_exp* binary_expr::ceil(int val){
  if( val == 1 ){
    return this;
  }
  else{
    return new binary_expr(this,new int_expr(val), P_DIVIDE);
  }
}



////--------CEIL Operations End -------------- ////


///-----Max operations Start--------------////////

parametric_exp* int_expr::max(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_INT ){
    value = MAX(value,static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_MAX);
    return new_exp;
  }
}

parametric_exp* int_expr::max(int val){
  value = MAX(value,val);
  return this;
}


parametric_exp* param_expr::max(parametric_exp* rhs, bool no_copy)
{
  if( is_equal(this,rhs) ){
    if( no_copy )
      delete rhs;
    return this;
  }
  return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_MAX);
}

parametric_exp* param_expr::max(int val){
  return new binary_expr(this,new int_expr(val),P_MAX);
}

parametric_exp* binary_expr::max(parametric_exp* rhs, bool no_copy)
{
  parametric_exp* return_exp = NULL;
  if( is_equal(this,rhs ) ){
    return_exp = this;
    if( no_copy )
      delete rhs;
  }
  else if( rhs->type == P_INT ){
    return_exp = max(static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
  }

  if( return_exp == NULL ){
    return_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_MAX);
  }
  return return_exp;
}


parametric_exp* binary_expr::max(int val){
  parametric_exp* return_exp = NULL;
  if( rhs_expr->type == P_INT ){
    if( type == P_MAX ){
      rhs_expr->max(val);
      return_exp = this;
    }
    else if( type == P_MAX ){
      rhs_expr->max(val);
      return_exp = this;
    }
  }
  else if( lhs_expr->type == P_INT) {
    if( type == P_MAX  ){
      lhs_expr->max(val);
      return_exp = this;
    }
  }
  if( return_exp == NULL )
    return_exp = new binary_expr(this,new int_expr(val), P_MAX);
  return return_exp;
}

///-----MAX operations End--------------////////


///-----Min operations Start--------------////////

parametric_exp* int_expr::min(parametric_exp* rhs, bool no_copy){
  if( rhs->type == P_INT ){
    value = MIN(value,static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
    return this;
  }
  else{
    parametric_exp* new_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_MIN);
    return new_exp;
  }
}

parametric_exp* int_expr::min(int val){
  value = MIN(value,val);
  return this;
}


parametric_exp* param_expr::min(parametric_exp* rhs, bool no_copy)
{
  if( is_equal(this,rhs) ){
    if( no_copy )
      delete rhs;
    return this;
  }
  return new binary_expr(this,(no_copy ? rhs : copy(rhs) ),P_MIN);
}

parametric_exp* param_expr::min(int val){
  return new binary_expr(this,new int_expr(val),P_MIN);
}

parametric_exp* binary_expr::min(parametric_exp* rhs, bool no_copy)
{
  parametric_exp* return_exp = NULL;
  if( is_equal(this,rhs ) ){
    return_exp = this;
    if( no_copy )
      delete rhs;
  }
  else if( rhs->type == P_INT ){
    return_exp = min(static_cast<int_expr*>(rhs)->value);
    if( no_copy )
      delete rhs;
  }
  if( return_exp == NULL ){
    return_exp = new binary_expr(this,(no_copy ? rhs : copy(rhs)),P_MIN);
  }
  return return_exp;
}


parametric_exp* binary_expr::min(int val){
  parametric_exp* return_exp = NULL;
  if( rhs_expr->type == P_INT ){
    if( type == P_MIN ){
      rhs_expr->min(val);
      return_exp = this;
    }
    else if( type == P_MIN ){
      rhs_expr->min(val);
      return_exp = this;
    }
  }
  else if( lhs_expr->type == P_INT) {
    if( type == P_MIN  ){
      lhs_expr->min(val);
      return_exp = this;
    }
  }
  if( return_exp == NULL )
    return_exp = new binary_expr(this,new int_expr(val), P_MIN);
  return return_exp;
}

///-----MIN operations End--------------////////




parametric_exp* parametric_exp::copy(const parametric_exp* orig)
{
  parametric_exp* ret_copy;
  switch( orig->type ){
    
  case P_INT:
    ret_copy = new int_expr(static_cast<const int_expr*>(orig)->value);
    break;
  case P_PARAM:
    ret_copy = new param_expr(static_cast<const param_expr*>(orig)->param);
    break;
  case P_ADD:
  case P_SUBTRACT:
  case P_DIVIDE:
  case P_MULTIPLY:
  case P_CEIL:
  case P_MAX:
  case P_MIN: {
    const binary_expr* orig_binary = static_cast<const binary_expr*>(orig);
    ret_copy = new binary_expr(copy(orig_binary->lhs_expr),copy(orig_binary->rhs_expr),orig_binary->type);
    break;
  }
  case P_DEFAULT:
    ret_copy = new default_expr();
    break;
  default:
    assert((0) && ("[ME] : Unsupported type in parametric expressions\n"));
    ret_copy = NULL;
  }

  return ret_copy;
}

bool parametric_exp::is_equal(const parametric_exp* lhs, const parametric_exp* rhs)
{
  if( lhs->type == rhs->type ){
    switch(lhs->type) {
    case P_INT:
      return static_cast<const int_expr*>(lhs)->value == static_cast<const int_expr*>(rhs)->value;
    case P_PARAM:
      return ( static_cast<const param_expr*>(lhs)->param->param_id.compare(static_cast<const param_expr*>(rhs)->param->param_id) == 0 ? true : false );
    case P_ADD:
    case P_MULTIPLY:
    case P_MAX:
    case P_MIN: {
      const binary_expr* lhs_binary = static_cast<const binary_expr*>(lhs);
      const binary_expr* rhs_binary = static_cast<const binary_expr*>(rhs);
      return ( is_equal(lhs_binary->lhs_expr,rhs_binary->lhs_expr) && is_equal(lhs_binary->rhs_expr,rhs_binary->rhs_expr ) ) || ( is_equal(lhs_binary->lhs_expr,rhs_binary->rhs_expr) && is_equal(lhs_binary->rhs_expr,rhs_binary->lhs_expr ) );
    }
    case P_SUBTRACT:
    case P_DIVIDE:
    case P_CEIL: {
      const binary_expr* lhs_binary = static_cast<const binary_expr*>(lhs);
      const binary_expr* rhs_binary = static_cast<const binary_expr*>(rhs);
      return ( is_equal(lhs_binary->lhs_expr,rhs_binary->lhs_expr) && is_equal(lhs_binary->rhs_expr,rhs_binary->rhs_expr ) ) || ( is_equal(lhs_binary->lhs_expr,rhs_binary->rhs_expr) && is_equal(lhs_binary->rhs_expr,rhs_binary->lhs_expr ) );
    }
    default:
      assert(0);
      return false;
    }
  }
  else{
    return false; 
  }
}
