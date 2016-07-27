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
#ifndef __PROGRAM_OPTS_HPP__
#define __PROGRAM_OPTS_HPP__

#include <cstdio>
#include <deque>
#include <string>


struct program_options{
  /// input file
  FILE* inp_file;

  /// Forma optimizations
  bool unroll_loops;
  bool forward_exprs;
  bool inline_vectorfn;

  /// Pretty print
  bool pretty_print;

  /// generic codegen options
  std::string kernel_name;
  std::string header_file_name;
  bool init_zero;
  bool use_single_malloc;
  bool generate_unroll_code;
  std::deque<int> unroll_factors;

  /// Dot code-generation
  bool print_dot;
  std::string dot_file_name;

  /// C code generation options
  bool print_c;
  bool generate_affine;
  bool use_openmp;
  bool use_icc_pragmas;
  std::string c_output_file;

  /// Cuda code generation options
  bool print_cuda;
  bool use_texture;
  bool use_syncthreads;
  std::string cuda_output_file;

  /// LLVM Code generation options
  bool print_llvm;
  std::string llvm_file_name;

  program_options() :
    inp_file(NULL),

    unroll_loops(true),
    forward_exprs(false),
    inline_vectorfn(false),

    pretty_print(false),

    kernel_name(""),
    header_file_name(""),
    init_zero(false),
    use_single_malloc(false),
    generate_unroll_code(false),

    print_dot(false),
    dot_file_name(""),

    print_c(false),
    generate_affine(false),
    use_openmp(true),
    use_icc_pragmas(false),
    c_output_file(""),

    print_cuda(false),
    use_texture(false),
    use_syncthreads(true),
    cuda_output_file(""),

    print_llvm(false),
    llvm_file_name("")
  {  }
  ~program_options(){  }
  void parse_options(int,char**);
};

#endif
