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

#define absd(a) ( (a) > 0 ? (a) : -(a) )
void canny_gold(const float* input, int height, int width, float* output)
{
  float* gaussian = new float[height*width];
  for( int i = 0 ; i < 2 ; i++ ){
    for( int j = 0 ; j < 2 ; j++ )
      gaussian[i*width+j] = (2*input[(((i-2) < 0 ? 2-i : i-2))*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(((i-2) < 0 ? 2-i : i-2))*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(((i-2) < 0 ? 2-i : i-2))*width+j] + 4*input[(((i-2) < 0 ? 2-i : i-2))*width+j+1] + 2*input[(((i-2) < 0 ? 2-i : i-2))*width+j+2] + 4*input[(((i-1) < 0 ? 1-i : i-1))*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(((i-1) < 0 ? 1-i : i-1))*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(((i-1) < 0 ? 1-i : i-1))*width+j] + 9*input[(((i-1) < 0 ? 1-i : i-1))*width+j+1] + 4*input[(((i-1) < 0 ? 1-i : i-1))*width+j+2] + 5*input[i*width+((j-2) < 0 ? 2-j : j-2)] + 12*input[i*width+((j-1) < 0 ? 1-j : j-1)] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(i+1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i+1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+j+1] + 4*input[(i+1)*width+j+2] + 2*input[(i+2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i+2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+j+1] + 2*input[(i+2)*width+j+2]) / 159;
    for( int j = 2 ; j < width -2 ; j++ )
      gaussian[i*width+j] = (2*input[(((i-2) < 0 ? 2-i : i-2))*width+j-2] + 4*input[(((i-2) < 0 ? 2-i : i-2))*width+j-1] + 5*input[(((i-2) < 0 ? 2-i : i-2))*width+j] + 4*input[(((i-2) < 0 ? 2-i : i-2))*width+j+1] + 2*input[(((i-2) < 0 ? 2-i : i-2))*width+j+2] + 4*input[(((i-1) < 0 ? 1-i : i-1))*width+j-2] + 9*input[(((i-1) < 0 ? 1-i : i-1))*width+j-1] + 12*input[(((i-1) < 0 ? 1-i : i-1))*width+j] + 9*input[(((i-1) < 0 ? 1-i : i-1))*width+j+1] + 4*input[(((i-1) < 0 ? 1-i : i-1))*width+j+2] + 5*input[i*width+j-2] + 12*input[i*width+j-1] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(i+1)*width+j-2] + 9*input[(i+1)*width+j-1] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+j+1] + 4*input[(i+1)*width+j+2] + 2*input[(i+2)*width+j-2] + 4*input[(i+2)*width+j-1] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+j+1] + 2*input[(i+2)*width+j+2]) / 159;
    for( int j = width-2 ; j < width ; j++ )
      gaussian[i*width+j] = (2*input[(((i-2) < 0 ? 2-i : i-2))*width+j-2] + 4*input[(((i-2) < 0 ? 2-i : i-2))*width+j-1] + 5*input[(((i-2) < 0 ? 2-i : i-2))*width+j] + 4*input[(((i-2) < 0 ? 2-i : i-2))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 2*input[(((i-2) < 0 ? 2-i : i-2))*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 4*input[(((i-1) < 0 ? 1-i : i-1))*width+j-2] + 9*input[(((i-1) < 0 ? 1-i : i-1))*width+j-1] + 12*input[(((i-1) < 0 ? 1-i : i-1))*width+j] + 9*input[(((i-1) < 0 ? 1-i : i-1))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 4*input[(((i-1) < 0 ? 1-i : i-1))*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 5*input[i*width+j-2] + 12*input[i*width+j-1] + 15*input[i*width+j] + 12*input[i*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 5*input[i*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 4*input[(i+1)*width+j-2] + 9*input[(i+1)*width+j-1] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 4*input[(i+1)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 2*input[(i+2)*width+j-2] + 4*input[(i+2)*width+j-1] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 2*input[(i+2)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)]) / 159;
  }
  for( int i = 2 ; i < height -2 ; i++ ){
    for( int j = 0 ; j < 2 ; j++ )
      gaussian[i*width+j] = (2*input[(i-2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i-2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+j+1] + 2*input[(i-2)*width+j+2] + 4*input[(i-1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+j+1] + 4*input[(i-1)*width+j+2] + 5*input[i*width+((j-2) < 0 ? 2-j : j-2)] + 12*input[i*width+((j-1) < 0 ? 1-j : j-1)] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(i+1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i+1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+j+1] + 4*input[(i+1)*width+j+2] + 2*input[(i+2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i+2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+j+1] + 2*input[(i+2)*width+j+2]) / 159;
    for( int j = 2 ; j < width -2 ; j++ )
      gaussian[i*width+j] = (2*input[(i-2)*width+j-2] + 4*input[(i-2)*width+j-1] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+j+1] + 2*input[(i-2)*width+j+2] + 4*input[(i-1)*width+j-2] + 9*input[(i-1)*width+j-1] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+j+1] + 4*input[(i-1)*width+j+2] + 5*input[i*width+j-2] + 12*input[i*width+j-1] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(i+1)*width+j-2] + 9*input[(i+1)*width+j-1] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+j+1] + 4*input[(i+1)*width+j+2] + 2*input[(i+2)*width+j-2] + 4*input[(i+2)*width+j-1] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+j+1] + 2*input[(i+2)*width+j+2]) / 159;
    for( int j = width-2 ; j < width ; j++ )
      gaussian[i*width+j] = (2*input[(i-2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i-2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 2*input[(i-2)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 4*input[(i-1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 4*input[(i-1)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 5*input[i*width+((j-2) < 0 ? 2-j : j-2)] + 12*input[i*width+((j-1) < 0 ? 1-j : j-1)] + 15*input[i*width+j] + 12*input[i*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 5*input[i*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 4*input[(i+1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i+1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 4*input[(i+1)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 2*input[(i+2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i+2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 2*input[(i+2)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)]) / 159;
  }
  for( int i = height-2 ; i < height ; i++ ){
    for( int j = 0 ; j < 2 ; j++ )
      gaussian[i*width+j] = (2*input[(i-2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i-2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+j+1] + 2*input[(i-2)*width+j+2] + 4*input[(i-1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+j+1] + 4*input[(i-1)*width+j+2] + 5*input[i*width+((j-2) < 0 ? 2-j : j-2)] + 12*input[i*width+((j-1) < 0 ? 1-j : j-1)] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j] + 9*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+1] + 4*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+2] + 2*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j] + 4*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j+1] + 2*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j+2]) / 159;
    for( int j = 2 ; j < width -2 ; j++ )
      gaussian[i*width+j] = (2*input[(i-2)*width+j-2] + 4*input[(i-2)*width+j-1] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+j+1] + 2*input[(i-2)*width+j+2] + 4*input[(i-1)*width+j-2] + 9*input[(i-1)*width+j-1] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+j+1] + 4*input[(i-1)*width+j+2] + 5*input[i*width+j-2] + 12*input[i*width+j-1] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j-2] + 9*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j-1] + 12*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j] + 9*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+1] + 4*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+2] + 2*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j-2] + 4*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j-1] + 5*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j] + 4*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j+1] + 2*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j+2]) / 159;
    for( int j = width-2 ; j < width ; j++ )
      gaussian[i*width+j] = (2*input[(i-2)*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(i-2)*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 2*input[(i-2)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 4*input[(i-1)*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 4*input[(i-1)*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 5*input[i*width+((j-2) < 0 ? 2-j : j-2)] + 12*input[i*width+((j-1) < 0 ? 1-j : j-1)] + 15*input[i*width+j] + 12*input[i*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 5*input[i*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 4*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j-2) < 0 ? 2-j : j-2)] + 9*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j-1) < 0 ? 1-j : j-1)] + 12*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j] + 9*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 4*input[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)] + 2*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+((j-2) < 0 ? 2-j : j-2)] + 4*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+((j-1) < 0 ? 1-j : j-1)] + 5*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+j] + 4*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] + 2*input[(((i+2)>=height ? 2*(height-1)-(i+2) : i+2))*width+((j+2)>=width ? 2*(width-1)-(j+2) : j+2)]) / 159;
  }


  for( int i = 0 ; i < 1; i++ ){
    for( int j = 0 ; j < 1 ; j++ ){
      float gradientx = gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j+1] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+((j-1) < 0 ? 1-j : j-1)] + 2*(gaussian[i*width+j+1] - gaussian[i*width+((j-1) < 0 ? 1-j : j-1)]) + gaussian[(i+1)*width+j+1] - gaussian[(i+1)*width+((j-1) < 0 ? 1-j : j-1)];
      float gradienty = gaussian[(i+1)*width+((j-1) < 0 ? 1-j : j-1)] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+((j-1) < 0 ? 1-j : j-1)] + 2*(gaussian[(i+1)*width+j] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j]) + gaussian[(i+1)*width+j+1] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
    for( int j = 1 ; j < width -1 ; j++ ){
      float gradientx = gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j+1] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j-1] + 2*(gaussian[i*width+j+1] - gaussian[i*width+j-1]) + gaussian[(i+1)*width+j+1] - gaussian[(i+1)*width+j-1];
      float gradienty = gaussian[(i+1)*width+j-1] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j-1] + 2*(gaussian[(i+1)*width+j] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j]) + gaussian[(i+1)*width+j+1] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
    for( int j = width-1 ; j < width  ; j++ ){
      float gradientx = gaussian[(((i-1) < 0 ? 1-i : i-1))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j-1] + 2*(gaussian[i*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[i*width+j-1]) + gaussian[(i+1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(i+1)*width+j-1];
      float gradienty = gaussian[(i+1)*width+j-1] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j-1] + 2*(gaussian[(i+1)*width+j] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+j]) + gaussian[(i+1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(((i-1) < 0 ? 1-i : i-1))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)];
      output[i*width+j] = gradientx+gradienty;
    }
  }
  for( int i = 1 ; i < height - 1; i++ ){
    for( int j = 0 ; j < 1 ; j++ ){
      float gradientx = gaussian[(i-1)*width+j+1] - gaussian[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 2*(gaussian[i*width+j+1] - gaussian[i*width+((j-1) < 0 ? 1-j : j-1)]) + gaussian[(i+1)*width+j+1] - gaussian[(i+1)*width+((j-1) < 0 ? 1-j : j-1)];
      float gradienty = gaussian[(i+1)*width+((j-1) < 0 ? 1-j : j-1)] - gaussian[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 2*(gaussian[(i+1)*width+j] - gaussian[(i-1)*width+j]) + gaussian[(i+1)*width+j+1] - gaussian[(i-1)*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
    for( int j = 1 ; j < width -1 ; j++ ){
      float gradientx = gaussian[(i-1)*width+j+1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[i*width+j+1] - gaussian[i*width+j-1]) + gaussian[(i+1)*width+j+1] - gaussian[(i+1)*width+j-1];
      float gradienty = gaussian[(i+1)*width+j-1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[(i+1)*width+j] - gaussian[(i-1)*width+j]) + gaussian[(i+1)*width+j+1] - gaussian[(i-1)*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
    for( int j = width-1 ; j < width  ; j++ ){
      float gradientx = gaussian[(i-1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(i-1)*width+j-1] + 2*(gaussian[i*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[i*width+j-1]) + gaussian[(i+1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(i+1)*width+j-1];
      float gradienty = gaussian[(i+1)*width+j-1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[(i+1)*width+j] - gaussian[(i-1)*width+j]) + gaussian[(i+1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(i-1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)];
      output[i*width+j] = gradientx+gradienty;
    }
  }
  for( int i = height-1 ; i < height; i++ ){
    for( int j = 0 ; j < 1 ; j++ ){
      float gradientx = gaussian[(i-1)*width+j+1] - gaussian[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 2*(gaussian[i*width+j+1] - gaussian[i*width+((j-1) < 0 ? 1-j : j-1)]) + gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+1] - gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j-1) < 0 ? 1-j : j-1)];
      float gradienty = gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j-1) < 0 ? 1-j : j-1)] - gaussian[(i-1)*width+((j-1) < 0 ? 1-j : j-1)] + 2*(gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j] - gaussian[(i-1)*width+j]) + gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+1] - gaussian[(i-1)*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
    for( int j = 1 ; j < width -1 ; j++ ){
      float gradientx = gaussian[(i-1)*width+j+1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[i*width+j+1] - gaussian[i*width+j-1]) + gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+1] - gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j-1];
      float gradienty = gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j-1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j] - gaussian[(i-1)*width+j]) + gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j+1] - gaussian[(i-1)*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
    for( int j = width-1 ; j < width  ; j++ ){
      float gradientx = gaussian[(i-1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(i-1)*width+j-1] + 2*(gaussian[i*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[i*width+j-1]) + gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j-1];
      float gradienty = gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j-1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+j] - gaussian[(i-1)*width+j]) + gaussian[(((i+1)>=height ? 2*(height-1)-(i+1) : i+1))*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)] - gaussian[(i-1)*width+((j+1)>=width ? 2*(width-1)-(j+1) : j+1)];
      output[i*width+j] = gradientx+gradienty;
    }
  }
  delete[] gaussian;
}


extern "C" void canny_mirror(float* input, int height, int width, float* output);


int main()
{
  int width = 1200;
  int height = 1000;

  float * input = new float[width*height];
  float * output = new float[width*height];
  
  for( int i = 0 ; i < height ; i++) 
    for( int j = 0 ; j < width ; j++ ){
      input[i*width+j] = (rand()%256)/ 5.0;
      output[i*width+j] = 0.0;
    }
  

  canny_mirror(input,height,width,output);

  float* output_gold = new float[width*height];
  canny_gold(input,height,width,output_gold);

#ifdef PRINT_OUTPUT
  printf("Input : \n");
  for( int i = 0 ; i < height ; i++ ){
    for( int j = 0 ; j < width ; j++ )
      printf(" %f",input[i*width+j]);
    printf("\n");
  }
  printf("Output : \n");
  for( int i = 0 ; i < height-0 ; i++ ){
    for( int j = 0 ; j < width-0 ; j++ )
      printf(" %f",output[i*width+j]);
    printf("\n");
  }
  printf("Output_Gold : \n");
  for( int i = 0 ; i < height-0 ; i++ ){
    for( int j = 0 ; j < width-0 ; j++ )
      printf(" %f",output_gold[i*width+j]);
    printf("\n");
  }
#endif

  double max_error = 0.0;
  double avg_error = 0.0;
  int err_location_i=-1,err_location_j=-1;
  for( int i = 0 ; i < height - 0 ; i++ )
    for( int j = 0 ; j < width - 0 ; j++ ){
      double curr_error = absd(output[i*width+j] - output_gold[i*width+j]);
      if( curr_error > max_error ){
	printf("MaxDiff %e at (%d,%d), Prev MaxDiff %e\n",curr_error, err_location_i, err_location_j, max_error);
  	max_error = curr_error;
  	err_location_i = i;
  	err_location_j = j;
      }
      avg_error += curr_error;
    }

  printf("Max Error : %lf at (%d,%d), Cummulative Error: %lf\n",max_error,err_location_j,err_location_i,avg_error);
  delete[] output_gold;
  delete[] input;
  delete[] output;

  if( avg_error / (height * width) > 1e-3 ){
    printf("Incorrect Result\n");
    exit(1);
  }
  return 0;
}
  
