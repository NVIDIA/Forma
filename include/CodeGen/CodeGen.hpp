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
#ifndef __CODEGEN_HPP__
#define __CODEGEN_HPP__

#include "program_opts.hpp"
#include <sstream>
#include "AST/parser.hpp"

class program_node;


struct stringBuffer{
  std::stringstream buffer;
  int ntabs;
  stringBuffer(int nt = 0) : ntabs(nt) { }
  inline void increaseIndent(){
    ntabs+=2;
  }
  inline void decreaseIndent(){
    ntabs-=2;
  }
  inline void indent(){
    for(int i = 0 ; i < ntabs ; i++ )
      buffer << " ";
  }
  inline void newline() {
    buffer << "\n";
  }
  inline void newlineIndent(){
    newline();
    indent();
  }
  inline int getIndent() const { return ntabs;}
};


class CodeGen {
  
public:
  
  CodeGen() { }

  virtual void GenerateCode(const program_node* program, const program_options& opts)=0;
  
  virtual ~CodeGen() { }

  void PrintCHeader(const program_node* program, const program_options& opts);

  /// \brief computeOffsets Computes the offset to the loop bounds to get to the
  /// interiors of the computation, i.e., where boundary conditions need not be
  /// applied
  /// \param fn The function application expression being evaluated
  /// \param offsets The offset hull that is computed by this routine
  /// \param outputIdxInfo Scale domain used on the LHS(affects the loop bounds)
  /// \param considerBdyInfo When set to true it evaluates the offsets to be
  /// used for the entire computations, When false it computes offsets to be
  /// used for the interior of the computation
  virtual void computeOffsets
  (const fnid_expr_node* fn, std::deque<offset_hull>& offsets,
   const domainfn_node* outputIdxInfo, bool considerBdyInfo = false) const;

  /// Function to apply an offset to a domain to ignore the boundaries
  void ApplyOffset
  (domain_node* loop_domain, const std::deque<offset_hull>& boundary_offsets)
    const;

protected:

  void PrintCDomainSize(const domain_node* curr_domain, std::stringstream& stream);

  void PrintCParametricExpr(const parametric_exp* curr_expr, std::stringstream& stream);

  void PrintCStructDefinition
  (stringBuffer& curr_buffer, bool define_cuda_structs = true);

  void PrintCParametricDefines(stringBuffer& curr_buffer);
};


#endif
