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

typedef std::chrono::high_resolution_clock HighResolutionClock;
typedef std::chrono::milliseconds milliseconds;

#ifndef _NTRIALS_
#define _NTRIALS_ 5
#endif

struct rgb8{
  unsigned char r;
  unsigned char g;
  unsigned char b;
};


extern "C" void camera_pipe(short * input, float * m3200, float* m7000, float color_temp, float gamma, float contrast,  rgb8 * output);

int main()
{
  int width = 2592;
  int height = 1968;

  height -= 10;
  width -= 14;
  short * input = new short[width*height];
  rgb8 * output = new rgb8[width*height];

  for( int i = 0 ; i < height ; i++)
      for( int j = 0 ; j < width ; j++ ){
	short val = (rand() % 256) / 5.0;
	if (i >= 10 && j >= 14)
	  input[(i-10)*width+j-14] = val;
    }

  milliseconds time(0);

  float color_temp = 3200.0f;
  float gamma = 1.8f;
  float contrast = 10.0f;
  // These color matrices are for the sensor in the Nokia N900 and are
  // taken from the FCam source.
  float _matrix_3200[][4] = {{ 1.6697f, -0.2693f, -0.4004f, -42.4346f},
			     {-0.3576f,  1.0615f,  1.5949f, -37.1158f},
			     {-0.2175f, -1.8751f,  6.9640f, -26.6970f}};
    
  float _matrix_7000[][4] = {{ 2.2997f, -0.4478f,  0.1706f, -39.0923f},
			     {-0.3826f,  1.5906f, -0.2080f, -25.4311f},
			     {-0.0888f, -0.7344f,  2.2832f, -20.0826f}};

  ///Warmup
  camera_pipe(input,(float*)_matrix_3200,(float*)_matrix_7000,color_temp,gamma,contrast,output);

  for( int i = 0 ; i < _NTRIALS_ ; i++ ){
    auto start = HighResolutionClock::now();
    camera_pipe(input,(float*)_matrix_3200,(float*)_matrix_7000,color_temp,gamma,contrast,output);
    auto stop = HighResolutionClock::now();
    time += std::chrono::duration_cast<milliseconds>(stop-start);
  }
  printf("[FORMA] Total Time : %lf\n",(double)time.count()/_NTRIALS_);

  delete[] input;
  delete[] output;
}
