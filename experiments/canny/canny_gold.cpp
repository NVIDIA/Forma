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
extern void canny_gold(const float* input, int height, int width, float* output);

void canny_gold(const float* input, int height, int width, float* output)
{
  float* gaussian = new float[height*width];
  for( int i = 2 ; i < height -2 ; i++ )
    for( int j = 2 ; j < width -2 ; j++ ){
      gaussian[i*width+j] = (2*input[(i-2)*width+j-2] + 4*input[(i-2)*width+j-1] + 5*input[(i-2)*width+j] + 4*input[(i-2)*width+j+1] + 2*input[(i-2)*width+j+2] + 4*input[(i-1)*width+j-2] + 9*input[(i-1)*width+j-1] + 12*input[(i-1)*width+j] + 9*input[(i-1)*width+j+1] + 4*input[(i-1)*width+j+2] + 5*input[i*width+j-2] + 12*input[i*width+j-1] + 15*input[i*width+j] + 12*input[i*width+j+1] + 5*input[i*width+j+2] + 4*input[(i+1)*width+j-2] + 9*input[(i+1)*width+j-1] + 12*input[(i+1)*width+j] + 9*input[(i+1)*width+j+1] + 4*input[(i+1)*width+j+2] + 2*input[(i+2)*width+j-2] + 4*input[(i+2)*width+j-1] + 5*input[(i+2)*width+j] + 4*input[(i+2)*width+j+1] + 2*input[(i+2)*width+j+2]) / 159;
    }
  
  for( int i = 3 ; i < height - 3; i++ )
    for( int j = 3 ; j < width -3 ; j++ ){
      float gradientx = gaussian[(i-1)*width+j+1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[i*width+j+1] - gaussian[i*width+j-1]) + gaussian[(i+1)*width+j+1] - gaussian[(i+1)*width+j-1];
      float gradienty = gaussian[(i+1)*width+j-1] - gaussian[(i-1)*width+j-1] + 2*(gaussian[(i+1)*width+j] - gaussian[(i-1)*width+j]) + gaussian[(i+1)*width+j+1] - gaussian[(i-1)*width+j+1];
      output[i*width+j] = gradientx+gradienty;
    }
  delete[] gaussian;
}


