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
#ifndef __PRINT_DOT_HPP__
#define __PRINT_DOT_HPP__

#include <cstdio>
#include <sstream>
#include <map>
#include <set>
#include "CodeGen.hpp"

typedef std::map<const vector_expr_node*,int> dot_symbol_table;
typedef std::pair<const vector_expr_node*,int> dot_symbol_entry;

class PrintDot : public CodeGen{

public:

  PrintDot(){
    node_count = 0;
  }

  void GenerateCode(const program_node* curr_program, const program_options& program_opts);

private:

  int node_count;

  std::stringstream output_stream;

  void print(const vectorfn_defn_node*);

  void print(const stmt_node*, dot_symbol_table&);

  void print(const vector_expr_node*, dot_symbol_table&, std::set<int>&);

  void print(const vector_defn_node*, dot_symbol_table&);

  void print(const fnid_expr_node*, dot_symbol_table&, std::set<int>&);
  
  void print(const compose_expr_node*, dot_symbol_table&, std::set<int>&);

  void print(const make_struct_node*, dot_symbol_table&, std::set<int>&);

  void print(const vec_domainfn_node*, dot_symbol_table&, std::set<int>&);

  void print(const stencilfn_defn_node*, std::stringstream& )const;

  void print(const domain_node* , std::stringstream& )const;

  void print(const parametric_exp*, std::stringstream&) const;
  
  void print(const domainfn_node*, std::stringstream& )const;
};


#endif
