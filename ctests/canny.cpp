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

#define N 1200
#define M 1000

#define max(a,b) ( (a) > (b) ? (a) : (b) )
#define min(a,b) ( (a) < (b) ? (a) : (b) )


extern "C" void canny(double* , int , int , double * );

void gaussian(double (*input)[N], double (*output)[N]){
  for( int i = 2 ; i < M-2 ; i++ )
    for( int j = 2 ; j < N-2 ; j++ ){
      output[i][j] =
        (2*input[i-2][j-2] + 4*input[i-2][j-1] + 5*input[i-2][j] +
         4*input[i-2][j+1] + 2*input[i-2][j+2] + 4*input[i-1][j-2] +
         9*input[i-1][j-1] + 12*input[i-1][j] + 9*input[i-1][j+1] +
         4*input[i-1][j+2] + 5*input[i][j-2] + 12*input[i][j-1] + 15*input[i][j]
         + 12*input[i][j+1] + 5*input[i][j+2] + 4*input[i+1][j-2] +
         9*input[i+1][j-1] + 12*input[i+1][j] + 9*input[i+1][j+1] +
         4*input[i+1][j+2] + 2*input[i+2][j-2] + 4*input[i+2][j-1] +
         5*input[i+2][j] + 4*input[i+2][j+1] + 2*input[i+2][j+2]) / 159;
    }
}

void suppress
(double (*gx)[N], double (*gy)[N], double (*mag)[N], double (*output)[N]){
  for( int i = 1 ; i < M-1 ; i++ )
    for( int j = 1 ; j < N-1 ; j++) {
      double theta = ( gx[i][j] == 0 ? (3.14156/2) : atan(gy[i][j]/gx[i][j]) );
      output[i][j] =
        (theta < -3 * 3.14156 / 8 ?
         ( mag[i][j] > max(mag[i-1][j],mag[i+1][j]) ? mag[i][j] : 0.0 ) :
         ( theta < -3.14156 / 8 ?
           ( mag[i][j] > max(mag[i+1][j+1],mag[i-1][j-1]) ? mag[i][j] : 0.0 ) :
           ( theta < 3.14156 / 8 ?
             ( mag[i][j] > max(mag[i][j+1],mag[i][j-1]) ? mag[i][j] : 0.0 ) :
             ( theta < 3 * 3.14156 / 8 ?
               (mag[i][j] > max(mag[i-1][j+1],mag[i+1][j-1]) ? mag[i][j] : 0.0)
               : (mag[i][j] > max(mag[i-1][j],mag[i][j+1]) ? mag[i][j] : 0.0 ))
             ) ) );
    }
}


void gradientxy_mag(double (*input)[N], double (*output)[N]){
  double (*gx)[N] = (double (*)[N])new double[M*N];
  memset(gx,0,sizeof(double)*M*N);
  double (*gy)[N] = (double (*)[N])new double[M*N];
  memset(gy,0,sizeof(double)*M*N);
  double (*mag)[N] = (double (*)[N])new double[M*N];
  memset(mag,0,sizeof(double)*M*N);
  for( int i = 1 ; i < M - 1 ; i++ )
    for( int j = 1 ; j < N - 1 ; j++ )
      gx[i][j] = (input[i-1][j+1] - input[i-1][j-1]) +
        2.0 * (input[i][j+1] - input[i][j-1])
        + ( input[i+1][j+1] - input[i+1][j-1] );
  for( int i = 1 ; i < M - 1 ; i++ )
    for( int j = 1 ; j < N - 1 ; j++ )
      gy[i][j] = (input[i-1][j-1] - input[i+1][j-1]) +
        2.0 * (input[i-1][j] - input[i+1][j])
        + ( input[i-1][j+1] - input[i+1][j+1] );
  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ )
      mag[i][j] = fabs(gx[i][j]) + fabs(gy[i][j]);
  suppress(gx,gy,mag,output);
  delete[] gx;
  delete[] gy;
  delete[] mag;
}


void canny_ref(double (*input)[N], double (*output)[N]){
  double (*gauss)[N] = (double (*)[N])new double[M*N];
  memset(gauss,0,sizeof(double)*M*N);
  gaussian(input,gauss);
  gradientxy_mag(gauss,output);
  delete[] gauss;
}


int main(int argc, char** argv)
{
  double (*input)[N]  = (double (*)[N])new double[M*N];
  double (*output)[N]  = (double (*)[N])new double[M*N];
  double (*output_ref)[N]  = (double (*)[N])new double[M*N];

  for( int i = 0 ; i < M ; i++ )
    for( int j = 0 ; j < N ; j++ ){
      input[i][j] = (double)(rand()) / (double)(RAND_MAX);
      output[i][j] = 0.0;
      output_ref[i][j] = 0.0;
    }

  canny((double*)input,M,N,(double*)output);
  canny_ref(input,output_ref);

#ifdef PRINT_OUTPUT
  printf("Output :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %lf",output[i][j]);
    printf("\n");
  }
  printf("Output_Ref :\n");
  for( int i = 0 ; i < M ; i++ ){
    for( int j = 0 ; j < N ; j++ )
      printf(" %lf",output_ref[i][j]);
    printf("\n");
  }
#endif
  double diff = 0.0;
  double max_error = 0.0;
  int max_i = 0, max_j = 0;
  for( int i = 4 ; i < M-4 ; i++ )
    for( int j = 4 ; j < N-4 ; j++ ) {
      double curr_error = fabs(output_ref[i][j] - output[i][j]);
      if (curr_error > max_error) {
        printf
          ("New Max Error : %e (%d,%d) , Prev Max : %e, Ref : %e\n",
           curr_error, i, j, max_error, output_ref[i][j]);
        max_error = curr_error;
        max_i = i;
        max_j = j;
      }
      diff += curr_error * curr_error;
    }
  diff = sqrt(diff / (M*N));
  printf("Diff : %e\n",diff);
  if( diff > 2e-5 || max_error > 1e-8){
    printf("Incorrect Result\n");
    exit(1);
  }

  delete[] input;
  delete[] output;
  delete[] output_ref;

  return 0;
}
