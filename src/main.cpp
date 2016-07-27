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
#include <cstdio>
#include "AST/parser.hpp"
#include "CodeGen/print_C.hpp"
#include "CodeGen/print_dot.hpp"
#include "CodeGen/print_CUDA.hpp"
#include "CodeGen/LLVM/CGModule.h"
#include "program_opts.hpp"
#include "ASTVisitor/unroll.hpp"
#include "ASTVisitor/forward_expr.hpp"
#include "ASTVisitor/inline_vectorfn.hpp"

using namespace std;

program_node* parser::root_node = NULL;

void print_header(const program_node* curr_program);

int main(int argc, char ** argv)
{
  program_options mode;
  mode.parse_options(argc,argv);

  parser::set_input_file(mode.inp_file);
  parser::parse_input();

  if( parser::root_node ){

    ///Optimization passes. They are ordered here
    if( mode.unroll_loops ){
      LoopUnroll unroll_for_loops;
      printf("Loop Unroll ...");
      unroll_for_loops.visit(parser::root_node,0);
      printf(" done\n");
    }
    if( mode.forward_exprs || mode.inline_vectorfn ){
      ForwardExpr forward_single_uses;
      printf("Forward expressions ...");
      forward_single_uses.visit(parser::root_node,NULL);
      printf(" done\n");
    }
    if( mode.inline_vectorfn ){
      InlineVectorfn inline_vectorfns;
      printf("Inline Vector Functions ...");
      inline_vectorfns.visit(parser::root_node,NULL);
      printf(" done\n");
    }

    ///Codegen Passes
    if( mode.pretty_print ){
      parser::root_node->pretty_print();
      printf("Done Pretty print\n");
    }
    if( mode.print_dot ){
      PrintDot dot_printer;
      dot_printer.GenerateCode(parser::root_node,mode);
      printf("Done Dot print\n");
    }
    if( mode.print_c ){
      parser::root_node->compute_domain();
      PrintC my_printer; // = PrintC::instance();
      my_printer.PrintCHeader(parser::root_node,mode);
      my_printer.GenerateCode(parser::root_node,mode);
      //delete my_printer;
      printf("Done C print\n");
    }
    if( mode.print_cuda) {
      parser::root_node->compute_domain();
      PrintCUDA cuda_printer;
      cuda_printer.PrintCHeader(parser::root_node,mode);
      cuda_printer.GenerateCode(parser::root_node,mode);
      printf("Done CUDA print\n");
    }
    if (mode.print_llvm) {
#ifdef FORMA_USE_LLVM
      parser::root_node->compute_domain();
      CGModule codegen;
      codegen.PrintCHeader(parser::root_node,mode);
      codegen.GenerateCode(parser::root_node,mode);
      printf("Done LLVM CodeGen\n");
#else
      printf("LLVM support required, but not compiled in\n");
#endif
    }

    delete parser::root_node;
    parser::root_node = NULL;
  }
  else
    printf("[ME] No rootnode!\n");

  return 0;
}

