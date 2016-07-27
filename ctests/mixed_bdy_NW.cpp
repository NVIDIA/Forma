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
#include <cassert>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

#define N 10
#define M 12

extern "C"
void mixed_bdy_NW(float*, float*, int, int, float*);

// extern "C"
// void *__formart_alloc(size_t size) {
//   return new char[size];
// }

void ref_output(float * input1, float* input2, float * output)
{
  for( int i = 3 ; i < M - 2 ; i++ )
    for( int j = 2 ; j < N - 3; j++ ){
      int i_index_1,j_index_1,i_index_2,j_index_2;
      i_index_1 = i-2;
      j_index_1 = j-3;
      i_index_2 = i+3;
      j_index_2 = j+2;
      
      if( i_index_2 >= M )
	i_index_2 -= M;
      if( j_index_1 < 0 )
	j_index_1 += N;

      output[i*N+j] = input1[(i-3)*N+j-2] + input1[(i+2)*N+j+3]
	+ input2[(i_index_1)*N+j_index_1] + input2[(i_index_2)*N+j_index_2]; 
    }
}


int main() {
  float* inp1 = new float[M*N];
  float* inp2 = new float[M*N];
  float* outp = new float[M*N];
  float* outp_ref = new float[M*N];

  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ ){
      inp1[i*N+j] = (float)rand() / (float)RAND_MAX;
      inp2[i*N+j] = (float)rand() / (float)RAND_MAX;
      outp[i*N+j] = 0.0;
      outp_ref[i*N+j] = 0.0;
    }

  mixed_bdy_NW(inp1,inp2,M,N,outp);
  ref_output(inp1,inp2,outp_ref);

  printf("Output :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %f", outp[i*N+j]);
    printf("\n");
  }
  printf("Output-Ref :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %f", outp_ref[i*N+j]);
    printf("\n");
  }
  double diff = 0.0;
  for( int i = 0 ; i < M ; i++) 
    for( int j = 0 ; j < N ; j++ )
      diff += fabs(outp_ref[i*N+j] - outp[i*N+j]);
  printf("Diff : %f\n",diff);
  if (diff != 0.0){
    printf("Incorrect Result\n");
    exit(1);
  }
  delete[] inp1;
  delete[] inp2;
  delete[] outp;
  delete[] outp_ref;
}
