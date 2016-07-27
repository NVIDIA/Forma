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

struct rgb{
  double r;
  double g;
  double b;
};

extern "C"
void hdr_direct(rgb*,rgb*, int, int, rgb*);

void compute_weights(const rgb (*image)[N], double (*weight)[N])
{
  double (*make_grey)[N] = (double (*)[N])new double[M*N];
  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ ){
      make_grey[i][j] = (0.02126 * image[i][j].r + 0.7152 * image[i][j].g + 0.0722 * image[i][j].b) / 255;
    }
  for( int i = 1 ; i < M-1 ; i++ )
    for( int j = 1 ; j < N-1 ; j++ ){
      double average = (image[i][j].r + image[i][j].g + image[i][j].b)/3.0; 
      double stddev = sqrt( ( (image[i][j].r-average)*(image[i][j].r-average) + 
  			      (image[i][j].g-average)*(image[i][j].g-average) + 
  			      (image[i][j].b-average)*(image[i][j].b-average))/3.0);
      double red_var = (image[i][j].r / 255.0) - 0.5;
      double green_var = (image[i][j].g / 255.0) - 0.5;
      double blue_var = (image[i][j].b / 255.0) - 0.5;
      double well_exposed = exp(-12.5*red_var*red_var) * exp(-12.5*green_var*green_var) * exp(-12.5*blue_var*blue_var);
      double laplacian = make_grey[i][j]*4 - ( make_grey[(i-1)][j] + make_grey[i][j-1] + make_grey[i][j+1] + make_grey[(i+1)][j]);
      weight[i][j] =  laplacian * stddev * well_exposed;
    }
  delete[] make_grey;
}


void hdr_direct_ref(const rgb (*input1)[N], const rgb (*input2)[N], rgb (*output)[N])
{
  double (*weights1)[N] = (double (*)[N])new double[M*N];
  compute_weights(input1,weights1);
  double (*weights2)[N] = (double (*)[N])new double[M*N];
  compute_weights(input2,weights2);
  for( int i = 1 ; i < M-1 ; i++ )
    for( int j = 1 ; j < N-1 ; j++ ){
      double sum = weights1[i][j] + weights2[i][j];
      output[i][j].r = (weights1[i][j] * input1[i][j].r + weights2[i][j] * input2[i][j].r) / sum;
      output[i][j].g = (weights1[i][j] * input1[i][j].g + weights2[i][j] * input2[i][j].g) / sum;
      output[i][j].b = (weights1[i][j] * input1[i][j].b + weights2[i][j] * input2[i][j].b) / sum;
    }
  delete[] weights1;
  delete[] weights2;
}


int main() {
  rgb (*inp1)[N] = (rgb (*)[N])new rgb[M*N];
  rgb (*inp2)[N] = (rgb (*)[N])new rgb[M*N];
  rgb (*outp)[N] = (rgb (*)[N])new rgb[M*N];
  rgb (*outp_ref)[N] = (rgb (*)[N])new rgb[M*N];

  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ ){
      inp1[i][j].r = rand() % 256;
      inp1[i][j].g = rand() % 256;
      inp1[i][j].b = rand() % 256;
      inp2[i][j].r = rand() % 256;
      inp2[i][j].g = rand() % 256;
      inp2[i][j].b = rand() % 256;
      outp[i][j].r = 0;
      outp[i][j].g = 0;
      outp[i][j].b = 0;
      outp_ref[i][j].r = 0;
      outp_ref[i][j].g = 0;
      outp_ref[i][j].b = 0;
    }

  hdr_direct((rgb *)inp1,(rgb *)inp2,M,N,(rgb*)outp);
  hdr_direct_ref(inp1,inp2,outp_ref);

  printf("Input1 :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" [%lf,%lf,%lf]",inp1[i][j].r,inp1[i][j].g,inp1[i][j].b);
    printf("\n");
  }
  printf("Input2 :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" [%lf,%lf,%lf]",inp2[i][j].r,inp2[i][j].g,inp2[i][j].b);
    printf("\n");
  }
  printf("Output :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" [%lf,%lf,%lf]",outp[i][j].r,outp[i][j].g,outp[i][j].b);
    printf("\n");
  }
  printf("Output_Ref :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" [%lf,%lf,%lf]",outp_ref[i][j].r,outp_ref[i][j].g,outp_ref[i][j].b);
    printf("\n");
  }

  double diff = 0;
  double pixel_threshold = 1e-6;
  for( int i = 1 ; i < M-1 ; i++ )
    for( int j = 1 ; j < N-1 ; j++ ){
      double curr_diffr = abs(outp_ref[i][j].r - outp[i][j].r);
      if( curr_diffr > pixel_threshold )
	diff += curr_diffr;
      double curr_diffg = abs(outp_ref[i][j].g - outp[i][j].g);
      if( curr_diffg > pixel_threshold )
	diff += curr_diffg;
      double curr_diffb = abs(outp_ref[i][j].b - outp[i][j].b);
      if( curr_diffb > pixel_threshold )
	diff += curr_diffb;
    }
  printf("Diff : %lf\n",diff);
  if( diff > 0 ){
    printf("Incorrect Result\n");
    exit(1);
  }

  delete[] inp1;
  delete[] outp;
  delete[] outp_ref;

  return 0;
}

