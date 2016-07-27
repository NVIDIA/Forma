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
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include "cuda_runtime.h"

typedef std::chrono::high_resolution_clock HighResolutionClock;
typedef std::chrono::milliseconds milliseconds;

#ifndef _NTRIALS_
#define _NTRIALS_ 5
#endif

struct rgb{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

extern "C" void hdr_pyramid(struct rgb* input1, struct rgb* input2, struct rgb* input3, struct rgb* input4, struct rgb* output);

int main()
{
  int width = 752;
  int height = 500;
  
  rgb * input1 = new rgb[width*height];
  rgb * input2 = new rgb[width*height];
  rgb * input3 = new rgb[width*height];
  rgb * input4 = new rgb[width*height];
  rgb * output = new rgb[width*height];
  rgb * output_gpu = new rgb[width*height];
  rgb * output_texture = new rgb[width*height];
  
  bool useDevMem = false;
  if (const char *var = getenv("FORMA_USE_DEVICE_MEMORY")) {
    useDevMem = (atoi(var) != 0);
  }
  
  struct rgb* inputPtr1 = input1;
  struct rgb* inputPtr2 = input2;
  struct rgb* inputPtr3 = input3;
  struct rgb* inputPtr4 = input4;
  if (useDevMem) {
    cudaMalloc(&inputPtr1, sizeof(struct rgb)*width*height);
    cudaMemcpy(inputPtr1, input1, sizeof(struct rgb)*width*height, cudaMemcpyHostToDevice);
    cudaMalloc(&inputPtr2, sizeof(struct rgb)*width*height);
    cudaMemcpy(inputPtr2, input2, sizeof(struct rgb)*width*height, cudaMemcpyHostToDevice);
    cudaMalloc(&inputPtr3, sizeof(struct rgb)*width*height);
    cudaMemcpy(inputPtr3, input3, sizeof(struct rgb)*width*height, cudaMemcpyHostToDevice);
    cudaMalloc(&inputPtr4, sizeof(struct rgb)*width*height);
    cudaMemcpy(inputPtr4, input4, sizeof(struct rgb)*width*height, cudaMemcpyHostToDevice);
  }

  for( int i = 0 ; i < height ; i++) 
    for( int j = 0 ; j < width ; j++ ){
      input1[i*width+j].r = (rand()%256);
      input1[i*width+j].g = (rand()%256);
      input1[i*width+j].b = (rand()%256);
      input2[i*width+j].r = (rand()%256);
      input2[i*width+j].g = (rand()%256);
      input2[i*width+j].b = (rand()%256);
      input3[i*width+j].r = (rand()%256);
      input3[i*width+j].g = (rand()%256);
      input3[i*width+j].b = (rand()%256);
      input4[i*width+j].r = (rand()%256);
      input4[i*width+j].g = (rand()%256);
      input4[i*width+j].b = (rand()%256);
    }

  milliseconds time(0);

  ///Warmup
  hdr_pyramid(inputPtr1,inputPtr2,inputPtr3,inputPtr4,output);

  for( int i = 0 ; i < 5 ; i++ ){
    auto start = HighResolutionClock::now();
    hdr_pyramid(inputPtr1,inputPtr2,inputPtr3,inputPtr4,output);
    auto stop = HighResolutionClock::now();
    time += std::chrono::duration_cast<milliseconds>(stop-start);
  }
  printf("[FORMA] Base CPU : %lf\n",(double)time.count()/_NTRIALS_);

  if( useDevMem ){
    cudaFree(inputPtr1);
    cudaFree(inputPtr2);
    cudaFree(inputPtr3);
    cudaFree(inputPtr4);
  }

  delete[] input1;
  delete[] input2;
  delete[] input3;
  delete[] input4;
  delete[] output;
}
