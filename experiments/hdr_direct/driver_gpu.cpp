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
  float r;
  float g;
  float b;
};

#define absd(a) ( (a) > 0 ? (a) : (-(a)) )
extern void hdr_direct_gold(const rgb*, const rgb*, const rgb*, const rgb* , int,int, rgb*);

void check_error(const rgb* image1, const rgb* image2, const rgb* image3, const rgb* image4 , int height ,int width, const rgb* output)
{
  rgb* output_gold = new rgb[height*width];
  hdr_direct_gold(image1,image2,image3,image4,height,width,output_gold);
  double max_error=0.0,avg_error=0.0;
  int max_error_j=-1,max_error_i=-1,max_error_c=-1;
  
  for( int i = 1 ; i < height-1 ; i++ )
    for( int j = 1 ; j < width-1 ; j++ ) {
      double curr_error = absd(output_gold[i*width+j].r - output[i*width+j].r);
      if( curr_error > max_error ){
	max_error_j = j;
	max_error_i = i;
	max_error_c = 0;
	max_error = curr_error;
      }
      avg_error += curr_error;
      curr_error = absd(output_gold[i*width+j].g - output[i*width+j].g );
      if( curr_error > max_error ){
      	max_error_j = j;
      	max_error_i = i;
      	max_error_c = 1;
      	max_error = curr_error;
      }
      avg_error += curr_error;
      curr_error = absd(output_gold[i*width+j].b - output[i*width+j].b );
      if( curr_error > max_error ){
      	max_error_j = j;
      	max_error_i = i;
      	max_error_c = 2;
      	max_error = curr_error;
      }
      avg_error += curr_error;
    }
  avg_error /= ((width-2)*(height-2)*3);
  printf("Max Error : %lf at (%d,%d,%d), Average Error : %lf\n",max_error,max_error_i,max_error_j,max_error_c,avg_error);

  delete[] output_gold;
}

extern "C" void hdr_direct(rgb* input1, rgb* input2, rgb* input3, rgb* input4, int height, int width, rgb* output);

int main()
{
  int width = 752;
  int height = 500;
  
  rgb * input1 = new rgb[width*height];
  rgb * input2 = new rgb[width*height];
  rgb * input3 = new rgb[width*height];
  rgb * input4 = new rgb[width*height];
  rgb * output = new rgb[width*height];
  

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

  ///Warmup
  hdr_direct(inputPtr1,inputPtr2,inputPtr3,inputPtr4,height,width,output);
  check_error(input1,input2,input3,input4,height,width,output);

  for( int i = 0 ; i < _NTRIALS_ ; i++ ){
    auto start = HighResolutionClock::now();
    hdr_direct(inputPtr1,inputPtr2,inputPtr3,inputPtr4,height,width,output);
    auto stop = HighResolutionClock::now();
    time += std::chrono::duration_cast<milliseconds>(stop-start);
  }
  printf("[FORMA] Total Time(ms) : %lf\n",(double)time.count()/_NTRIALS_);

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
