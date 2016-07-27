/****************************************************************************/
/* Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.             */
/*                                                                          */
/* Redistribution and use in source and binary forms, with or without       */
/* modification, are permitted provided that the following conditions       */
/* are met:                                                                 */
/*  * Redistributions of source code must retain the above copyright        */
/*    notice, this list of conditions and the following disclaimer.         */
/*  * Redistributions in binary form must reproduce the above copyright     */
/*    notice, this list of conditions and the following disclaimer in the   */
/*    documentation and/or other materials provided with the distribution.  */
/*  * Neither the name of NVIDIA CORPORATION nor the names of its           */
/*    contributors may be used to endorse or promote products derived       */
/*    from this software without specific prior written permission.         */
/*                                                                          */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY     */
/* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE        */
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR       */
/* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR        */
/* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,    */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,      */
/* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR       */
/* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY      */
/* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT             */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     */
/****************************************************************************/
%{
  //#include "tokenizer.hpp"
  //using namespace std;
#define YY_NO_UNISTD_H
  extern "C" int yylex(void);
  #include "parser.tab.h"
  #include "AST/data_types.hpp"

  int yycolumn = 1;
  #define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;\
                         yylloc.first_column = yycolumn; yylloc.last_column = yycolumn+yyleng-1; \
                         yycolumn += yyleng;
%}

%option noyywrap
%option never-interactive
%option yylineno

ID [a-zA-Z][a-zA-Z0-9_]*
DIGIT [0-9]+

%%

[\(\)\[\],=;@\{\}\?:\.] { 
  //printf("Found %c\n",yytext[0]); 
  return yytext[0]; 
}

[\+\-\*/\^<>] { 
  //printf("Found %c\n",yytext[0]);
  return yytext[0]; 
}

".." {
  //printf("DotDot\n"); 
  return DOTDOT; 
}

"<=" {
  return LE;
}

">=" {
  return GE;
}

"==" {
  return EQ;
}

"!=" {
  return NE;
}

{DIGIT}   {  
  yylval.int_val = atoi(yytext); 
  //printf("Integer : %d\n",yylval.int_val); 
  return INT; 
}

{DIGIT}"."{DIGIT}"f" {
  yylval.float_val = atof(yytext);
  //printf("Float : %f\n",yylval.float_val); 
  return FLOAT;
		 }

{DIGIT}"."{DIGIT} {
  yylval.double_val = atof(yytext);
  //printf("Double : %lf\n",yylval.double_val); 
  return DOUBLE;
       }

for {
  //printf("Found Token FOR\n");
  return FOR;
}

endfor {
  return ENDFOR;
}


do {
  //printf("Found DO\n");
  return DO;
 }

 while {
  //printf("Found WHILE\n");
   return WHILE;
 }
  

float {
  yylval.int_val = T_FLOAT;
  return BASICTYPE;
}

double {
  //printf("Keyword : double\n");
  yylval.int_val = T_DOUBLE;
  return BASICTYPE;
}

int {
  yylval.int_val = T_INT;
  return BASICTYPE;
}

int8 {
  yylval.int_val = T_INT8;
  return BASICTYPE;
}

int16 {
  yylval.int_val = T_INT16;
  return BASICTYPE;
}

cast {
  return CAST;
}

struct {
  //printf("STRUCT\n");
  return STRUCT;
}

constant {
  return CONSTANT;
}

clamped {
  return CLAMPED;
}

extend {
  return EXTEND;
}

wrap {
  return WRAP;
}

mirror {
  return MIRROR;
}


vector  {
  //printf("Keywork : vector\n");
  return VECTORFN;
}

parameter {
  return PARAMETER;
}

vector"#" {
  //printf("Keywork : vector#\n");
  return VECTOR;
}

stencil   {
  //printf("Found Stencil\n");
  return STENCIL;
}

return      {
  //printf("Found Return\n");
  return RETURN;
}

{ID} { 
  //printf("Found ID : %s\n",yytext);
  yylval.id_name = strdup(yytext);
  return ID;
}

"//"[^\n]*\n {
  yycolumn = 1;
}

[ \t]+
\n {
  yycolumn = 1;
}

<<EOF>> { return 0; }
%%



	 
