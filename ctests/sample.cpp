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

extern "C" void sample(double*,int,int,double*);

// extern "C"
// void *__formart_alloc(size_t size) {
//   return new char[size];
// }

void ref_output(double* input, double * output)
{
  for( int i = 1; i < M/2 ; i++ ) 
    for( int j = 0 ; j < N/3-1 ; j++ )
      output[i*(N/3)+j] = input[(2*i-1)*N+3*j+5];
}


int main(int argc, char** argv)
{
  double * input = new double[M*N];
  double * output = new double[(M/2)*(N/3)];
  double * output_ref = new double[(M/2)*(N/3)];

  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ )
      input[i*N+j] = (double)(rand()) / (double)(RAND_MAX-1);
      
  for( int i = 0 ; i < M/2 ; i++ ) 
    for( int j = 0 ; j < N/3 ; j++ ){
      output[i*(N/3)+j] = 0.0;
      output_ref[i*(N/3)+j] = 0.0;
    }

  sample(input,M,N,output);
  ref_output(input,output_ref);
  
  printf("Output[%d,%d] :\n",M/2,N/3);
  for( int i = 0 ; i < M/2 ; i++ ){
    for( int j = 0 ; j < N/3 ; j++ )
      printf(" %f",output[i*(N/3)+j]);
    printf("\n");
  }
  printf("OutputRef[%d,%d] :\n",M/2,N/3);
  for( int i = 0 ; i < M/2 ; i++ ){
    for( int j = 0 ; j < N/3 ; j++ )
      printf(" %f",output_ref[i*(N/3)+j]);
    printf("\n");
  }
  double diff = 0.0;
  for( int i = 0 ; i < M/2 ; i++ )
    for( int j = 0 ; j < N/3 ; j++ )
      diff += fabs(output_ref[i*(N/3)+j] - output[i*(N/3)+j]);
  printf("Diff : %f\n",diff);
  if( diff != 0 ){
    printf("Incorrect Result\n");
    exit(1);
  }

  delete[] input;
  delete[] output;
  delete[] output_ref;
  return 0;
}
