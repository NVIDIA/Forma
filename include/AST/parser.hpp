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
#ifndef __PARSER_HPP__
#define __PARSER_HPP__

#include <deque>
#include <cstdio>
#include <string>
#include <cstdlib>

#include "AST/stencil_expr.hpp"
#include "AST/vector_stmt.hpp"


/**Class that represents the root node of the AST for the program   
   program ::= fndefns vectordefns stmtseq
*/
class program_node : public ast_node{

private:
  
  ///The base function that contains the body of the program. This has
  ///no return expr set or name
  vectorfn_defn_node* program_body;

public:
  
  ///Constructor
  program_node( vectorfn_defn_node* body){
    program_body = body;
  }

  ///Destructor
  ~program_node(){
    if( fn_defs ){
      delete fn_defs;
    }
    if( defined_types ){
      for( std::deque<std::pair<std::string,defined_data_type*> >::const_iterator it = defined_types->begin(); it != defined_types->end() ; it++ )
	delete it->second;
      delete defined_types;
    }
    if( global_params ){
      for( std::deque<std::pair<std::string,parameter_defn*> >::const_iterator it = global_params->begin() ; it != global_params->end() ; it++ ){
	delete it->second;
      }
      delete global_params;
    }   

    delete program_body;
  }

  ///overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Return the program body
  const vectorfn_defn_node* get_body() const{
    return program_body;
  }

  int compute_pretty_print_size() { return 0; }
  void pretty_print() const;

  // int compute_simpleC_size() { return 0; }
  // void print_simpleC() const;

  ///Compute the domain of the return expr
  inline void compute_domain() { 
    program_body->compute_domain();
  }
  const domain_node* get_expr_domain(){
    return program_body->get_expr_domain();
  }

  template<typename State> friend class ASTVisitor;
};


/**
   Static class to interface with the parser
*/
class parser{
public:
  ///The root node of the program
  static program_node * root_node;

  ///Function to set the input file to read from
  static void set_input_file(FILE*);

  ///Function to start the parsing
  static void parse_input();
};


#endif
