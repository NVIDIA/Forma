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
#include "AST/pretty_print.hpp"
#include <cmath>

using namespace std;

// extern int get_window_size();

PrettyPrinter* PrettyPrinter::pretty_printer = NULL;

int NUM_STRING_SIZE(int a)
{
  stringstream curr_string;
  curr_string << a;
  return curr_string.str().size();
}

int domainfn_node::compute_pretty_print_size()
{
  pretty_print_size = 3; ///@[ ]
  for( deque<scale_coeffs>::iterator it = scale_fn.begin() ; it != scale_fn.end() ; it++ ){
    pretty_print_size += 1 + NUM_STRING_SIZE(it->offset) + 1 + NUM_STRING_SIZE(it->scale) + 1 ; ///(a,b)
  }
  pretty_print_size += (scale_fn.size() == 0 ? 0 : scale_fn.size() -1 ); ///,
  return pretty_print_size;
}

void domainfn_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("@["),2);
  
  for( deque<scale_coeffs>::const_iterator it = scale_fn.begin() ; it != scale_fn.end() ; it++ ){
    // int new_string_size = 1 + NUM_STRING_SIZE(it->offset) + 1 + NUM_STRING_SIZE(it->scale) + 1 + 1; ///(a,b), or (a,b)]
    // //PrettyPrinter::instance()->print(PPTokenBreak(0,0),new_string_size);
    stringstream curr_string;
    curr_string << "(" << it->offset << "," << it->scale << ")" ;
    if( it != scale_fn.end() - 1 ){
      curr_string << ",";
    }
    string temp_string = curr_string.str();
    PrettyPrinter::instance()->print(PPTokenString(curr_string.str()),(int)curr_string.str().size());
  }
  PrettyPrinter::instance()->print(PPTokenString("]"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int int_expr::compute_pretty_print_size()
{
  stringstream curr_stream; 
  curr_stream << value;
  pretty_print_size = curr_stream.str().size();
  return pretty_print_size;
}

void int_expr::pretty_print() const
{
  stringstream curr_stream;
  curr_stream << value;
  PrettyPrinter::instance()->print(PPTokenString(curr_stream.str()),curr_stream.str().size());
}

int param_expr::compute_pretty_print_size()
{
  pretty_print_size = param->param_id.size();
  return pretty_print_size;
}

void param_expr::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenString(param->param_id),param->param_id.size());
}

int binary_expr::compute_pretty_print_size()
{
  pretty_print_size = 2 + const_cast<parametric_exp*>(lhs_expr)->compute_pretty_print_size() + const_cast<parametric_exp*>(rhs_expr)->compute_pretty_print_size(); /// '('<lhs>...<rhs>')'
  switch (type) {
  case P_ADD:
  case P_SUBTRACT:
  case P_MULTIPLY:
  case P_DIVIDE:
    pretty_print_size += 1; /// '+','-','*','/'
    break;
  case P_MAX:
  case P_MIN:
    pretty_print_size += 3 + 1; ///'MAX','MIN",','
    break;
  case P_CEIL:
    pretty_print_size += 4 + 1; ///'CEIL',','
   break;
  default:
    assert(0);
  }
  return pretty_print_size;
}


void binary_expr::pretty_print() const
{
  switch(type) {
  case P_ADD:
    PrettyPrinter::instance()->print(PPTokenString("("),1);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString("+"),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  case P_SUBTRACT:
    PrettyPrinter::instance()->print(PPTokenString("("),1);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString("-"),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  case P_MULTIPLY:
    PrettyPrinter::instance()->print(PPTokenString("("),1);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString("*"),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  case P_DIVIDE:
    PrettyPrinter::instance()->print(PPTokenString("("),1);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString("/"),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  case P_MAX:
    PrettyPrinter::instance()->print(PPTokenString("MAX("),4);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(","),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  case P_MIN:
    PrettyPrinter::instance()->print(PPTokenString("MIN("),4);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(","),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  case P_CEIL:
    PrettyPrinter::instance()->print(PPTokenString("CEIL("),4);
    lhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(","),1);
    rhs_expr->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(")"),1);
    break;
  default:
    assert(0);
  }
}


int domain_node::compute_pretty_print_size()
{
  pretty_print_size = 2; ///'[' ']'
  for( deque<range_coeffs>::iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
    pretty_print_size += it->lb->compute_pretty_print_size();
    pretty_print_size += 2; /// '..'
    pretty_print_size += it->ub->compute_pretty_print_size();
  }
  pretty_print_size += (range_list.size() == 0 ? 0 : range_list.size() -1 );
  return pretty_print_size;
}


void domain_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("["),1);
  for( deque<range_coeffs>::const_iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
    if( it != range_list.begin() ) {
      int new_string_size = 3;
      new_string_size += it->lb->get_pretty_print_size();
      new_string_size += it->ub->get_pretty_print_size();
      if( it == range_list.end() -1 )
	new_string_size += 1;
      PrettyPrinter::instance()->print(PPTokenBreak(0,0),new_string_size);
      PrettyPrinter::instance()->print(PPTokenString(","),1);
    }
    it->lb->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(".."),2);
    it->ub->pretty_print();
  }
  PrettyPrinter::instance()->print(PPTokenString("]"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int math_fn_expr_node::compute_pretty_print_size()
{
  pretty_print_size = fn_name.size() + 2;
  for( deque<expr_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += (fn_args.size() == 0 ? 0 : fn_args.size()-1);
  return pretty_print_size;
}

void math_fn_expr_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(fn_name),fn_name.size());
  PrettyPrinter::instance()->print(PPTokenString("("),1);
  deque<expr_node*>::const_iterator it = fn_args.begin();
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size()+1);
  (*it)->pretty_print();
  it++;
  for( ; it != fn_args.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenString(","),1);
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size()+1);
    (*it)->pretty_print();
  }
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int ternary_expr_node::compute_pretty_print_size()
{
  pretty_print_size = 8 + bool_expr->compute_pretty_print_size() + true_expr->compute_pretty_print_size() + false_expr->compute_pretty_print_size(); ///(<expr> ? <expr> : <expr>)
  return pretty_print_size;
}

void ternary_expr_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("("),1);
  bool_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("?"),1);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+true_expr->get_pretty_print_size());
  true_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString(":"),1);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+false_expr->get_pretty_print_size()+1);
  false_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int expr_op_node::compute_pretty_print_size()
{
  pretty_print_size = 1 + lhs_expr->compute_pretty_print_size() + 2 + rhs_expr->compute_pretty_print_size() + get_op_string_size(op) + 1; ///(<lhs> <op> <rhs>)
  return pretty_print_size;
}

void expr_op_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("("),1);
  lhs_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+get_op_string_size(op));
  PrettyPrinter::instance()->print(PPTokenString(get_op_string(op)),get_op_string_size(op));
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+rhs_expr->get_pretty_print_size());
  rhs_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int array_access_node::compute_pretty_print_size()
{
  pretty_print_size = param_name.size() + 1;
  for( deque<expr_node*>::iterator it = index_exprs.begin() ; it != index_exprs.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
    pretty_print_size += 1;
  }
  return pretty_print_size;
}

void array_access_node::pretty_print() const 
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(param_name),param_name.size());
  PrettyPrinter::instance()->print(PPTokenString("["),1);
  deque<expr_node*>::const_iterator it = index_exprs.begin();
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size()+1);
  (*it)->pretty_print();
  it++;
  for( ; it != index_exprs.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenString(","),1);
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size()+1);
    (*it)->pretty_print();
  }
  PrettyPrinter::instance()->print(PPTokenString("]"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int stencil_op_node::compute_pretty_print_size()
{
  pretty_print_size = param_name.size() + scale_fn->compute_pretty_print_size();
  if( field_num != -1 ){
    assert(base_elem_type.type == T_STRUCT && base_elem_type.struct_info != NULL );
    pretty_print_size += 1 + base_elem_type.struct_info->fields[field_num].field_name.size();
  }
  return pretty_print_size;
}

void stencil_op_node::pretty_print() const 
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(param_name),param_name.size());
  scale_fn->pretty_print();
  if( field_num != -1 ){
    int field_print_size = base_elem_type.struct_info->fields[field_num].field_name.size();
    PrettyPrinter::instance()->print(PPTokenString("."),1);
    PrettyPrinter::instance()->print(PPTokenString(base_elem_type.struct_info->fields[field_num].field_name),field_print_size);
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int pt_struct_node::compute_pretty_print_size()
{
  pretty_print_size = 6 + 1 + struct_info->name.size() + 1 + 1 ;
  for( deque<expr_node*>::iterator it = field_expr.begin() ; it != field_expr.end() ; it++ )
    pretty_print_size += (*it)->compute_pretty_print_size();
  pretty_print_size += struct_info->fields.size()-1;
  return pretty_print_size;
}

void pt_struct_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  stringstream new_stream ;
  new_stream << "struct " << struct_info->name << "(" ;
  PrettyPrinter::instance()->print(PPTokenString(new_stream.str()),new_stream.str().size());
  for( deque<expr_node*>::const_iterator it = field_expr.begin() ; it != field_expr.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+(*it)->get_pretty_print_size());
    (*it)->pretty_print();
    if( it != field_expr.end() - 1 )
      PrettyPrinter::instance()->print(PPTokenString(","),1);
  }
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int pt_stmt_node::compute_pretty_print_size()
{
  pretty_print_size = lhs_name.size() + 3 + rhs_expr->compute_pretty_print_size() + 1;
  return pretty_print_size;
}

void pt_stmt_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,true),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(lhs_name),(int)lhs_name.size());
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("="),1);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2+(int)rhs_expr->get_pretty_print_size());
  rhs_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(";"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int compose_expr_node::compute_pretty_print_size()
{
  pretty_print_size = 2; /// ( )
  for( deque<pair<domain_desc_node*,vector_expr_node*> >::iterator it = output_domains.begin() ; it != output_domains.end() ; it++ ){
    pretty_print_size += it->first->compute_pretty_print_size();
    pretty_print_size += 3; /// ' = '
    pretty_print_size += it->second->compute_pretty_print_size();
    pretty_print_size += 1; /// ';'
  }
  if( sub_domain )
    pretty_print_size += sub_domain->compute_pretty_print_size();
  if( field_num != -1 ){
    assert(elem_type.type ==T_STRUCT && elem_type.struct_info != NULL );
    pretty_print_size += 1 + elem_type.struct_info->fields[field_num].field_name.size();
  }
  return pretty_print_size;
}

void compose_expr_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("("),1);
  for( deque<pair<domain_desc_node*,vector_expr_node*> >::const_iterator it = output_domains.begin() ; it != output_domains.end() ; it++ ){
    int curr_print_size = it->first->get_pretty_print_size() + 3 + it->second->get_pretty_print_size() + 1; ///<domain> = <vectorexpr>;
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),curr_print_size);
    PrettyPrinter::instance()->print(PPTokenBegin(2,false),curr_print_size);
    it->first->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(" = "),3);
    it->second->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(";"),1);
    PrettyPrinter::instance()->print(PPTokenEnd(),0);
  }
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),1);
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  if( sub_domain )
    sub_domain->pretty_print();
  if( field_num != -1 ){
    int field_print_size = elem_type.struct_info->fields[field_num].field_name.size();
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),1+field_print_size);
    PrettyPrinter::instance()->print(PPTokenString("."),1);
    PrettyPrinter::instance()->print(PPTokenString(elem_type.struct_info->fields[field_num].field_name),field_print_size);
  }				     
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int vec_id_expr_node::compute_pretty_print_size()
{
  pretty_print_size = name.size();
  if( sub_domain )
    pretty_print_size += sub_domain->compute_pretty_print_size();
  if( access_iterator ){
    pretty_print_size += access_iterator->name.size() +2; ///'<'<ID>-<INT>'>' 
    if( offset != DEFAULT_RANGE ){
      pretty_print_size += 1 + NUM_STRING_SIZE(offset);
    }
  }
  else if( offset != DEFAULT_RANGE)
    pretty_print_size += NUM_STRING_SIZE(offset) + 2; ///'<'<INT'>'
  if( field_num != -1 ){
    assert(base_elem_type.type ==T_STRUCT && base_elem_type.struct_info != NULL );
    pretty_print_size += 1 + base_elem_type.struct_info->fields[field_num].field_name.size();
  }
  return pretty_print_size;
}

void vec_id_expr_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(name),(int)name.size());
  if( sub_domain )
    sub_domain->pretty_print();
  if( access_iterator != NULL ){
    PrettyPrinter::instance()->print(PPTokenString("<"),1);
    PrettyPrinter::instance()->print(PPTokenString(access_iterator->name),access_iterator->name.size());
    if( offset != DEFAULT_RANGE ){
      if( access_iterator->is_positive ){
	PrettyPrinter::instance()->print(PPTokenString("-"),1);
      }
      else{ 
	PrettyPrinter::instance()->print(PPTokenString("+"),1);
      }
      stringstream curr_num;
      curr_num << offset;
      PrettyPrinter::instance()->print(PPTokenString(curr_num.str()),curr_num.str().size());
    }
    PrettyPrinter::instance()->print(PPTokenString(">"),1);
  }
  else if( offset != DEFAULT_RANGE ){
    PrettyPrinter::instance()->print(PPTokenString("<"),1);
    stringstream curr_num;
    curr_num << offset;
    PrettyPrinter::instance()->print(PPTokenString(curr_num.str()),curr_num.str().size());
    PrettyPrinter::instance()->print(PPTokenString(">"),1);
  }
  if( field_num != -1 ){
    int field_print_size = base_elem_type.struct_info->fields[field_num].field_name.size();
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),1+field_print_size);
    PrettyPrinter::instance()->print(PPTokenString("."),1);
    PrettyPrinter::instance()->print(PPTokenString(base_elem_type.struct_info->fields[field_num].field_name),field_print_size);
  }				     
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int make_struct_node::compute_pretty_print_size() 
{
  pretty_print_size = 7 + elem_type.struct_info->name.size() + 2; ///"struct <id-name>( )"
  for( deque<vector_expr_node*>::iterator it = field_inputs.begin() ; it != field_inputs.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += field_inputs.size() -1; ///','
  if( field_num != -1 ){
    assert(base_elem_type.type ==T_STRUCT && base_elem_type.struct_info != NULL );
    pretty_print_size += 1 + base_elem_type.struct_info->fields[field_num].field_name.size();
  }
  return pretty_print_size;
}


void make_struct_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("struct "),7);
  PrettyPrinter::instance()->print(PPTokenString(elem_type.struct_info->name),elem_type.struct_info->name.size());
  PrettyPrinter::instance()->print(PPTokenString("("),1);
  for( deque<vector_expr_node*>::const_iterator it = field_inputs.begin() ; it != field_inputs.end() ; it++ ){
    if( it != field_inputs.begin() ){
      PrettyPrinter::instance()->print(PPTokenString(","),1);
      PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size()+1);
    }
    (*it)->pretty_print();
  }
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  if( field_num != -1 ){
    int field_print_size = base_elem_type.struct_info->fields[field_num].field_name.size();
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),1+field_print_size);
    PrettyPrinter::instance()->print(PPTokenString("."),1);
    PrettyPrinter::instance()->print(PPTokenString(base_elem_type.struct_info->fields[field_num].field_name),field_print_size);
  }				     
    PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int bdy_info::compute_size() const
{
  int ret_val=0;
  switch(type){
  case B_NONE:
    break;
  case B_CONSTANT:
    ret_val += 1 + 8+1+NUM_STRING_SIZE(value)+1;
    break;
  case B_CLAMPED:
    ret_val+= 1 + 7;
    break;
  case B_WRAP:
    ret_val += 1 + 4;
    break;
  case B_MIRROR:
    ret_val += 1 + 6;
    break;
  case B_EXTEND:
    ret_val += 1 + 6;
  default:
    assert(0);
  }
  return ret_val;
}

void bdy_info::pretty_print() const 
{
  switch(type){
  case B_NONE:
    break;
  case B_CONSTANT:{
    stringstream bdy_string ;
    bdy_string << ":constant(" << value << ")" ;
    PrettyPrinter::instance()->print(PPTokenString(bdy_string.str()),bdy_string.str().size());
    break;
  }
  case B_CLAMPED:
    PrettyPrinter::instance()->print(PPTokenString(":clamped"),8);
    break;
  case B_WRAP:
    PrettyPrinter::instance()->print(PPTokenString(":wrap"),5);
    break;
  case B_MIRROR:
    PrettyPrinter::instance()->print(PPTokenString(":mirror"),7);
    break;
  case B_EXTEND:
    PrettyPrinter::instance()->print(PPTokenString(":extend"),7);
    break;
  default:
    assert(0);
  }
}

int fnid_expr_node::compute_pretty_print_size()
{
  pretty_print_size = id_name.size() + 2; ///<ID>( )
  for( deque<arg_info>::iterator it = args.begin() ; it != args.end() ; it++ ){
    pretty_print_size += it->arg_expr->compute_pretty_print_size();
    if( it->arg_expr->get_type() != VEC_SCALAR )
      pretty_print_size += it->bdy_condn->compute_size();
  }
  pretty_print_size += ( args.size() == 0 ? 0 : args.size()-1); ///,
  if( sub_domain )
    pretty_print_size += sub_domain->compute_pretty_print_size();
  if( field_num != -1 ){
    assert(base_elem_type.type ==T_STRUCT && base_elem_type.struct_info != NULL );
    pretty_print_size += 1 + base_elem_type.struct_info->fields[field_num].field_name.size();
  }
  return pretty_print_size;
}

void fnid_expr_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(id_name),(int)id_name.size());
  PrettyPrinter::instance()->print(PPTokenString("("),1);
  if( args.size() > 0 ){
    deque<arg_info>::const_iterator it = args.begin();
    it->arg_expr->pretty_print();
    if( it->arg_expr->get_type() != VEC_SCALAR )
      it->bdy_condn->pretty_print();
    it++;
    for( ; it != args.end() ; it++ ){
      PrettyPrinter::instance()->print(PPTokenString(","),1);
      it->arg_expr->pretty_print();
      if( it->arg_expr->get_type() != VEC_SCALAR )
	it->bdy_condn->pretty_print();
    }
  }
  PrettyPrinter::instance()->print(PPTokenString(")"),1);
  if( sub_domain )
    sub_domain->pretty_print();
  if( field_num != -1 ){
    int field_print_size = base_elem_type.struct_info->fields[field_num].field_name.size();
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),1+field_print_size);
    PrettyPrinter::instance()->print(PPTokenString("."),1);
    PrettyPrinter::instance()->print(PPTokenString(base_elem_type.struct_info->fields[field_num].field_name),field_print_size);
  }				     
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int vec_domainfn_node::compute_pretty_print_size()
{
  pretty_print_size = base_vec_expr->compute_pretty_print_size() + scale_fn->compute_pretty_print_size();
  if( sub_domain )
    pretty_print_size += sub_domain->compute_pretty_print_size();
  if( field_num != -1 ){
    assert(base_elem_type.type == T_STRUCT && base_elem_type.struct_info != NULL );
    pretty_print_size += 1 + base_elem_type.struct_info->fields[field_num].field_name.size();
  }
  return pretty_print_size;
}

void vec_domainfn_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  base_vec_expr->pretty_print();
  scale_fn->pretty_print();
  if( sub_domain )
    sub_domain->pretty_print();
  if( field_num != -1 ){
    int field_print_size = base_elem_type.struct_info->fields[field_num].field_name.size();
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),1+field_print_size);
    PrettyPrinter::instance()->print(PPTokenString("."),1);
    PrettyPrinter::instance()->print(PPTokenString(base_elem_type.struct_info->fields[field_num].field_name),field_print_size);
  }				     
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int parameter_defn::compute_pretty_print_size()
{
  pretty_print_size = 9 + 1 + param_id.size() + 1; ///'parameter' <ID> ';'
  if( value != DEFAULT_RANGE ){
    stringstream curr_stream;
    curr_stream << value;
    pretty_print_size += 2 + curr_stream.str().size();
  }
  return pretty_print_size;
}

void parameter_defn::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  stringstream curr_stream;
  curr_stream << "parameter " << param_id ;
  if( value != DEFAULT_RANGE ){
    curr_stream << "= " << value;
  }
  curr_stream << ";";
  PrettyPrinter::instance()->print(PPTokenString(curr_stream.str()),curr_stream.str().size());
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int vector_defn_node::compute_pretty_print_size()
{
  pretty_print_size = 8 + NUM_STRING_SIZE(ndims); ///"vector#<int> "
  switch(elem_type.type){
  case T_FLOAT:
    pretty_print_size += 6; ///"float "
    break;
  case T_DOUBLE:
    pretty_print_size += 7; ///"double "
    break;
  case T_INT:
    pretty_print_size += 4; ///"int "
    break;
  case T_INT8:
    pretty_print_size += 5; ///"int8 "
    break;
  case T_INT16:
    pretty_print_size += 6; ///"int16 "
    break;
  case T_STRUCT:
    assert(elem_type.struct_info);
    pretty_print_size += elem_type.struct_info->name.size() + 1; ///"<name> "
  default:
    ;
  }
  pretty_print_size += name.size();
  if( expr_domain )
    pretty_print_size += expr_domain->compute_pretty_print_size();
  return pretty_print_size;
}


void vector_defn_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("vector#"),7);
  stringstream dim_string;
  dim_string << ndims;
  PrettyPrinter::instance()->print(PPTokenString(dim_string.str()),dim_string.str().size());
  switch(elem_type.type){
  case T_FLOAT:
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),6);
    PrettyPrinter::instance()->print(PPTokenString("float"),5); 
    break;
  case T_DOUBLE:
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),7);
    PrettyPrinter::instance()->print(PPTokenString("double"),6); 
    break;
  case T_INT:
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),4);
    PrettyPrinter::instance()->print(PPTokenString("int"),3); 
    break;
  case T_INT8:
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),5);
    PrettyPrinter::instance()->print(PPTokenString("int8"),4); 
    break;
  case T_INT16:
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),6);
    PrettyPrinter::instance()->print(PPTokenString("int16"),5); 
    break;
  case T_STRUCT:
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),elem_type.struct_info->name.size()+1);
    PrettyPrinter::instance()->print(PPTokenString(elem_type.struct_info->name),elem_type.struct_info->name.size());
    break;
  default:
    ;
  }
  if( expr_domain ){
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+name.size()+expr_domain->get_pretty_print_size());
    PrettyPrinter::instance()->print(PPTokenString(name),(int)name.size());
    expr_domain->pretty_print();
  }
  else{
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+(int)name.size());
    PrettyPrinter::instance()->print(PPTokenString(name),(int)name.size());
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int for_stmt_node::compute_pretty_print_size()
{
  pretty_print_size = 4 + iter_info->name.size() + 1 + 1 + 1 + iter_info->iter_domain->compute_pretty_print_size() + 1 + 6; ///for <ID> = <Range> <stmtseq> endfor
  if( !iter_info->is_positive )
    pretty_print_size += 3;
  for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ )
    pretty_print_size += (*it)->compute_pretty_print_size();
  return pretty_print_size;
}

void for_stmt_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,true),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("for"),3);
  int header_size = iter_info->name.size() + 1 + 1 + 1 + iter_info->iter_domain->get_pretty_print_size();
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+header_size);

  PrettyPrinter::instance()->print(PPTokenBegin(2,false),header_size);
  PrettyPrinter::instance()->print(PPTokenString(iter_info->name),iter_info->name.size());
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("="),1);
  if( iter_info->is_positive ){
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),iter_info->iter_domain->get_pretty_print_size()+1);
    iter_info->iter_domain->pretty_print();
  }
  else{
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),iter_info->iter_domain->get_pretty_print_size()+1+3);
    iter_info->iter_domain->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(",-1"),3);
  }

  int body_size = 0;
  for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ )
    body_size += (*it)->get_pretty_print_size();
  
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+body_size);
  
  PrettyPrinter::instance()->print(PPTokenBegin(2,true),body_size);
  for(  std::deque<stmt_node*>::const_iterator it = body->stmt_list.begin(); it != body->stmt_list.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size());
    (*it)->pretty_print();
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  PrettyPrinter::instance()->print(PPTokenBreak(1,0),7);
  PrettyPrinter::instance()->print(PPTokenString("endfor"),6);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int do_stmt_node::compute_pretty_print_size()
{
  pretty_print_size = 2 + 1 + iter_info->name.size() + 1 + 1 + 1 + iter_info->iter_domain->compute_pretty_print_size() +  6 + escape_condn->compute_pretty_print_size() + 2;
  if( !iter_info->is_positive )
    pretty_print_size += 3;
  for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ )
    pretty_print_size += (*it)->compute_pretty_print_size();
  return pretty_print_size;
}

void do_stmt_node::pretty_print () const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,true),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString("do"),2);
  int header_size = iter_info->name.size() + 1 + 1 + 1 + iter_info->iter_domain->get_pretty_print_size();
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+header_size);

  PrettyPrinter::instance()->print(PPTokenBegin(2,false),header_size);
  PrettyPrinter::instance()->print(PPTokenString(iter_info->name),iter_info->name.size());
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("="),1);
  if( iter_info->is_positive ){
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),iter_info->iter_domain->get_pretty_print_size()+1);
    iter_info->iter_domain->pretty_print();
  }
  else{
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),iter_info->iter_domain->get_pretty_print_size()+1+3);
    iter_info->iter_domain->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(",-1"),3);
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
  
  int body_size = 0;
  for( std::deque<stmt_node*>::iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ )
    body_size += (*it)->get_pretty_print_size();
  
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),1+body_size);
  
  PrettyPrinter::instance()->print(PPTokenBegin(2,true),body_size);
  for(  std::deque<stmt_node*>::const_iterator it = body->stmt_list.begin(); it != body->stmt_list.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size());
    (*it)->pretty_print();
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
  
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),6);
  PrettyPrinter::instance()->print(PPTokenString("while("),6);
  
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),escape_condn->get_pretty_print_size());
  escape_condn->pretty_print();
  
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),2);
  PrettyPrinter::instance()->print(PPTokenString(");"),2);

  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int stmt_node::compute_pretty_print_size()
{
  pretty_print_size = lhs.size() + 4 + rhs->compute_pretty_print_size() + (sub_domain != NULL ? sub_domain->compute_pretty_print_size() : 0 ) + ( scale_domain != NULL ? scale_domain->compute_pretty_print_size() : 0); /// <id><domain> = <vectorexpr>;
  if( access_iterator ){
    pretty_print_size += access_iterator->name.size() + 3 + NUM_STRING_SIZE(offset); ///'<'<ID>-<INT>'>' 
  }
  else if( offset != DEFAULT_RANGE)
    pretty_print_size += NUM_STRING_SIZE(offset) + 2; ///'<'<INT'>'
  return pretty_print_size;
}


void stmt_node::pretty_print() const
{
  PrettyPrinter::instance()->print(PPTokenBegin(2,true),pretty_print_size);
  PrettyPrinter::instance()->print(PPTokenString(lhs),(int)lhs.size());
  if( access_iterator ){
    PrettyPrinter::instance()->print(PPTokenString("<"),1);
    PrettyPrinter::instance()->print(PPTokenString(access_iterator->name),access_iterator->name.size());
    PrettyPrinter::instance()->print(PPTokenString("-"),1);
    stringstream curr_num;
    curr_num << offset;
    PrettyPrinter::instance()->print(PPTokenString(curr_num.str()),curr_num.str().size());
    PrettyPrinter::instance()->print(PPTokenString(">"),1);
  }
  else if( offset != DEFAULT_RANGE ){
    PrettyPrinter::instance()->print(PPTokenString("<"),1);
    stringstream curr_num;
    curr_num << offset;
    PrettyPrinter::instance()->print(PPTokenString(curr_num.str()),curr_num.str().size());
    PrettyPrinter::instance()->print(PPTokenString(">"),1);
  }
  if( sub_domain )
    sub_domain->pretty_print();
  else if( scale_domain )
    scale_domain->pretty_print();
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("="),1);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),(int)rhs->get_pretty_print_size()+2);
  rhs->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(";"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}

int stencilfn_defn_node::compute_pretty_print_size()
{
  pretty_print_size = 8 + fn_name.size() + 2 + 2 + 1; ///stencil <ID>(){ } 
  for( deque<vector_defn_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += ( fn_args.size() == 0 ? 0 : fn_args.size()-1); ///,
  for( deque<pt_stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += 7 + return_expr->compute_pretty_print_size() + 1; ///return <expr>;
  return pretty_print_size;
}


void stencilfn_defn_node::pretty_print() const
{
  int header_size = 8 + fn_name.size() + 2 + 1; ///vector <ID>(){
  for( deque<vector_defn_node*>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    header_size += (*it)->compute_pretty_print_size();
  }
  header_size += ( fn_args.size() == 0 ? 0 : fn_args.size()-1); ///,

  PrettyPrinter::instance()->print(PPTokenBegin(2,false),header_size);

  PrettyPrinter::instance()->print(PPTokenString("stencil"),7);
  if( fn_args.size() != 0 ){
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),fn_name.size()+1+1); /// ID(
    PrettyPrinter::instance()->print(PPTokenString(fn_name),fn_name.size());
    PrettyPrinter::instance()->print(PPTokenString("("),1);
    deque<vector_defn_node*>::const_iterator it = fn_args.begin();
    PrettyPrinter::instance()->print(PPTokenBreak(0,2),(*it)->get_pretty_print_size()+1);
    (*it)->pretty_print();
    it++;
    for( ; it != fn_args.end() ; it++ ){
      PrettyPrinter::instance()->print(PPTokenString(","),1);
      if( it == fn_args.end() - 1 )
  	PrettyPrinter::instance()->print(PPTokenBreak(0,2),(*it)->get_pretty_print_size()+2);
      else
  	PrettyPrinter::instance()->print(PPTokenBreak(0,2),(*it)->get_pretty_print_size()+1);
      (*it)->pretty_print();
    }
    PrettyPrinter::instance()->print(PPTokenString("){"),2);
  }
  else{
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),fn_name.size()+1+3); /// ID(){
    PrettyPrinter::instance()->print(PPTokenString(fn_name),fn_name.size());
    PrettyPrinter::instance()->print(PPTokenString("(){"),3);
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  int stmts_size = 0;
  for( deque<pt_stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    stmts_size += (*it)->get_pretty_print_size();
  stmts_size += 7 + return_expr->get_pretty_print_size() + 1;
  PrettyPrinter::instance()->print(PPTokenBreak(1,2),stmts_size+1);

  PrettyPrinter::instance()->print(PPTokenBegin(2,true),stmts_size);

  for( deque<pt_stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size());
    (*it)->pretty_print();
  }  
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),return_expr->get_pretty_print_size()+8);

  PrettyPrinter::instance()->print(PPTokenBegin(2,false),return_expr->get_pretty_print_size()+8);
  PrettyPrinter::instance()->print(PPTokenString("return"),6);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),return_expr->get_pretty_print_size()+2);
  return_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(";"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("}"),1);
}


int vectorfn_defn_node::compute_pretty_print_size()
{
  pretty_print_size = 7 + fn_name.size() + 2 + 2 + 1; ///vector <ID>(){ } 
  for( deque<vector_defn_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += ( fn_args.size() == 0 ? 0 : fn_args.size()-1); ///,
  for( deque<stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += 7 + return_expr->compute_pretty_print_size() + 1; ///return <expr>;
  return pretty_print_size;
}


void vectorfn_defn_node::pretty_print() const
{
  int header_size = 7 + fn_name.size() + 2 + 1; ///vector <ID>(){
  for( deque<vector_defn_node*>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    header_size += (*it)->compute_pretty_print_size();
  }
  header_size += ( fn_args.size() == 0 ? 0 : fn_args.size()-1); ///,

  PrettyPrinter::instance()->print(PPTokenBegin(2,false),header_size);

  PrettyPrinter::instance()->print(PPTokenString("vector"),6);
  if( fn_args.size() != 0 ){
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),fn_name.size()+1+1); /// ID(
    PrettyPrinter::instance()->print(PPTokenString(fn_name),fn_name.size());
    PrettyPrinter::instance()->print(PPTokenString("("),1);
    deque<vector_defn_node*>::const_iterator it = fn_args.begin();
    PrettyPrinter::instance()->print(PPTokenBreak(0,2),(*it)->get_pretty_print_size()+1);
    (*it)->pretty_print();
    it++;
    for( ; it != fn_args.end() ; it++ ){
      PrettyPrinter::instance()->print(PPTokenString(","),1);
      if( it == fn_args.end() - 1 )
	PrettyPrinter::instance()->print(PPTokenBreak(0,2),(*it)->get_pretty_print_size()+2);
      else
	PrettyPrinter::instance()->print(PPTokenBreak(0,2),(*it)->get_pretty_print_size()+1);
      (*it)->pretty_print();
    }
    PrettyPrinter::instance()->print(PPTokenString("){"),2);
  }
  else{
    PrettyPrinter::instance()->print(PPTokenBreak(1,0),fn_name.size()+1+3); /// ID(){
    PrettyPrinter::instance()->print(PPTokenString(fn_name),fn_name.size());
    PrettyPrinter::instance()->print(PPTokenString("(){"),3);
  }
  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  int stmts_size = 0;
  for( deque<stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    stmts_size += (*it)->get_pretty_print_size();
  stmts_size += 7 + return_expr->get_pretty_print_size() + 1;
  PrettyPrinter::instance()->print(PPTokenBreak(1,2),stmts_size+1);

  PrettyPrinter::instance()->print(PPTokenBegin(2,true),stmts_size);

  for( deque<stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size());
    (*it)->pretty_print();
  }  
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),return_expr->get_pretty_print_size()+8);

  PrettyPrinter::instance()->print(PPTokenBegin(2,false),return_expr->get_pretty_print_size()+8);
  PrettyPrinter::instance()->print(PPTokenString("return"),6);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),return_expr->get_pretty_print_size()+2);
  return_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(";"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  PrettyPrinter::instance()->print(PPTokenBreak(1,0),2);
  PrettyPrinter::instance()->print(PPTokenString("}"),1);
}


int vectorfn_defn_node::compute_pretty_print_program()
{
  pretty_print_size = 0;
  for( deque<vector_defn_node*>::iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size() + 1; ///<vectordefn>;
  }
  for( deque<pair<string,parameter_defn*> >::const_iterator it = global_params->begin() ; it != global_params->end() ; it++ ){
    pretty_print_size += it->second->compute_pretty_print_size();
  }
  for( deque<stmt_node*>::iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    pretty_print_size += (*it)->compute_pretty_print_size();
  }
  pretty_print_size += 7 + return_expr->compute_pretty_print_size() + 1; ///return <expr>;
  return pretty_print_size;
}


void vectorfn_defn_node::pretty_print_program() const
{
  for( deque<pair<string, parameter_defn*> >::const_iterator it = global_params->begin() ; it != global_params->end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),it->second->get_pretty_print_size());
    it->second->pretty_print();
  }
  for( deque<vector_defn_node*>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size()+1);
    (*it)->pretty_print();
    PrettyPrinter::instance()->print(PPTokenString(";"),1);
  }
  for( deque<stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),(*it)->get_pretty_print_size());
    (*it)->pretty_print();
  }
  int return_size = 7 + return_expr->get_pretty_print_size() + 1;
  PrettyPrinter::instance()->print(PPTokenBreak(0,0),return_size);
  
  PrettyPrinter::instance()->print(PPTokenBegin(2,false),return_size);
  PrettyPrinter::instance()->print(PPTokenString("return"),6);
  PrettyPrinter::instance()->print(PPTokenBreak(1,0),return_expr->get_pretty_print_size()+2);
  return_expr->pretty_print();
  PrettyPrinter::instance()->print(PPTokenString(";"),1);
  PrettyPrinter::instance()->print(PPTokenEnd(),0);
}


int fn_defs_table::compute_pretty_print_size()
{
  pretty_print_size = 0;
  for( deque<pair<string,fn_defn_node*> >::const_iterator it = fn_defs->symbols.begin() ; it != fn_defs->symbols.end() ; it++ ){
    pretty_print_size += it->second->compute_pretty_print_size();
  }
  return pretty_print_size;
}

void fn_defs_table::pretty_print() const
{
  for( deque<pair<string,fn_defn_node*> >::const_iterator it = fn_defs->symbols.begin() ; it != fn_defs->symbols.end() ; it++ ){
    PrettyPrinter::instance()->print(PPTokenBreak(0,0),it->second->get_pretty_print_size());
    it->second->pretty_print();
  }
}


void program_node::pretty_print() const
{
  // PrettyPrinter* my_printer =PrettyPrinter::instance(get_window_size());
  PrettyPrinter* my_printer =PrettyPrinter::instance(80);

  int pretty_print_size = fn_defs->compute_pretty_print_size() +  program_body->compute_pretty_print_program();

  PrettyPrinter::instance()->print(PPTokenBegin(0,true),pretty_print_size);
  fn_defs->pretty_print();
  program_body->pretty_print_program();
  PrettyPrinter::instance()->print(PPTokenEnd(),0);

  PrettyPrinter::instance()->print_string();
  
  delete my_printer;
}



PrettyPrinterBase::PrettyPrinterBase (int width)
{
  // set up variables and queues
  margin = space = width;
  arraysize = 2000;
  token = new PPToken*[arraysize];
  size = new int[arraysize];
}

PrettyPrinterBase::~PrettyPrinterBase ()
{
  // free memory
  delete[] token;
  delete[] size;
}

void PrettyPrinterBase::printNewLine (int amount)
{
  outstream << std::endl;
  while (amount-- > 0) outstream << ' ';
}

void PrettyPrinterBase::indent (int amount)
{
  while (amount-- > 0) outstream << ' ';
}


void PrettyPrinterBase::print (PPTokenBegin tBegin, const int l)
{
  if (l > space)
    printStack.push (PrintStackEntry (space - tBegin.offset, tBegin.consistent ? PSB_CONSISTENT : PSB_INCONSISTENT));
  else
    printStack.push (PrintStackEntry (0, PSB_FITS));
    
  return;
}


void PrettyPrinterBase::print (PPTokenEnd tEnd, const int l)
{
  printStack.pop();
  return;
}


void PrettyPrinterBase::print(PPTokenBreak tBreak, const int l)
{
  if (printStack.empty()) {
    space = space - tBreak.blankSpace;
    indent (tBreak.blankSpace);
  }
  else {
    switch (printStack.top().psBreak)  {
    case PSB_FITS:  {
      space = space - tBreak.blankSpace;
      indent (tBreak.blankSpace);
    } break;
    case PSB_CONSISTENT: {
      space = printStack.top().offset - tBreak.offset;
      printNewLine (margin - space);
    } break;
    case PSB_INCONSISTENT:  {
      if (l > space)  {
	space = printStack.top().offset - tBreak.offset;
	printNewLine (margin-space);
      }
      else {
	space = space - tBreak.blankSpace;
	indent (tBreak.blankSpace);
      }
    } break;
    default: assert (false);
    }
  }
    
  return;
}


void PrettyPrinterBase::print(PPTokenString tString, const int l)
{
  //assert (l <= space);
  space = space - l;
  outstream << tString.str;
    
  return;
}

