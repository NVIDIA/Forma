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

static int ntabs=0;

void print_tabs(FILE* outfile) {
  for(int i =0 ; i < ntabs ; i++ )
    fprintf(outfile,"\t");
}

template<>
void value_node<float>::print_node(FILE* outfile) const{
  fprintf(outfile,"%f",val);
}

template<>
void value_node<double>::print_node(FILE* outfile) const{
  fprintf(outfile,"%lf",val);
}


void id_expr_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"%s",id_name.c_str());
}


void math_fn_expr_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"%s(",fn_name.c_str());
  if( fn_args.size() != 0 ){
    deque<expr_node*>::const_iterator it = fn_args.begin();
    (*it)->print_node(outfile);
    it++;
    for(; it != fn_args.end() ; it++ ){
      fprintf(outfile,",");
      (*it)->print_node(outfile);
    }
  }
  fprintf(outfile,")");
}


void expr_op_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"(");
  lhs_expr->print_node(outfile);
  switch(op){
  case O_LT:
    fprintf(outfile,"<");
    break;
  case O_GT:
    fprintf(outfile,">");
    break;
  case O_LE:
    fprintf(outfile,"<=");
    break;
  case O_GE:
    fprintf(outfile,">=");
    break;
  case O_EQ:
    fprintf(outfile,"==");
    break;
  case O_NE:
    fprintf(outfile,"!=");
    break;
  case O_PLUS:
    fprintf(outfile,"+");
    break;
  case O_MULT:
    fprintf(outfile,"*");
    break;
  case O_MINUS:
    fprintf(outfile,"-");
    break;
  case O_DIV:
    fprintf(outfile,"/");
    break;
  case O_EXP:
    fprintf(outfile,"^");
    break;
  // case O_MATHFN:
  //   fprintf(outfile,"(");
  //   break;
  default:
    fprintf(stderr,"Unsupported mathematical type\n");
  }
  rhs_expr->print_node(outfile);
  // if( op == O_MATHFN )
  //   fprintf(outfile,")");
  fprintf(outfile,")");
}


void ternary_expr_node::print_node(FILE* outfile) const{
  fprintf(outfile,"( ");
  bool_expr->print_node(outfile);
  fprintf(outfile," ? ");
  true_expr->print_node(outfile);
  fprintf(outfile," : ");
  false_expr->print_node(outfile);
  fprintf(outfile," )");
}

void array_access_node::print_node(FILE* outfile) const 
{
  fprintf(outfile,"%s[",param_name.c_str());
  for( deque<expr_node*>::const_iterator it = index_exprs.begin() ; it != index_exprs.end() ; it++){
    (*it)->print_node(outfile);
    if( it != index_exprs.end() - 1 )
      fprintf(outfile,",");
  }
  fprintf(outfile,"]");
}


void stencil_op_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"%s",param_name.c_str());
  scale_fn->print_node(outfile);
}


void domainfn_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"@[");
  deque<scale_coeffs>::const_iterator it = scale_fn.begin();
  fprintf(outfile,"(%d,%d)",it->offset,it->scale);
  it++;
  for(; it != scale_fn.end() ; it++) 
    fprintf(outfile,",(%d,%d)",it->offset,it->scale);
  fprintf(outfile,"]");
#ifdef CHECK_DOMAINS
  fprintf(outfile,"#%d",get_dim());
#endif
}


void int_expr::print_node(FILE* outfile) const
{
  fprintf(outfile,"%d",value);
}

void param_expr::print_node(FILE* outfile) const
{
  fprintf(outfile,"%s",param->param_id.c_str());
}

void binary_expr::print_node(FILE* outfile) const
{
  switch(type) {
  case P_ADD: {
    fprintf(outfile,"(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,"+");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  case P_SUBTRACT: {
    fprintf(outfile,"(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,"-");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  case P_MULTIPLY: {
    fprintf(outfile,"(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,"*");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  case P_DIVIDE: {
    fprintf(outfile,"(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,"/");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  case P_CEIL: {
    fprintf(outfile,"CEIL(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,",");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  case P_MIN: {
    fprintf(outfile,"MIN(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,",");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  case P_MAX: {
    fprintf(outfile,"MAX(");
    lhs_expr->print_node(outfile);
    fprintf(outfile,",");
    rhs_expr->print_node(outfile);
    fprintf(outfile,")");
    break;
  }
  default:
    assert(0);
  }
}

void domain_node::print_node(FILE* outfile) const
{
  if( range_list.size() != 0 ){
    fprintf(outfile,"[");
    for( deque<range_coeffs>::const_iterator it = range_list.begin() ; it != range_list.end() ; it++ ){
      if( it->lb->type == P_INT && it->ub->type == P_INT && static_cast<const int_expr*>(it->lb)->value == static_cast<const int_expr*>(it->ub)->value )
	it->lb->print_node(outfile);
      else{
	it->lb->print_node(outfile);
	fprintf(outfile,"..");
	it->ub->print_node(outfile);
      }
      if( it != range_list.end() - 1) 
	fprintf(outfile,",");
    }
    fprintf(outfile,"]");
#ifdef CHECK_DOMAINS
    fprintf(outfile,"#%d",get_dim());
#endif
  }
}


void compose_expr_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"( ");
  for( deque<pair<domain_desc_node*,vector_expr_node*> >::const_iterator it = output_domains.begin() ; it != output_domains.end() ; it++ ){
    it->first->print_node(outfile);
    fprintf(outfile," = ");
    it->second->print_node(outfile);
    fprintf(outfile,"; ");
  }
  fprintf(outfile,")");
  if( sub_domain )
    sub_domain->print_node(outfile);
#ifdef CHECK_DOMAINS
  fprintf(outfile,"#%d",ndims); if( expr_domain ) expr_domain->print_node(outfile);
#endif
}


void vec_id_expr_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"%s",name.c_str());
  if( sub_domain )
    sub_domain->print_node(outfile);
#ifdef CHECK_DOMAINS
  fprintf(outfile,"#%d",ndims); if( expr_domain ) expr_domain->print_node(outfile);
#endif
}


void bdy_info::print_node(FILE* outfile) const
{
  switch(type){
  case B_NONE:
    break;
  case B_CONSTANT:
    fprintf(outfile,":constant(%d)",value);
    break;
  case B_CLAMPED:
    fprintf(outfile,":clamped");
    break;
  case B_EXTEND:
    fprintf(outfile,":extend");
    break;
  case B_WRAP:
    fprintf(outfile,":wrap");
    break;
  case B_MIRROR:
    fprintf(outfile,":mirror");
    break;
  default:
    assert(0);
  }  
}


void fnid_expr_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"%s(",fn_defn->get_name());
  deque<arg_info>::const_iterator it = args.begin();
  it->arg_expr->print_node(outfile);
  it->bdy_condn->print_node(outfile);
  it++;
  for( ; it != args.end() ; it++ ){
    fprintf(outfile,",");
    it->arg_expr->print_node(outfile);
    it->bdy_condn->print_node(outfile);
  }
  fprintf(outfile,")");
  if( sub_domain )
    sub_domain->print_node(outfile);
#ifdef CHECK_DOMAINS
  fprintf(outfile,"#%d",ndims); if( expr_domain ) expr_domain->print_node(outfile);
#endif
}


void vec_domainfn_node::print_node(FILE* outfile) const
{
  base_vec_expr->print_node(outfile);
  scale_fn->print_node(outfile);
  if( sub_domain )
    sub_domain->print_node(outfile);
#ifdef CHECK_DOMAINS
  fprintf(outfile,"#%d",ndims); if( expr_domain ) expr_domain->print_node(outfile);
#endif
}


void pt_stmt_node::print_node(FILE* outfile) const
{
  print_tabs(outfile);
  fprintf(outfile,"%s = ",lhs_name.c_str());
  rhs_expr->print_node(outfile);
  fprintf(outfile,";\n");
}


void fn_defn_node::print_node(FILE* outfile) const
{
  if( fn_args.size() != 0 ){
    deque<vector_defn_node*>::const_iterator it = fn_args.begin();
    (*it)->print_node(outfile);
    it++;
    for( ; it != this->fn_args.end() ; it++ ){
      fprintf(outfile,",");
      (*it)->print_node(outfile);
    }
  }
  //fn_symbols->print_node(outfile);
}


void stencilfn_defn_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"stencil %s(",fn_name.c_str());
  fn_defn_node::print_node(outfile);
  fprintf(outfile,"){\n");
  ntabs++;
  for( deque<pt_stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    (*it)->print_node(outfile);
  print_tabs(outfile);
  fprintf(outfile,"return ");
  return_expr->print_node(outfile);
  fprintf(outfile,";\n");
  ntabs--;
  fprintf(outfile,"}\n");
}


void vectorfn_defn_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"vector %s(",fn_name.c_str());
  fn_defn_node::print_node(outfile);
  // fn_symbols->print_node(outfile);
  fprintf(outfile,"){\n");
  ntabs++;
  for( deque<stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    (*it)->print_node(outfile);
  print_tabs(outfile);
  fprintf(outfile,"return ");
  return_expr->print_node(outfile);
  fprintf(outfile,";\n");
  ntabs--;
  fprintf(outfile,"}\n");
}


void vectorfn_defn_node::print_statements(FILE* outfile) const
{
  for( deque<stmt_node*>::const_iterator it = fn_body.begin() ; it != fn_body.end() ; it++ )
    (*it)->print_node(outfile);
  fprintf(outfile,"return ");
  return_expr->print_node(outfile);
  fprintf(outfile,";\n");
}

void vectorfn_defn_node::print_symbols(FILE* outfile) const
{
  fn_symbols->print_program(outfile);
}


void parameter_defn::print_node(FILE* outfile) const
{
  fprintf(outfile,"parameter %s",param_id.c_str());
  if( value != DEFAULT_RANGE ){
    fprintf(outfile," = %d",value);
  }
  fprintf(outfile,";\n");
}


void vector_defn_node::print_node(FILE* outfile) const
{
  fprintf(outfile,"vector#%d ",ndims);
  switch(elem_type.type){
  case T_FLOAT:
    fprintf(outfile,"float ");
    break;
  case T_DOUBLE:
    fprintf(outfile,"double ");
    break;
  case T_INT:
    fprintf(outfile,"int ");
    break;
  case T_INT8:
    fprintf(outfile,"int8 ");
    break;
  case T_INT16:
    fprintf(outfile,"int16 ");
    break;
  case T_STRUCT:
    assert(elem_type.struct_info);
    fprintf(outfile,"struct %s ",elem_type.struct_info->name.c_str());
  }
  fprintf(outfile,"%s",name.c_str());
  if( expr_domain ){
    expr_domain->print_node(outfile);
  }
}


void stmt_node::print_node(FILE* outfile) const
{
  print_tabs(outfile);
  fprintf(outfile,"%s",lhs.c_str());
  if( scale_domain){
    scale_domain->print_node(outfile);
  }
  if( sub_domain ){
    sub_domain->print_node(outfile);
  }
  fprintf(outfile," = ");
  rhs->print_node(outfile);
  fprintf(outfile,";\n");
#ifdef CHECK_DOMAINS
  fprintf(outfile,"#%d",ndims); if( expr_domain ) expr_domain->print_node(outfile); fprintf(outfile,"\n");
#endif
}

void local_symbols::print_node(FILE* outfile) const
{
  if( symbols.size() != 0 ){
    deque<pair<string,vector_expr_node*> > ::const_iterator it = symbols.begin();
    vector_defn_node* curr_defn = dynamic_cast<vector_defn_node*>(it->second);
    if( curr_defn)
      curr_defn->print_node(outfile);
    it++;
    for( ; it != this->symbols.end() ; it++ ){
      vector_defn_node* curr_defn = dynamic_cast<vector_defn_node*>(it->second);
      if( curr_defn ){
	fprintf(outfile,",");
	curr_defn->print_node(outfile);
      }
    }
  }
}

void local_symbols::print_program(FILE* outfile) const
{
  if( symbols.size() != 0 ){
    deque<pair<string,vector_expr_node*> > ::const_iterator it = symbols.begin();
    vector_defn_node* curr_defn = dynamic_cast<vector_defn_node*>(it->second);
    if( curr_defn){
      curr_defn->print_node(outfile);
      fprintf(outfile,";\n");
    }
    it++;
    for( ; it != this->symbols.end() ; it++ ){
      curr_defn = dynamic_cast<vector_defn_node*>(it->second);
      if( curr_defn ){
	curr_defn->print_node(outfile);
	fprintf(outfile,";\n");
      }
    }
  }
}


void fn_defs_table::print_node(FILE* outfile) const
{
  for( deque<pair<string,fn_defn_node*> >::const_iterator it = this->symbols.begin() ; it != this->symbols.end() ; it++ )
    it->second->print_node(outfile);
}

void program_node::print_node(FILE* outfile) const
{
  fn_defs->print_node(outfile);
  program_body->print_symbols(outfile);
  program_body->print_statements(outfile);
}
