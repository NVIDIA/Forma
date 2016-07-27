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
/*
 *  Copyright 2014 NVIDIA Corporation.  All rights reserved.
 *
 *  NOTICE TO USER: The source code, and related code and software
 *  ("Code"), is copyrighted under U.S. and international laws.
 *
 *  NVIDIA Corporation owns the copyright and any patents issued or
 *  pending for the Code.
 *
 *  NVIDIA CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY
 *  OF THIS CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS-IS" WITHOUT EXPRESS
 *  OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA CORPORATION DISCLAIMS ALL
 *  WARRANTIES WITH REGARD TO THE CODE, INCLUDING NON-INFRINGEMENT, AND
 *  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE.  IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *  WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATED TO THE USE OR
 *  PERFORMANCE OF THE CODE, INCLUDING, BUT NOT LIMITED TO, INFRINGEMENT,
 *  LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 *  NEGLIGENCE OR OTHER TORTIOUS ACTION, AND WHETHER OR NOT THE
 *  POSSIBILITY OF SUCH DAMAGES WERE KNOWN OR MADE KNOWN TO NVIDIA
 *  CORPORATION.
 */
#include <cstdlib>
#include <cstdio>
#include <cmath>

#define M 11
#define N 14

extern "C" void compose(double*,int,int,double*);

// extern "C"
// void *__formart_alloc(size_t size) {
//   return new char[size];
// }

void ref_upx_even(double (*input)[N], double (*output)[N])
{
  for( int i = 1; i < M ; i++ ) 
    for( int j = 0 ; j < N ; j++ )
      output[2*i][j] = input[i-1][j] * 0.25 + input[i][j] * 0.75;
}
void ref_upx_odd(double (*input)[N], double (*output)[N])
{
  for( int i = 0; i < M-1 ; i++ ) 
    for( int j = 0 ; j < N ; j++ )
      output[2*i+1][j] = input[i+1][j] * 0.25 + input[i][j] * 0.75;
}
void ref_output(double (*input)[N], double (*output)[N])
{
  ref_upx_even(input,output);
  ref_upx_odd(input,output);
}


int main(int argc, char** argv)
{
  double (*input)[N] = (double (*)[N])new double[M*N];
  double (*output)[N] = (double (*)[N])new double[2*M*N];
  double (*output_ref)[N] = (double (*)[N])new double[2*M*N];

  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ )
      input[i][j] = (double)(rand()) / (double)(RAND_MAX-1);
      
  for( int i = 0 ; i < 2*M ; i++ ) 
    for( int j = 0 ; j < N ; j++ ){
      output[i][j] = 0.0;
      output_ref[i][j] = 0.0;
    }

  compose((double*)input,M,N,(double*)output);
  ref_output(input,output_ref);
  
  printf("Input :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %f",input[i][j]);
    printf("\n");
  }
  printf("Output[%d,%d] :\n",2*M,N);
  for( int i = 0 ; i < 2*M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %f",output[i][j]);
    printf("\n");
  }
  printf("OutputRef[%d,%d] :\n",2*M,N);
  for( int i = 0 ; i < 2*M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %f",output_ref[i][j]);
    printf("\n");
  }
  double diff = 0.0;
  for( int i = 0 ; i < 2*M ; i++ )
    for( int j = 0 ; j < N ; j++ )
      diff += fabs(output_ref[i][j] - output[i][j]);
  printf("Diff : %e\n",diff);
  if( diff > 1e-6 ){
    printf("Incorrect result\n");
    exit(1);
  }
  delete[] input;
  delete[] output;
  delete[] output_ref;
  return 0;
}
