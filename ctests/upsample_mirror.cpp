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
#include <cstdlib>
#include <cstring>

#define LARGE

extern "C" void upsample_mirror(float* input, int height, int width, float* output);

void upsample_gold(float* input, int height, int width, float* output_gold)
{
  for( int i = 0 ; i < height ; i++ )
    for( int j = 0 ; j < width ; j++ ){
      output_gold[2*i*width+j] = 0.3125f * input[((i-1) > 0 ? (i-1) : -(i-1))*width+j] +
        0.625f * input[i*width+j] + 0.0625f * input[((i+1) > height-1 ? 2*(height-1)-(i+1) : (i+1))*width+j];
    }
  for( int i = 0 ; i < height ; i++ )
    for( int j = 0 ; j < width ; j++ ){
      output_gold[(2*i+1)*width+j] = 0.0625 * input[((i-1) > 0 ? (i-1) : -(i-1))*width+j] +
        0.625f * input[i*width+j] + 0.3125f * input[((i+1) > height-1 ? 2*(height-1)-(i+1) : (i+1))*width+j];
    }
}

void check_error(float* input, int height, int width, float* output)
{
  float* output_gold = new float[2*height*width];
  memset(output_gold,0,sizeof(float)*2*width*height);
  upsample_gold(input,height,width,output_gold);
#ifndef LARGE
  printf("Output Gold:\n");
  for( int i = 0 ; i < height*2 ; i++ ){
    for( int j = 0 ; j < width ; j++ )
      printf(" %f",output_gold[i*width+j]);
    printf("\n");
  }
#endif
  double error = 0.0;
  double maxerror = 0.0;
  int maxi = 0;
  int maxj = 0;
  for( int i = 0 ; i < 2*(height) ; i++ )
    for( int j = 0 ; j < width  ; j++ ){
      double curr_error = output_gold[i*width+j] - output[i*width+j];
      error += curr_error*curr_error;
      double abs_error = (curr_error > 0 ? curr_error : -curr_error);
      if( abs_error > maxerror ){
        maxerror = abs_error;
        maxi = i;
        maxj = j;
      }
    }
  error /= (width)*(2*height);
  printf
    ("Average Error : %lf, Max Error : %lf at (%d,%d)\n",
     error,maxerror,maxi,maxj);
  delete[] output_gold;
}


int main()
{
  int width = 1200;
  int height = 1000;

  float * input = new float[width*height];
  for( int i = 0 ; i < height ; i++)
    for( int j = 0 ; j < width ; j++ ){
      input[i*width+j] = (rand()%256)/ 5.0;
    }
  float * output = new float[width*height*2];
  memset(output,0,sizeof(float)*width*height*2);

  upsample_mirror(input,height,width,output);
#ifndef LARGE
  printf("Input :\n");
  for( int i = 0 ; i < height ; i++ ){
    for( int j = 0 ; j < width ; j++ )
      printf(" %f",input[i*width+j]);
    printf("\n");
  }
  printf("Output :\n");
  for( int i = 0 ; i < height*2 ; i++ ){
    for( int j = 0 ; j < width ; j++ )
      printf(" %f",output[i*width+j]);
    printf("\n");
  }
#endif

  check_error(input,height,width,output);

  delete[] input;
  delete[] output;
}
