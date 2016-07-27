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
#include "program_opts.hpp"
#include <cstdlib>

using namespace std;

void program_options::parse_options(int argc, char** argv)
{
  int inp_file_num = 0;

  string enable_unroll("--unroll-loops");
  string enable_forwarding("--forward-exprs");
  string enable_inline_vectorfn("--inline-vector-functions");

  string enable_pretty_print("--pretty-print");

  string set_kernel_name("--kernel-name");
  string set_header_file("--header-file");
  string enable_init_zero("--init-zero");
  string enable_single_malloc("--use-single-malloc");
  string set_unroll_factors("--unroll-factors");

  string enable_dot("--print-dot");
  string set_dot_output_file("--dot-output");

  string enable_c("--print-c");
  string enable_affine("--generate-affine");
  string enable_sequential("--disable-openmp");
  string enable_icc_vecflags("--enable-icc-pragmas");
  string set_c_output_file("--c-output");

  string enable_cuda("--print-cuda");
  string enable_texture("--use-texture");
  string disable_syncthreads("--disable-syncthreads");
  string set_cuda_output_file("--cuda-output");

  string enable_llvm("--print-llvm");
  string set_llvm_file_name("--llvm-output");

  string help("--help");
  for( int i = 1 ; i < argc ; i++ ){

    /// pipeline optimizations
    if( enable_unroll.compare(argv[i]) == 0 ){
      unroll_loops = true;
      continue;
    }
    else if( enable_forwarding.compare(argv[i]) == 0 ){
      forward_exprs = true;
      continue;
    }
    else if( enable_inline_vectorfn.compare(argv[i]) == 0 ){
      inline_vectorfn = true;
      continue;
    }

    /// generic code-gen options
    else if( set_kernel_name.compare(argv[i]) == 0 ){
      if( i == argc - 1 ) {
        printf
          ("Missing kernel name after %s, using default : __forma_kernel__\n",
           set_kernel_name.c_str());
      }
      else
        kernel_name = argv[++i];
      continue;
      //printf("Name : %s\n",kernel_name.c_str());
    }
    else if( set_header_file.compare(argv[i]) == 0 ){
      if( i == argc - 1 ){
        printf
          ("Missing header file name after %s, using default : forma.h\n",
           set_header_file.c_str());
      }
      else
        header_file_name = argv[++i];
      continue;
    }
    else if( enable_init_zero.compare(argv[i]) == 0 ){
      init_zero = true;
      continue;
    }

    ///Pretty print
    else if( enable_pretty_print.compare(argv[i]) == 0 ){
      pretty_print = true;
      continue;
    }

    /// dot print options
    else if( enable_dot.compare(argv[i]) == 0 ){
      print_dot = true;
      continue;
    }
    else if( set_dot_output_file.compare(argv[i]) == 0 ){
      if( i == argc - 1 ) {
        printf
          ("Missing output file name after %s, using default <inp_file>.dot\n",
           set_dot_output_file.c_str());
      }
      else
        dot_file_name = argv[++i];
      //printf("Dot Output File : %s\n", dot_file_name.c_str());
      continue;
    }

    /// C codegen options
    else if( enable_c.compare(argv[i]) == 0 ){
      print_c = true;
      continue;
    }
    else if( enable_affine.compare(argv[i]) == 0 ){
      generate_affine = true;
      use_openmp = false;
      continue;
    }
    else if( enable_sequential.compare(argv[i]) == 0 ){
      use_openmp = false;
      continue;
    }
    else if( enable_icc_vecflags.compare(argv[i]) == 0 ){
      use_icc_pragmas = true;
      continue;
    }
    else if ( enable_single_malloc.compare(argv[i]) == 0 ){
      use_single_malloc = true;
      continue;
    }
    else if ( set_unroll_factors.compare(argv[i]) == 0 ){
      if (i == argc - 1) {
        printf
          ("Missing unroll factors after %s, disabling unroll\n",
           set_unroll_factors.c_str());
      }
      else {
        generate_unroll_code = true;
        std::string factor_list = argv[++i];
        size_t start = 0;
        while (start < factor_list.size()) {
          size_t end = factor_list.find_first_of(",", start);
          int substr_len =
            (end == string::npos ? factor_list.size() : end) - start;
          int factor = atoi(factor_list.substr(start, substr_len).c_str());
          unroll_factors.push_back(factor);
          start += substr_len+1;
        }
      }
      continue;
    }
    else if( set_c_output_file.compare(argv[i]) == 0 ){
      if( i == argc - 1 ) {
        printf
          ("Missing output file name after %s, using default <inp_file>.c\n"
           ,set_c_output_file.c_str());
      }
      else
        c_output_file = argv[++i];
      printf("C Output File : %s\n", c_output_file.c_str());
      continue;
    }

    ///CUDA codegen options
    else if( enable_cuda.compare(argv[i]) == 0 ){
      print_cuda = true;
      continue;
    }
    else if( enable_texture.compare(argv[i]) == 0 ){
      use_texture = true;
      continue;
    }
    else if( disable_syncthreads.compare(argv[i]) == 0 ){
      use_syncthreads = false;
      continue;
    }
    else if( set_cuda_output_file.compare(argv[i]) == 0 ){
      if( i == argc - 1 ) {
        printf
          ("Missing output file name after %s, using default <inp_file>.cu\n",
           set_cuda_output_file.c_str());
      }
      else
        cuda_output_file = argv[++i];
      printf("Cuda Output File : %s\n", cuda_output_file.c_str());
      continue;
    }

    /// LLVM codegen options
    else if (enable_llvm.compare(argv[i]) == 0){
      print_llvm = true;
      continue;
    }
    else if( set_llvm_file_name.compare(argv[i]) == 0 ){
      if( i == argc - 1 ) {
        printf
          ("Missing output file name after %s, using default <inp_file>.ll\n",
           set_llvm_file_name.c_str());
      }
      else
        llvm_file_name = argv[++i];
      printf("LLVM Output File : %s\n", llvm_file_name.c_str());
      continue;
    }

    else if( help.compare(argv[i]) == 0 ){
      printf("Forma program information :\n");
      printf("%s : Pretty Print the input code\n",enable_pretty_print.c_str());
      printf("%s : Print Dot version of AST\n",enable_dot.c_str());
      printf
        ("%s <name> : Specify output file name for generated dot output code, "
         "valid only with %s\n",set_dot_output_file.c_str(),enable_dot.c_str());
      printf("\n");

      printf("Generic code-generation options :\n");
      printf
        ("%s <name> : Specify name for the kernel\n",set_kernel_name.c_str());
      printf
        ("%s <name> : Name of the file where the C-sginature of the generated "
         "Forma code is to be printed\n",set_header_file.c_str());
      printf
        ("%s : Initialize all buffers to zero on allocation"
         " [default:disabled]\n",enable_init_zero.c_str());
      printf
        ("%s : Use a single malloc to allocate all required memory\n",
         enable_single_malloc.c_str());
      printf
        ("%s <integer_list> : Specify a list of unroll factors to use for the "
         "different loop nests. <integer_list> is a comma-separated list of "
         "integers with the unroll factor specified from innermost dimension to"
         " outermost dimension\n", set_unroll_factors.c_str());
      printf("\n");

      printf("C code-generation options :\n");
      printf("%s : Print C code\n",enable_c.c_str());
      // printf("--generate-affine : Generate code that can be parsed by a
      // polyhedral compiler, disables OpenMP\n");
      printf
        ("%s : Generate sequential C code (default uses OpenMP)\n",
         enable_sequential.c_str());
      printf
        ("%s : Generate ICC pragmas for vectorization\n",
         enable_icc_vecflags.c_str());
      printf
        ("%s <name> : Specify output file name for generated C code\n",
         set_c_output_file.c_str());
      printf("\n");

      printf("Cuda code-generation options :\n");
      printf("%s : Print Cuda code\n",enable_cuda.c_str());
      printf
        ("%s : Use texture references wherever possible [default:disabled]\n",
         enable_texture.c_str());
      printf
        ("%s <name> : Specify output file name for generated CUDA code\n",
         set_cuda_output_file.c_str());
      printf("\n");

      printf("LLVM code-generation options :\n");
      printf("%s : Generate LLVM code\n",enable_llvm.c_str());
      printf
        ("%s <name> : Specify output file name for generated LLVM IR\n",
         set_llvm_file_name.c_str());
      printf("\n");
      exit(0);
    }
    else if( inp_file_num == 0 ){
      inp_file_num = i;
      continue;
    }
    else{
      printf("Unknown option %s\n",argv[i]);
      exit(1);
    }
  }


  if( inp_file_num == 0 ){
    fprintf(stderr,"[ME] : Input format %s <opts> <file>\n",argv[0]);
    exit(1);
  }
  inp_file = fopen(argv[inp_file_num],"r");

  if( kernel_name.size() == 0 ){
    kernel_name = "__forma_kernel__";
  }

  if( header_file_name.size() == 0 ){
    header_file_name = "forma.h";
  }

  if(print_dot ){
    if (dot_file_name.size() == 0 ){
      dot_file_name = argv[inp_file_num];
      dot_file_name.append(".dot");
    }
  }

  if( print_c ){
    if (c_output_file.size() == 0) {
      c_output_file = argv[inp_file_num];
      c_output_file.append(".c");
    }
  }

  if( print_cuda) {
    if (cuda_output_file.size() == 0) {
      cuda_output_file = argv[inp_file_num];
      cuda_output_file.append(".cu");
    }
  }

  if( print_llvm ){
    if( llvm_file_name.size() == 0 ){
      llvm_file_name = argv[inp_file_num];
      llvm_file_name.append(".ll");
    }
  }

}


