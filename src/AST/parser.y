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
%{
#include "AST/parser.hpp"
#include <stdio.h>
#include <string>
#include <deque>

  extern "C" int yylex();
  extern "C" int yyparse();
  extern FILE* yyin;

  ///Global definition of the function definitions table.
  fn_defs_table* fn_defs=NULL;
  
  ///Global definition of defined data types
  defined_types_table* defined_types = NULL;

  ///Global definitions of parameters
  param_table* global_params = NULL;

  void yyerror(const char * s);
  local_symbols* curr_local_symbols = new local_symbols();
  local_scalar_symbols* curr_local_scalars = NULL;
  std::deque<for_iterator*> curr_iterator_stack;
  //param_table* curr_parameters = new param_table();

%}

%union{
  int int_val;
  char * id_name;
  float float_val;
  double double_val;
  class domainfn_node * domainfn_type;
  class expr_node* expr_type;
  class domain_node * domain_type;
  class vector_expr_node * vector_type;
  class compose_expr_node * compose_type;
  class make_struct_node* struct_expr_type;
  class fnid_expr_node* fn_type;
  class bdy_info* bdy_info_type;
  class local_symbols* local_sym_type;
  class vector_defn_node* vector_defn_type;
  class stmt_node* stmt_type;
  class for_stmt_seq* stmtseq_type;
  class vectorfn_defn_node* vectorfn_type;
  class stencilfn_defn_node* stencilfn_type;
  class fn_defs_table* fn_def_type;
  class math_fn_expr_node* pt_fn_type;
  class data_types* data_type;
  class program_node* program_type;
  class parametric_exp* param_type;
  class defined_data_type* def_data_type;
  class expr_node* stencil_ret_type;
  class pt_struct_node* stencil_struct_type;
  class array_access_node* array_access_type;
 }

%token <int_val> INT
%token <float_val> FLOAT
%token <double_val> DOUBLE
%token DOTDOT
%token STENCIL
%token RETURN
%token PARAMETER
%token VECTOR
%token VECTORFN
%token FOR
%token ENDFOR
%token DO
%token WHILE
%token CLAMPED
%token WRAP
%token CONSTANT
%token EXTEND
%token MIRROR
%token CAST
/* %token MIN */
/* %token MAX */
/* %token CEIL */
%token STRUCT
%token <int_val> BASICTYPE
%token <id_name> ID
%left '<' '>' LE GE NE EQ
%left '-' '+'
%left '*' '/'
%left NEG
%right '^'
%type <domainfn_type> domainfn
%type <domainfn_type> offsetlist
%type <expr_type> expr
%type <expr_type> idexpr
%type <stencilfn_type> stencilstmts
%type <stencil_ret_type> stencilret
%type <stencil_struct_type> fieldlist
%type <array_access_type> arrayindexlist
%type <def_data_type> fieldsdefn
%type <domain_type> domain
%type <domain_type> rangelist
%type <param_type> paramexpr
%type <compose_type> compose
%type <struct_expr_type> structexprlist
%type <vector_type> vectorexpr
%type <domain_type> intlist
%type <data_type> datatype
%type <fn_type> vectorexprlist
%type <bdy_info_type> bdytype
%type <vectorfn_type> stmtseq
%type <pt_fn_type> ptfnarglist
%type <local_sym_type> arglist
%type <stmt_type> stmt
%type <stmtseq_type> forstmtseq
%type <program_type> program

%locations
%error-verbose

%%

program :
fntypedefs variabledefs stmtseq RETURN vectorexpr ';' {
  $3->add_ret_expr($5);
  printf("Nfunctions:%d\n",fn_defs->get_nsymbols());
  program_node* new_program = new program_node($3);
  $$ = new_program;
  parser::root_node = $$;
}

fntypedefs :
fntypedefs stencilfn{ }
| fntypedefs vectorfn{ }
| fntypedefs typedefn{ }
| { }


typedefn :
STRUCT ID '{' fieldsdefn '}' {
  defined_data_type* new_data_type  = defined_types->find_symbol($2);
  if( new_data_type != 0 ){
    fprintf(stderr,"[ME] : Error! Duplicate defined type (%s) defn :%d.%d-%d.%d \n",$2,yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  defined_types->add_new_symbol($2,$4);
  $4->set_name($2);
  free($2);
}
| STRUCT ID ';' {
  data_types::add_cuda_types($2);
  free($2);
}

fieldsdefn :
fieldsdefn BASICTYPE ID ';' {
  $1->add_field($3,get_basic_data_type($2));
  $$ = $1;
  free($3);
}
| BASICTYPE ID ';' {
  defined_data_type* new_data_type = new defined_data_type;
  new_data_type->add_field($2,get_basic_data_type($1));
  $$ = new_data_type;
  free($2);
}

stencilfn:
STENCIL ID '(' arglist ')' '{' stencilstmts stencilret ';' '}'{
  $7->set_name($2);
  $7->add_ret_expr($8);
  fn_defs->add_fn_def($7);
  curr_local_symbols = new local_symbols();
  free($2);
}

stencilret:
RETURN expr {
  $$ = $2;
}
| RETURN STRUCT ID '(' fieldlist ')' {
  $5->find_struct_definition($3);
  $$ = $5;
  free($3);
}


fieldlist: 
fieldlist ',' expr {
  $1->add_field($3);
  $$ = $1;
}
| expr {
  $$ = new pt_struct_node($1);
}


vectorfn:
VECTORFN ID '(' arglist ')' '{' stmtseq RETURN vectorexpr ';' '}' {
  $7->set_name($2);
  $7->add_ret_expr($9);
  fn_defs->add_fn_def($7);
  curr_local_symbols = new local_symbols();
  //curr_parameters = new param_table();
  free($2);
}

datatype:
BASICTYPE {
  data_types* curr_data_type = new data_types();
  curr_data_type->assign(get_basic_data_type($1));
  $$ = curr_data_type;
}
| ID {
  data_types* curr_data_type = new data_types();
  curr_data_type->assign(get_data_type($1));
  $$ = curr_data_type;
  free($1);
  }



arglist:
arglist ',' VECTOR INT datatype ID {
  vector_defn_node* new_arg = new vector_defn_node($5,$4,$6);
  $1->add_local_symbol($6,new_arg);
  $$ = $1;
  free($6);
  delete $5;
}
| arglist ',' BASICTYPE ID {
  data_types* curr_data_type = new data_types;
  curr_data_type->assign(get_basic_data_type($3));
  vector_defn_node* new_arg = new vector_defn_node(curr_data_type,0,$4);
  delete curr_data_type;
  $1->add_local_symbol($4,new_arg);
  $$ = $1;
  free($4);
  }
| VECTOR INT datatype ID {
  //local_symbols* sym_table = new local_symbols();
  vector_defn_node* new_arg = new vector_defn_node($3,$2,$4);
  curr_local_symbols->add_local_symbol($4,new_arg);
  $$ = curr_local_symbols;
  delete $3;
  free($4);
}
| BASICTYPE ID{
  //local_symbols* sym_table = new local_symbols();
  data_types* curr_data_type = new data_types;
  curr_data_type->assign(get_basic_data_type($1));
  vector_defn_node* new_arg = new vector_defn_node(curr_data_type,0,$2);
  delete curr_data_type;
  curr_local_symbols->add_local_symbol($2,new_arg);
  $$ = curr_local_symbols;
  free($2);
}
| {
  //local_symbols* sym_table = new local_symbols();
  $$ = curr_local_symbols;
  }


stencilstmts :
stencilstmts ID '=' expr ';' {
  pt_stmt_node* new_stmt = new pt_stmt_node($2,$4,curr_local_symbols);
  $1->add_stmt(new_stmt);
  curr_local_scalars->add_local_scalar($2,$4);
  $$ = $1;
  free($2);
}
| {
  curr_local_scalars = new local_scalar_symbols();
  stencilfn_defn_node* new_stencil = new stencilfn_defn_node(curr_local_symbols,curr_local_scalars);
  $$ = new_stencil;
}

expr:
idexpr {
  $$ = $1;
}
| idexpr '.' ID {
  if( dynamic_cast<stencil_op_node*>($1) == NULL ){
    fprintf(stderr,"[ME]: Error at %d.%d-%d.%d : Unsupported structure field reference to local variable within stencil function\n",yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  stencil_op_node* new_stencil_access = static_cast<stencil_op_node*>($1);
  new_stencil_access->add_access_field($3);
  $$ = new_stencil_access;
  free($3);
}
| FLOAT {
  $$ = new value_node<float>($1);
}
| DOUBLE {
  $$ = new value_node<double>($1);
  }
| INT {
  $$ = new value_node<int>($1);
  }
| expr '<' expr {
  $$ = new expr_op_node($1,$3,O_LT);
}
| CAST '<' BASICTYPE '>' '(' expr ')' {
  if( $6->get_data_type().type == T_STRUCT ){
    fprintf(stderr,"[ME] : Error at %d.%d-%d.%d : Invalid cast of struct operator\n",yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  $6->cast_to_type(get_basic_data_type($3));
  $$ = $6;
  }
| expr '>' expr {
  $$ = new expr_op_node($1,$3,O_GT);
  }
| expr LE expr {
  $$ = new expr_op_node($1,$3,O_LE);
}
| expr GE expr {
  $$ = new expr_op_node($1,$3,O_GE);
  }
| expr NE expr {
  $$ = new expr_op_node($1,$3,O_NE);
}
| expr EQ expr {
  $$ = new expr_op_node($1,$3,O_EQ);
  }
| expr '+' expr {
  $$ = new expr_op_node($1,$3,O_PLUS);
}
| expr '-' expr {
  $$ = new expr_op_node($1,$3,O_MINUS);
  }
| expr '*' expr {
  $$ = new expr_op_node($1,$3,O_MULT);
  }
| expr '/' expr {
  $$ = new expr_op_node($1,$3,O_DIV);
  }
| expr '^' expr {
  $$ = new expr_op_node($1,$3,O_EXP);
  }
| '(' expr ')' {
  $$ = $2;
  }
| '-' expr %prec NEG {
  $$ = new unary_neg_expr_node($2) ;
  }
| ID '(' ptfnarglist ')' {
  $3->set_name($1);
  $$ = $3;
  free($1);
  }
| '(' expr '?' expr ':' expr ')' {
  $$ = new ternary_expr_node($2,$4,$6);
  }


idexpr:
ID {
  vector_expr_node* id_defn = (curr_local_symbols->find_symbol($1));
  if( id_defn == 0 ){
    $$ = new id_expr_node($1,curr_local_scalars);
  }
  else{
    /* if( id_defn->get_dim() == 0 ){ */
    /*   $$ = new id_expr_node($1,id_defn); */
    /* } */
    /* else{ */
      $$ = new stencil_op_node(dynamic_cast<vector_defn_node*>(id_defn));
    /* } */
  }
  free($1);
}
| ID domainfn {
  $$ = new stencil_op_node($1,$2,curr_local_symbols);
  free($1);
}
| ID '[' arrayindexlist ']' {
  $3->set_name($1,curr_local_symbols);
  $$ = $3;
  free($1);
}

arrayindexlist :
arrayindexlist ','expr {
  $1->add_index($3);
  $$ = $1;
}
| expr {
  array_access_node* new_access = new array_access_node($1);
  $$ = new_access;
  }


ptfnarglist:
ptfnarglist ',' expr {
  $1->add_arg($3);
  $$ = $1;
}
| expr {
  math_fn_expr_node* new_fn = new math_fn_expr_node();
  new_fn->add_arg($1);
  $$ = new_fn;
  }
| {
  math_fn_expr_node* new_fn = new math_fn_expr_node();
  $$ = new_fn;
  }


/* variabledefs : */
/* variabledefs parameterdefs  */
/* | variabledefs vectordefs  */
/* | parameterdefs */
/* | vectordefs */
variabledefs:
variabledefs parameterdefs
| variabledefs vectordefs
| {  }


parameterdefs:
PARAMETER parameterlist ';' 

parameterlist:
parameterlist ',' ID {
  int not_new_param = global_params->add_symbol($3,new parameter_defn($3));
  if( not_new_param ){
    fprintf(stderr,"[ME] : Error! Duplicate parameter defn :%d.%d-%d.%d \n",yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  free($3);
}
| parameterlist ',' ID '=' INT {
  int not_new_param = global_params->add_symbol($3,new parameter_defn($3,$5));
  if( not_new_param ){
    fprintf(stderr,"[ME] : Error! Duplicate parameter defn :%d.%d-%d.%d \n",yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  free($3);
  }
| ID {
  int not_new_param = global_params->add_symbol($1,new parameter_defn($1));
  if( not_new_param ){
    fprintf(stderr,"[ME] : Error! Duplicate parameter defn :%d.%d-%d.%d \n",yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  free($1);
  }
| ID '=' INT {
  int not_new_param = global_params->add_symbol($1,new parameter_defn($1,$3));
  if( not_new_param ){
    fprintf(stderr,"[ME] : Error! Duplicate parameter defn :%d.%d-%d.%d \n",yylloc.first_line, yylloc.first_column,yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  free($1);
  }


vectordefs:
VECTOR INT datatype ID '[' intlist ']' ';' {
  //$7->set_vector_info(get_data_type($4),$3,$5);
  vector_defn_node* new_vecdef = new vector_defn_node($3,$2,$4,$6);
  curr_local_symbols->add_local_symbol($4,new_vecdef);
  //$$ = $1;
  free($4);
  delete $3;
  delete $6;
}
| BASICTYPE ID ';' {
  data_types* curr_data_type = new data_types();
  curr_data_type->assign(get_basic_data_type($1));
  vector_defn_node* new_vecdef = new vector_defn_node(curr_data_type,0,$2);
  delete curr_data_type;
  curr_local_symbols->add_local_symbol($2,new_vecdef);
  //  $$ = $1;
  free($2);
}


intlist :
intlist ',' paramexpr{
  $1->add_size($3);
  $$ = $1;
}
| paramexpr {
  domain_node* new_domain = new domain_node;
  new_domain->add_size($1);
  $$ = new_domain;
}



stmtseq :
stmtseq stmt{
  $1->add_stmt($2);
  $$ = $1;
}
| { 
  $$ = new vectorfn_defn_node(curr_local_symbols);
}


stmt :
ID '=' vectorexpr ';'{
  stmt_node* new_stmt = new stmt_node($1,$3);
  curr_local_symbols->add_local_symbol($1,new_stmt);
  $$ = new_stmt;
  free($1);
}
| ID domainfn '=' vectorexpr ';'{
  stmt_node* new_stmt = new stmt_node($1,$4,$2);
  curr_local_symbols->add_local_symbol($1,new_stmt);
  $$ = new_stmt;
  free($1);
} 
| ID domain '=' vectorexpr ';'{
  stmt_node* new_stmt = new stmt_node($1,$4,$2);
  curr_local_symbols->add_local_symbol($1,new_stmt);
  $$ = new_stmt;
  free($1);
}
| ID '<' INT '>' '=' vectorexpr ';' {
  stmt_node* new_stmt = new stmt_node($1,$6,$3);
  vector_expr_node* curr_sym = curr_local_symbols->find_symbol($1);
  multi_stmt_node* new_multi_stmt;
  if( curr_sym == NULL ){
    new_multi_stmt = new multi_stmt_node(new domain_node(new int_expr($3),new int_expr($3)),new_stmt);
    curr_local_symbols->add_local_symbol($1,new_multi_stmt);
  }
  else{
    new_multi_stmt = dynamic_cast<multi_stmt_node*>(curr_sym);
    new_multi_stmt->add_def_domain(new domain_node(new int_expr($3),new int_expr($3)),new_stmt);
  }
  $$ = new_stmt;
  free($1);
  }
| ID '<' INT '>'domain '=' vectorexpr ';' {
  stmt_node* new_stmt = new stmt_node($1,$7,$5,$3);
  vector_expr_node* curr_sym = curr_local_symbols->find_symbol($1);
  multi_stmt_node* new_multi_stmt;
  if( curr_sym == NULL ){
    new_multi_stmt = new multi_stmt_node(new domain_node(new int_expr($3),new int_expr($3)),new_stmt);
    curr_local_symbols->add_local_symbol($1,new_multi_stmt);
  }
  else{
    new_multi_stmt = dynamic_cast<multi_stmt_node*>(curr_sym);
    new_multi_stmt->add_def_domain(new domain_node(new int_expr($3),new int_expr($3)),new_stmt);
  }
  $$ = new_stmt;
  free($1);
  }
| ID '<' INT '>'domainfn '=' vectorexpr ';' {
  stmt_node* new_stmt = new stmt_node($1,$7,$5,$3);
  vector_expr_node* curr_sym = curr_local_symbols->find_symbol($1);
  multi_stmt_node* new_multi_stmt;
  if( curr_sym == NULL ){
    new_multi_stmt = new multi_stmt_node(new domain_node(new int_expr($3),new int_expr($3)),new_stmt);
    curr_local_symbols->add_local_symbol($1,new_multi_stmt);
  }
  else{
    new_multi_stmt = dynamic_cast<multi_stmt_node*>(curr_sym);
    new_multi_stmt->add_def_domain(new domain_node(new int_expr($3),new int_expr($3)),new_stmt);
  }
  $$ = new_stmt;
  free($1);
  }
| ID '<' ID '>' '=' vectorexpr ';' {
  std::deque<for_iterator*>::reverse_iterator it = curr_iterator_stack.rbegin() ;
  for( ; it != curr_iterator_stack.rend() ; it++ ){
    if( (*it)->name.compare($3) == 0 ){
      vector_expr_node* curr_sym = curr_local_symbols->find_symbol($1);
      stmt_node* new_stmt = new stmt_node($1,$6,0,(*it));
      multi_stmt_node* new_multi_stmt;
      if( curr_sym == NULL ){
	new_multi_stmt = new multi_stmt_node(new domain_node((*it)->iter_domain),new_stmt);
	curr_local_symbols->add_local_symbol($1,new_multi_stmt);
      }
      else{
	new_multi_stmt = dynamic_cast<multi_stmt_node*>(curr_sym);
	new_multi_stmt->add_def_domain(new domain_node((*it)->iter_domain),new_stmt);
      }
      $$ = new_stmt;
      break;
    }
  }
  if( it == curr_iterator_stack.rend() ){
    fprintf(stderr,"[ME] : ERROR! : in %s<%s> %s, doesnt refer to an iterator\n",$1,$3,$3);
    exit(1);
  }
  free($3);
  free($1);
  }
| ID '<' ID '>'domainfn '=' vectorexpr ';' {
  std::deque<for_iterator*>::reverse_iterator it = curr_iterator_stack.rbegin() ;
  for( ; it != curr_iterator_stack.rend() ; it++ ){
    if( (*it)->name.compare($3) == 0 ){
      vector_expr_node* curr_sym = curr_local_symbols->find_symbol($1);
      stmt_node* new_stmt = new stmt_node($1,$7,$5,0,(*it));
      multi_stmt_node* new_multi_stmt;
      if( curr_sym == NULL ){
	new_multi_stmt = new multi_stmt_node(new domain_node((*it)->iter_domain),new_stmt);
	curr_local_symbols->add_local_symbol($1,new_multi_stmt);
      }
      else{
	new_multi_stmt = dynamic_cast<multi_stmt_node*>(curr_sym);
	new_multi_stmt->add_def_domain(new domain_node((*it)->iter_domain),new_stmt);
      }
      $$ = new_stmt;
      break;
    }
  }
  if( it == curr_iterator_stack.rend() ){
    fprintf(stderr,"[ME] : ERROR! : in %s<%s> %s, doesnt refer to an iterator\n",$1,$3,$3);
    exit(1);
  }
  free($3);
  free($1);
  }
| ID '<' ID '>'domain '=' vectorexpr ';' {
  std::deque<for_iterator*>::reverse_iterator it = curr_iterator_stack.rbegin() ;
  for( ; it != curr_iterator_stack.rend() ; it++ ){
    if( (*it)->name.compare($3) == 0 ){
      vector_expr_node* curr_sym = curr_local_symbols->find_symbol($1);
      stmt_node* new_stmt = new stmt_node($1,$7,$5,0,(*it));
      multi_stmt_node* new_multi_stmt;
      if( curr_sym == NULL ){
	new_multi_stmt = new multi_stmt_node(new domain_node((*it)->iter_domain),new_stmt);
	curr_local_symbols->add_local_symbol($1,new_multi_stmt);
      }
      else{
	new_multi_stmt = dynamic_cast<multi_stmt_node*>(curr_sym);
	new_multi_stmt->add_def_domain(new domain_node((*it)->iter_domain),new_stmt);
      }
      $$ = new_stmt;
      break;
    }
  }
  if( it == curr_iterator_stack.rend() ){
    fprintf(stderr,"[ME] : ERROR! : in %s<%s> %s, doesnt refer to an iterator\n",$1,$3,$3);
    exit(1);
  }
  free($3);
  free($1);
  }
| FOR ID '=' INT DOTDOT INT { 
  for_iterator* new_iterator;
  if( $4 <= $6 )
    new_iterator = new for_iterator($2,new domain_node(new int_expr($4),new int_expr($6)),true);
  else
    new_iterator = new for_iterator($2,new domain_node(new int_expr($6),new int_expr($4)),false);
  curr_iterator_stack.push_back(new_iterator);
}  forstmtseq ENDFOR {
  for_iterator * curr_iterator = curr_iterator_stack.back();
  curr_iterator_stack.pop_back();
  $$ = new for_stmt_node(curr_iterator,$8);
  free($2);
}
| DO ID '=' INT DOTDOT INT {
  for_iterator* new_iterator;
  if( $4 <= $6 )
    new_iterator = new for_iterator($2,new domain_node(new int_expr($4),new int_expr($6)),true);
  else
    new_iterator = new for_iterator($2,new domain_node(new int_expr($6),new int_expr($4)),false);
  curr_iterator_stack.push_back(new_iterator);
} forstmtseq WHILE '(' vectorexpr ')' ';' {
  for_iterator * curr_iterator = curr_iterator_stack.back();
  $$ = new do_stmt_node(curr_iterator,$8,$11);
  free($2);
}


forstmtseq :
forstmtseq stmt {
  $1->stmt_list.push_back($2);
  $$ = $1;
}
| stmt {
  $$ = new for_stmt_seq;
  $$->stmt_list.push_back($1);
}



vectorexpr :
ID {
  $$ = new vec_id_expr_node($1,curr_local_symbols) ;
  free($1);
} 
| vectorexpr '.' ID {
  $1->add_access_field($3);
  $$ = $1;
  free($3);
  }
| ID '(' vectorexprlist ')'{
  $3->find_definition($1);
  $$ = $3;
  free($1);
  }
| STRUCT ID '(' structexprlist ')' {
  $4->find_struct_definition($2);
  $$ = $4;
  free($2);
}
| vectorexpr domain{
  $1->set_domain($2);
  $$ = $1;
}
| vectorexpr domainfn{
  vec_domainfn_node* new_df_node = new vec_domainfn_node($1,$2);
  $$ = new_df_node;
}
| '(' compose ')'{
  $$ = $2;
  }
| ID '<' ID '-' INT '>' {
  std::deque<for_iterator*>::reverse_iterator it = curr_iterator_stack.rbegin() ;
  for( ; it != curr_iterator_stack.rend() ; it++){
    if( (*it)->name.compare($3) == 0 ){
      $$ = new vec_id_expr_node($1,curr_local_symbols,$5,(*it));
      break;
    }    
  }
  if( it == curr_iterator_stack.rend() ){
    fprintf(stderr,"[ME] : ERROR : Using undefined iterator %s in %s<%s-%d>\n",$3,$1,$3,$5);
    exit(1);
  }
  if( !(*it)->is_positive ){
    fprintf(stderr,"[ME] : ERROR : Using negetive offset with negetive iterator %s in %s<%s-%d>\n",$3,$1,$3,$5);
    exit(1);
  }
  free($1);
  free($3);
  }
| ID '<' ID '+' INT '>' {
  std::deque<for_iterator*>::reverse_iterator it = curr_iterator_stack.rbegin() ;
  for( ; it != curr_iterator_stack.rend() ; it++){
    if( (*it)->name.compare($3) == 0 ){
      $$ = new vec_id_expr_node($1,curr_local_symbols,$5,(*it));
      break;
    }    
  }
  if( it == curr_iterator_stack.rend() ){
    fprintf(stderr,"[ME] : ERROR : Using undefined iterator %s in %s<%s-%d>\n",$3,$1,$3,$5);
    exit(1);
  }
  if( (*it)->is_positive ){
    fprintf(stderr,"[ME] : ERROR : Using positive offset with positive iterator %s in %s<%s+%d>\n",$3,$1,$3,$5);
    exit(1);
  }
  free($1);
  free($3);
  }
| ID '<' INT '>' {
  $$ = new vec_id_expr_node($1,curr_local_symbols,$3,NULL);  
  free($1);
  }
| ID '<' ID '>' {
  std::deque<for_iterator*>::reverse_iterator it = curr_iterator_stack.rbegin() ;
  for( ; it != curr_iterator_stack.rend() ; it++){
    if( (*it)->name.compare($3) == 0 ){
      $$ = new vec_id_expr_node($1,curr_local_symbols,DEFAULT_RANGE,(*it));
      break;
    }    
  }
  if( it == curr_iterator_stack.rend() ){
    fprintf(stderr,"[ME] : ERROR : Using undefined iterator %s in %s<%s>\n",$3,$1,$3);
    exit(1);
  }
  free($1);
  free($3);
  }



structexprlist:
structexprlist ',' vectorexpr {
  $1->add_field_input($3);
  $$ = $1;
}
| vectorexpr {
  $$ = new make_struct_node($1);
  }


vectorexprlist :
vectorexprlist ',' vectorexpr {
  $1->add_arg($3,new bdy_info());
  $$ = $1;
}
| vectorexprlist ',' vectorexpr ':' bdytype {
  $1->add_arg($3,$5);
  $$ = $1;
  }
| vectorexprlist ',' FLOAT {
  $1->add_arg(new value_node<float>($3));
  $$ = $1;
  }
| vectorexprlist ',' DOUBLE {
  $1->add_arg(new value_node<double>($3));
  $$ = $1;
  }
| vectorexprlist ',' INT {
  $1->add_arg(new value_node<int>($3));
  $$ = $1;
  }
| vectorexpr {
  fnid_expr_node* new_fn = new fnid_expr_node();
  new_fn->add_arg($1,new bdy_info());
  $$ = new_fn;
  }
| vectorexpr ':' bdytype {
  fnid_expr_node* new_fn = new fnid_expr_node();
  new_fn->add_arg($1,$3);
  $$ = new_fn;
  }
| FLOAT {
  fnid_expr_node* new_fn = new fnid_expr_node();
  new_fn->add_arg(new value_node<float>($1));
  $$ = new_fn;
  }
| DOUBLE {
  fnid_expr_node* new_fn = new fnid_expr_node();
  new_fn->add_arg(new value_node<double>($1));
  $$ = new_fn;
  }
| INT {
  fnid_expr_node* new_fn = new fnid_expr_node();
  new_fn->add_arg(new value_node<int>($1));
  $$ = new_fn;
  }
| {
  fnid_expr_node* new_fn = new fnid_expr_node();
  $$ = new_fn;
  }


bdytype :
CONSTANT '(' INT ')' {
  $$ = new bdy_info(B_CONSTANT,$3);
}
| CLAMPED {
  $$ = new bdy_info(B_CLAMPED);
  }
| EXTEND {
  $$ = new bdy_info(B_EXTEND);
  }
| WRAP {
  $$ = new bdy_info(B_WRAP);
  }
| MIRROR {
  $$ = new bdy_info(B_MIRROR);
  }



compose :
compose domainfn '=' vectorexpr ';' {
  $1->add_expr($2,$4);
  $$ = $1;
}
| compose domain '=' vectorexpr ';' {
  $1->add_expr($2,$4);
  $$ = $1;
}
| compose domain '=' INT ';' {
  $1->add_expr($2,new value_node<int>($4));
  $$ = $1;
}
| compose domain '=' FLOAT ';' {
  $1->add_expr($2,new value_node<float>($4));
  $$ = $1;
}
| compose domain '=' DOUBLE ';' {
  $1->add_expr($2,new value_node<double>($4));
  $$ = $1;
}
| { 
  compose_expr_node* new_list = new compose_expr_node(); 
  $$ = new_list;
  }



domainfn:
'@' '[' offsetlist ']' {
  $$ = $3;
}

offsetlist:
offsetlist ',' '(' INT ',' INT ')' {
  $1->add_scale_coeffs($4,$6);
  $$ = $1;
}
| offsetlist ',' '(' '-' INT ',' INT ')' {
  $1->add_scale_coeffs(-$5,$7);
  $$ = $1;
  }
| offsetlist ',' INT {
  $1->add_scale_coeffs($3);
  $$ = $1;
  }
| offsetlist ',' '-' INT {
  $1->add_scale_coeffs(-$4);
  $$ = $1;
  }
| '(' INT ',' INT ')' {
  domainfn_node* new_domainfn = new domainfn_node();
  new_domainfn->add_scale_coeffs($2,$4);
  $$ = new_domainfn;
  }
| '(' '-' INT ',' INT ')' {
  domainfn_node* new_domainfn = new domainfn_node();
  new_domainfn->add_scale_coeffs(-$3,$5);
  $$ = new_domainfn;
  }
| INT {
  domainfn_node* new_domainfn = new domainfn_node();
  new_domainfn->add_scale_coeffs($1);
  $$ = new_domainfn;
  }
| '-' INT {
  domainfn_node* new_domainfn = new domainfn_node();
  new_domainfn->add_scale_coeffs(-$2);
  $$ = new_domainfn;
  }

domain :
'[' rangelist ']' {
  $$ = $2; 
}


rangelist:
rangelist ',' paramexpr DOTDOT paramexpr {
  $1->add_range($3,$5);
  $$ = $1;
}
| rangelist ',' DOTDOT paramexpr {
  $1->add_range(new default_expr(),$4);
  $$ = $1;
  }
| rangelist ',' paramexpr DOTDOT {
  $1->add_range($3,new default_expr());
  $$ = $1;
  }
| rangelist ',' paramexpr {
  $1->add_range($3);
  $$ = $1;
  }
| rangelist ',' DOTDOT {
  $1->add_range(new default_expr(),new default_expr());
  $$ = $1;
  }
| paramexpr DOTDOT paramexpr {
  domain_node* new_domain = new domain_node();
  new_domain->add_range($1,$3);
  $$ = new_domain;
}
| paramexpr DOTDOT {
  domain_node* new_domain = new domain_node();
  new_domain->add_range($1,new default_expr());
  $$ = new_domain;
}
| DOTDOT paramexpr {
  domain_node* new_domain = new domain_node();
  new_domain->add_range(new default_expr(),$2);
  $$ = new_domain;
}
| paramexpr {
  domain_node* new_domain = new domain_node();
  new_domain->add_range($1);
  $$ = new_domain;
  }
| DOTDOT {
  domain_node* new_domain = new domain_node();
  new_domain->add_range(new default_expr(), new default_expr());
  $$ = new_domain;
  }


paramexpr : 
paramexpr '+' paramexpr {
  $$ = $1->add($3);
  delete $3;
}
| paramexpr '-' paramexpr {
  $$ = $1->subtract($3);
  delete $3;
  }
| paramexpr '*' paramexpr {
  $$ = $1->multiply($3);
  delete $3;
  }
| paramexpr '/' INT {
  $$ = $1->divide($3);
  }
/* | MIN '(' paramexpr ',' paramexpr ')' { */
/*   $$ = $3->min($5); */
/*   delete $5; */
/*   } */
/* | MAX '(' paramexpr ',' paramexpr ')' { */
/*   $$ = $3->max($5); */
/*   delete $5; */
/*   } */
/* | CEIL '(' paramexpr ',' INT ')' { */
/*   $$ = $3->ceil($5); */
/*   } */
| INT {
  $$ = new int_expr($1);
  }
| ID {
  parameter_defn* curr_defn = global_params->find_symbol($1);
  if( curr_defn == 0 ){
    fprintf(stderr,"[ME]: ERROR! Unknown parameter :%s at %d.%d-%d.%d\n",$1, yylloc.first_line, yylloc.first_column, yylloc.last_line, yylloc.last_column);
    exit(1);
  }
  $$ = new param_expr(curr_defn);
  free($1);
  }


%%

void parser::set_input_file(FILE* inp_file)
{
  yyin = inp_file;
}

void parser::parse_input()
{
  fn_defs = new fn_defs_table();
  defined_types = new defined_types_table();
  global_params = new param_table;
  do{
    yyparse();
  }while(!feof(yyin));
  printf("Done Parsing\n");
}

void yyerror(const char* s){
  if (yylloc.first_line) {
    printf("%d.%d-%d.%d: ", yylloc.first_line, yylloc.first_column,
           yylloc.last_line, yylloc.last_column);
  }
  printf("[ME] : %s",s);
}
