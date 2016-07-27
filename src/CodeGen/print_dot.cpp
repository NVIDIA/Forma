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
#include "CodeGen/print_dot.hpp"

using namespace std;

void PrintDot::GenerateCode(const program_node* program, const program_options& program_opts)
{
  output_stream << "digraph program{" << endl;
  output_stream << "composite=true;" << endl;
  output_stream << "rankdir=\"BT\";" << endl;
  for(deque<pair<string,fn_defn_node*> >::const_iterator it = fn_defs->begin() ; it != fn_defs->end() ; it++ ){
    const vectorfn_defn_node* curr_fn = dynamic_cast<const vectorfn_defn_node*>(it->second);
    if( curr_fn )
      print(curr_fn);	    
  }
  print(program->get_body());
  output_stream << "}" << endl;

  FILE* CodeGenFile = fopen(program_opts.dot_file_name.c_str(),"w");
  fprintf(CodeGenFile,"%s",output_stream.str().c_str());
  fflush(CodeGenFile);
  fclose(CodeGenFile);
}


void PrintDot::print(const vectorfn_defn_node* vector_fn)
{
  dot_symbol_table fn_symbols;
  const string fn_name = vector_fn->get_name();
  output_stream << "subgraph " << (fn_name.size() == 0 ? "main" : fn_name) << " { " << endl;
  ///Print nodes for vector definitions
  const deque<vector_defn_node*> fn_args = vector_fn->get_args();
  for( deque<vector_defn_node*>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++) 
    if( (*it)->get_dim() > 0 )  {
    print((*it),fn_symbols);
  }
  ///Print nodes for stmts and process the statements
  const deque<stmt_node*> fn_stmts = vector_fn->get_body();
  for( deque<stmt_node*>::const_iterator it = fn_stmts.begin() ; it != fn_stmts.end() ; it++ ){
    print((*it),fn_symbols);
  }
  int curr_node = node_count++;
  output_stream << "n_" << curr_node << "[shape=box,label=\"fn:" << (fn_name.size() == 0 ? "main" : fn_name) << " return\"];" << endl;
  set<int> reaching_defs;
  print(vector_fn->get_return_expr(),fn_symbols,reaching_defs);
  stringstream new_stream;
  for( set<int>::iterator it = reaching_defs.begin() ; it != reaching_defs.end() ; it++ ){
    new_stream << "n_" << *it  << " -> " << "n_" << curr_node ;
    if( vector_fn->get_return_expr()->get_sub_domain() ){
      new_stream << " [label=\"I:" ;
      print(vector_fn->get_return_expr()->get_sub_domain(),new_stream);
      new_stream << "\"]" << endl;
    }
    new_stream << ";" << endl;
  }
  output_stream << new_stream.str();
  output_stream << "}" << endl;
}



void PrintDot::print(const vector_expr_node* vector_expr, dot_symbol_table& fn_symbols, set<int>& reaching_defs)
{
  switch(vector_expr->get_type()){
  case VEC_ID: {
    const set<vector_expr_node*> curr_defns = static_cast<const vec_id_expr_node*>(vector_expr)->get_defn();
    for( set<vector_expr_node*>::const_iterator jt = curr_defns.begin() ; jt != curr_defns.end() ; jt++ ){
      dot_symbol_table::iterator it = fn_symbols.find(*jt);
      assert(it != fn_symbols.end() );
      reaching_defs.insert(it->second);
    }
    break;
  }
  case VEC_DEFN:
    print(static_cast<const vector_defn_node*>(vector_expr),fn_symbols,reaching_defs);
    break;
  case VEC_COMPOSE:
    print(static_cast<const compose_expr_node*>(vector_expr),fn_symbols,reaching_defs);
    break;
  case VEC_SCALE:
    print(static_cast<const vec_domainfn_node*>(vector_expr),fn_symbols,reaching_defs);
    break;
  case VEC_FN:
    print(static_cast<const fnid_expr_node*>(vector_expr),fn_symbols,reaching_defs);
    break;
  case VEC_SCALAR:
    break;
  case VEC_MAKESTRUCT:
    print(static_cast<const make_struct_node*>(vector_expr),fn_symbols,reaching_defs);
    break;
  default:
    assert(0);
  }
}


void PrintDot::print(const stmt_node* stmt, dot_symbol_table& fn_symbols)
{
  if( stmt->get_type() == VEC_FORSTMT || stmt->get_type() == VEC_DOSTMT ){
    const for_stmt_seq* body = static_cast<const stmt_node*>(stmt)->get_body();
    for( deque<stmt_node*>::const_iterator it = body->stmt_list.begin() ; it != body->stmt_list.end() ; it++ ){
      print(*it,fn_symbols);
    }
  }
  else{
    if( stmt->get_access_iterator() ||  stmt->get_rhs()->get_sub_domain() || stmt->get_sub_domain() || stmt->get_scale_domain() ){
      int curr_node = node_count++;
      if( stmt->get_access_iterator() ){
	const for_iterator* access_iterator = stmt->get_access_iterator();
	output_stream << "n_" << curr_node << "[label=\"" << access_iterator->name << ":" ;
	assert(access_iterator->iter_domain->range_list.size() == 1);
	const range_coeffs iter_domain = access_iterator->iter_domain->range_list.front();
	print(iter_domain.lb,output_stream);
	output_stream << "," ;
	print(iter_domain.ub,output_stream);
	output_stream <<  "\\n" <<  "stmt:" << stmt->get_name_string() << "<" << access_iterator->name << ">\"];" << endl;    
      }
      else{
	output_stream << "n_" << curr_node << "[label=\"stmt:" << stmt->get_name_string() << "\"];" << endl;
      }
      stringstream new_stream;
      set<int> reaching_defs;
      print(stmt->get_rhs(),fn_symbols,reaching_defs);
      for( set<int>::iterator it = reaching_defs.begin() ; it != reaching_defs.end() ; it++ ){
	new_stream << "n_" << *it << " -> n_" << curr_node;
	new_stream << " [label=\"" ;
	if( stmt->get_rhs()->get_sub_domain() ){
	  new_stream << "I:" ;
	  print(stmt->get_rhs()->get_sub_domain(),new_stream);
	  new_stream << "\\n";
	}
	if( stmt->get_sub_domain() ){
	  new_stream << "O:" ;
	  print(stmt->get_sub_domain(),new_stream);
	}
	else if( stmt->get_scale_domain() ){
	  new_stream << "O:" ;
	  print(stmt->get_scale_domain(),new_stream);
	}
	new_stream << "\"]" ;
	new_stream << ";" << endl;
      }
      output_stream << new_stream.str();
      fn_symbols.insert(dot_symbol_entry(stmt,curr_node));
    }
    else{
      set<int> reaching_defs;
      print(stmt->get_rhs(),fn_symbols,reaching_defs);
      assert(reaching_defs.size() == 1);
      fn_symbols.insert(dot_symbol_entry(stmt,*(reaching_defs.begin())));
    }
  }
}


void PrintDot::print(const vector_defn_node* vector_defn, dot_symbol_table& fn_symbols)
{
  int curr_node = node_count++;
  output_stream << "n_" << curr_node << "[label=\"arg:" << vector_defn->get_name() << "\"];" << endl;
  fn_symbols.insert(dot_symbol_entry(vector_defn,curr_node));				     
}


void PrintDot::print(const fnid_expr_node* fn_call, dot_symbol_table& fn_symbols, set<int>& reaching_defs)
{
  int curr_node = node_count++;
  const fn_defn_node* fn_defn = fn_call->get_defn();
  const vectorfn_defn_node* vector_fn = dynamic_cast<const vectorfn_defn_node*>(fn_defn);
  output_stream << "n_" << curr_node;
  if( vector_fn ){
    output_stream << "[shape=box,label=\"fn:" << fn_defn->get_name() << "\"];" << endl;
  }
  else{
    print(static_cast<const stencilfn_defn_node*>(fn_defn),output_stream);
  }
  stringstream new_stream;
  const deque<arg_info> fn_args = fn_call->get_args();
  for( deque<arg_info>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++ ){
    set<int> curr_reaching_defs;
    print(it->arg_expr,fn_symbols,curr_reaching_defs);
    for( set<int>::iterator jt = curr_reaching_defs.begin() ; jt != curr_reaching_defs.end() ; jt++ ){
      new_stream << "n_" << *jt << " -> n_" << curr_node ;
      if( it->arg_expr->get_sub_domain() ){
	new_stream << " [label=\"I:" ;
	print(it->arg_expr->get_sub_domain(),new_stream);
	new_stream << "\"]";
      }
      new_stream << ";" << endl;
    }
  }
  output_stream << new_stream.str();
  reaching_defs.insert(curr_node);
}


void PrintDot::print(const stencilfn_defn_node* stencil_fn, stringstream& curr_stream) const
{
  const string fn_name = stencil_fn->get_name();
  const deque<vector_defn_node*> fn_args=  stencil_fn->get_args();
  int arg_num=0;
  curr_stream << "[shape=box,label=\"stencil:" << fn_name ;
  for( deque<vector_defn_node*>::const_iterator it = fn_args.begin() ; it != fn_args.end() ; it++, arg_num++ ){
    if( (*it)->get_dim() != 0 ){
      curr_stream << "\\n,arg" << arg_num << "[" ; 
      const deque<offset_hull>  arg_offset = (*it)->get_access_info();
      for( deque<offset_hull>::const_iterator jt = arg_offset.begin() ; jt != arg_offset.end() ; jt++ ){
	curr_stream << "(" << jt->max_negetive << "," << jt->max_positive << ")" ;
	if( jt != arg_offset.end() -1 )
	  curr_stream << ",";
      }
      curr_stream << "]" ;
    }
  }
  curr_stream << "\"];" << endl;
}


void PrintDot::print(const compose_expr_node* compose_expr, dot_symbol_table& fn_symbols, set<int>& reaching_defs)
{
  int curr_node = node_count++;
  output_stream << "n_" << curr_node << "[label=\"compose\"];" << endl;
  const deque<pair<domain_desc_node*,vector_expr_node*> > expr_list = compose_expr->get_expr_list();
  stringstream  new_stream;
  for( deque<pair<domain_desc_node*,vector_expr_node*> >::const_iterator it = expr_list.begin() ; it != expr_list.end() ; it++ ){
    set<int> curr_reaching_defs;
    print(it->second,fn_symbols,curr_reaching_defs);
    for( set<int>::iterator jt = curr_reaching_defs.begin() ; jt != curr_reaching_defs.end() ; jt++ ){
      new_stream << "n_" << *jt << " -> n_" << curr_node << " [label=\"" ;
      if( it->second->get_sub_domain() ){
	new_stream << "I:" ;
	print(it->second->get_sub_domain(),new_stream);
	new_stream << "\\n";
      }
      new_stream << "O:";
      const domain_node* offset = dynamic_cast<const domain_node*>(it->first);
      if( offset ){
	print(offset,new_stream);
      }
      else{
	print(static_cast<const domainfn_node*>(it->first),new_stream);
      }
      new_stream << "\"]" ;
      new_stream << ";" << endl;
    }
  }
  output_stream << new_stream.str();
  reaching_defs.insert(curr_node);
}


void PrintDot::print(const make_struct_node* struct_expr, dot_symbol_table& fn_symbols, set<int>& reaching_defs)
{
  int curr_node = node_count++;
  output_stream << "n_" << curr_node << "[label=\"make_struct\"];" << endl;
  const deque<vector_expr_node*> field_inputs = struct_expr->get_field_inputs();
  stringstream new_stream;
  for( deque<vector_expr_node*>::const_iterator it = field_inputs.begin() ; it != field_inputs.end() ; it++ ){
    set<int> curr_reaching_defs;
    print((*it),fn_symbols,curr_reaching_defs);
    for( set<int>::iterator jt = curr_reaching_defs.begin(); jt != curr_reaching_defs.end(); jt++ ){
      new_stream << "n_" << *jt << " -> n_" << curr_node << " [label=\"" ;
      if( (*it)->get_sub_domain() ){
	new_stream << "I:" ;
	print((*it)->get_sub_domain(),new_stream);
	new_stream << "\\n";
      }
      new_stream << "\"];"  << endl;      
    }
  }
}

void PrintDot::print(const vec_domainfn_node* scale_expr, dot_symbol_table& fn_symbols, set<int>& reaching_defs)
{
  int curr_node = node_count++;
  output_stream << "n_" << curr_node << "[label=\"Scale:" ;
  print(scale_expr->get_scale_fn(),output_stream);
  output_stream << "\"];"  << endl;
  stringstream new_stream;
  set<int> curr_reaching_defs;
  print(scale_expr->get_base_expr(),fn_symbols,curr_reaching_defs);
  for( set<int>::iterator it = curr_reaching_defs.begin() ; it != curr_reaching_defs.end() ; it++ ){
    new_stream << "n_" << *it << " -> n_" << curr_node;
    if( scale_expr->get_base_expr()->get_sub_domain() ){
      new_stream << " [label=\"I:" ;
      print(scale_expr->get_base_expr()->get_sub_domain(),new_stream);
      new_stream << "\"]" << endl;
    }
    new_stream << ";" << endl;
  }
  output_stream << new_stream.str();
  reaching_defs.insert(curr_node);
}


void PrintDot::print(const domain_node* domain, stringstream& curr_stream) const
{
  curr_stream << "[";
  const deque<range_coeffs>& range_list = domain->range_list;
  for( deque<range_coeffs>::const_iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
    if( !it->lb->is_default() )
      print(it->lb,curr_stream);
    curr_stream << ".." ;
    if( !it->ub->is_default() )
      print(it->ub,curr_stream);
    if( it != range_list.end() -1 )
      curr_stream << "," ;
  }
  curr_stream << "]";
}


void PrintDot::print(const parametric_exp* curr_exp, stringstream& curr_stream) const 
{
  switch(curr_exp->type){
  case P_INT:
    curr_stream << static_cast<const int_expr*>(curr_exp)->value;
    break;
  case P_PARAM:
    curr_stream << static_cast<const param_expr*>(curr_exp)->param->param_id;
    break;
  case P_ADD: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << "+";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_SUBTRACT: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << "-";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_MULTIPLY: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << "*";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_DIVIDE: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << "/";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_MAX: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "MAX(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << ",";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_MIN: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "MIN(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << ",";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_CEIL: {
    const binary_expr* curr_binary = static_cast<const binary_expr*>(curr_exp);
    curr_stream << "CEIL(";
    print(curr_binary->lhs_expr,curr_stream);
    curr_stream << ",";
    print(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  default:
    assert(0);
  }
}


void PrintDot::print(const domainfn_node* domainfn, stringstream& curr_stream) const
{
  curr_stream << "@[";
  const deque<scale_coeffs>& scale_fn = domainfn->scale_fn;
  for( deque<scale_coeffs>::const_iterator it = scale_fn.begin() ; it != scale_fn.end() ; it++ ){
    if( it->scale == 1 ){
      curr_stream << it->offset;
    }
    else{
      curr_stream << "(" << it->offset << "," << it->scale << ")";
    }
    if( it != scale_fn.end() - 1 )
      curr_stream << ",";
  }
  curr_stream << "]";
}


