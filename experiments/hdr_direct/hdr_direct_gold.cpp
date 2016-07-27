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
#include <cmath>

struct rgb{
  float r;
  float g;
  float b;
};

extern void hdr_direct_gold(const rgb* image1, const rgb* image2 , const rgb* image3, const rgb* image4, int height, int width, rgb* output);

void compute_weights_gold(const rgb* image, int height, int width, float* weight)
{
  float* make_grey = new float[height*width];
  for( int i = 0 ; i < height ; i++ )
    for( int j = 0 ; j < width ; j++ )
      make_grey[i*width+j] = (0.02126f * image[i*width+j].r + 0.7152f * image[i*width+j].g + 0.0722f * image[i*width+j].b) / 255.0f;
  
  for( int i = 1 ; i < height-1; i++ )
    for( int j = 1 ; j < width-1 ; j++ ){
      float average = (image[i*width+j].r + image[i*width+j].g + image[i*width+j].b)/3.0f; 
      float stddev = sqrt( ( (image[i*width+j].r-average)*(image[i*width+j].r-average) + (image[i*width+j].g-average)*(image[i*width+j].g-average) + (image[i*width+j].b-average)*(image[i*width+j].b-average))/3.0f);
      float well_exposed = exp(-12.5f*((image[i*width+j].r/255.0f)-0.5f)*((image[i*width+j].r/255.0f)-0.5f)) * exp(-12.5f*((image[i*width+j].g/255.0f)-0.5f)*((image[i*width+j].g/255.0f)-0.5f)) * exp(-12.5f*((image[i*width+j].b/255.0f)-0.5f)*((image[i*width+j].b/255.0f)-0.5f));
      float laplacian = make_grey[i*width+j]*4 - ( make_grey[(i-1)*width+j] + make_grey[i*width+j-1] + make_grey[i*width+j+1] + make_grey[(i+1)*width+j]);
      weight[i*width+j] = stddev * laplacian * well_exposed;
    }
  delete[] make_grey;
}


void hdr_direct_gold(const rgb* image1, const rgb* image2 , const rgb* image3, const rgb* image4, int height, int width, rgb* output)
{
  float * weight1 = new float[height*width];
  float * weight2 = new float[height*width];
  float * weight3 = new float[height*width];
  float * weight4 = new float[height*width];

  compute_weights_gold(image1,height,width,weight1);
  compute_weights_gold(image2,height,width,weight2);
  compute_weights_gold(image3,height,width,weight3);
  compute_weights_gold(image4,height,width,weight4);

  for( int i = 1; i < height - 1 ; i++ )
    for( int j = 1 ; j < width - 1 ; j++ ){
      float sum = weight1[i*width+j] + weight2[i*width+j] + weight3[i*width+j] + weight4[i*width+j];
      output[i*width+j].r = (image1[i*width+j].r*weight1[i*width+j] + image2[i*width+j].r*weight2[i*width+j] + image3[i*width+j].r*weight3[i*width+j] + image4[i*width+j].r*weight4[i*width+j]) / sum;
      output[i*width+j].g = (image1[i*width+j].g*weight1[i*width+j] + image2[i*width+j].g*weight2[i*width+j] + image3[i*width+j].g*weight3[i*width+j] + image4[i*width+j].g*weight4[i*width+j]) / sum;
      output[i*width+j].b = (image1[i*width+j].b*weight1[i*width+j] + image2[i*width+j].b*weight2[i*width+j] + image3[i*width+j].b*weight3[i*width+j] + image4[i*width+j].b*weight4[i*width+j]) / sum;
    }
  delete[] weight1;
  delete[] weight2;
  delete[] weight3;
  delete[] weight4;
}
