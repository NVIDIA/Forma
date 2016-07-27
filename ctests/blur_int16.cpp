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

#define N 1000
#define M 1200

extern "C" void blur_int16(short *, int, int, short*);

void blur_ref(short (*input)[N], short (*output)[N])
{
  short (*by)[N] = (short (*)[N])new short[M*N];
  memset((short*)by,0,sizeof(short)*M*N);
  for( int i = 1 ; i < M-1 ; i++ )
    for( int j = 0 ; j < N ; j++ )
      by[i][j] = (input[i-1][j] + input[i][j] + input[i+1][j]) / 3.0;
  for( int i = 0 ; i < M ; i++ )
    for( int j = 1 ; j < N-1 ; j++ )
      output[i][j] = (by[i][j-1] + by[i][j] + by[i][j+1])/ 3.0;
  delete[] by;
}

int main(int argc, char** argv)
{
  short (*input)[N]  = (short (*)[N])new short[M*N];
  short (*output)[N]  = (short (*)[N])new short[M*N];
  short (*output_ref)[N]  = (short (*)[N])new short[M*N];

  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ ){
      input[i][j] = (rand()) % 32768;
      output[i][j] = 0;
      output_ref[i][j] = 0;
    }

  blur_int16((short*)input,M,N,(short*)output);
  blur_ref(input,output_ref);

#ifdef PRINT_OUTPUT
  printf("Input :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %u",input[i][j]);
    printf("\n");
  }
  printf("Output :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %u",output[i][j]);
    printf("\n");
  }
  printf("Output_Ref :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %u",output_ref[i][j]);
    printf("\n");
  }
#endif
  double diff = 0.0;
  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ )
      diff += ( output_ref[i][j] > output[i][j] ? 
		output_ref[i][j] - output[i][j] : 
		output[i][j] - output_ref[i][j] );
  
  printf("Diff : %lf\n",diff);
  if( diff > 1e-6 ){
    printf("Incorrect Result\n");
    exit(1);
  }

  delete[] input;
  delete[] output;
  delete[] output_ref;

  return 0;
}

