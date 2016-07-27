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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifdef _WINDOWS
#include <windows.h>
/* typedef union  { */
/*   struct { */
/*     DWORD LowPart; */
/*     LONG HighPart;     }; */
/*   LONGLONG QuadPart; } LARGE_INTEGER; */
double LIToSecs( LARGE_INTEGER * L) {
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency( &frequency ) ;
  return ((double)L->QuadPart /(double)frequency.QuadPart) ; }
/* double getElapsedTime() { */
/*   LARGE_INTEGER time; */
/*   time.QuadPart = timer->stop.QuadPart - timer->start.QuadPart; */
/*   return LIToSecs( &time) ; */
/* }  */
extern double rtclock() {
  LARGE_INTEGER time;
  QueryPerformanceCounter(&time);
  return LIToSecs(&time);
}
#else
#include <sys/time.h>
extern double rtclock() {
  struct timezone Tzp;
  struct timeval Tp;
  int stat;
  stat = gettimeofday (&Tp, &Tzp);
  if (stat != 0) printf("Error return from gettimeofday: %d",stat);
  return (Tp.tv_sec + Tp.tv_usec*1.0e-6);
}
#endif


void initialize_double_array(double * array, int size, double val)
{
  int i;
  for( i = 0 ; i < size ; i++ )
    array[i] = val;
}

void initialize_int_array(int * array, int size, int val)
{
  int i;
  for( i = 0 ; i < size ; i++ )
    array[i] = val;
}

void initialize_int8_array(unsigned char * array, int size, unsigned char val)
{
  int i;
  for( i = 0 ; i < size ; i++ )
    array[i] = val;
}

void initialize_short_array(short * array, int size, short val)
{
  int i;
  for( i = 0 ; i < size ; i++ )
    array[i] = val;
}


void initialize_float_array(float * array, int size, float val)
{
  int i;
  for( i = 0 ; i < size ; i++ )
    array[i] = val;
}


double** malloc_2d_double_array(int outer_dim, int inner_dim)
{
  double * base_array = (double*)malloc(sizeof(double)*inner_dim*outer_dim);
  int i;
  double** return_array = (double**)malloc(sizeof(double*)*outer_dim);
  for( i = 0 ; i < outer_dim ; i++)
    return_array[i] = &base_array[i*inner_dim];
  return return_array;
}

void free_2d_double_array(double** curr_array)
{
  free(curr_array[0]);
  free(curr_array);
}

float** malloc_2d_float_array(int outer_dim, int inner_dim)
{
  float * base_array = (float*)malloc(sizeof(float)*inner_dim*outer_dim);
  int i;
  float** return_array = (float**)malloc(sizeof(float*)*outer_dim);
  for( i = 0 ; i < outer_dim ; i++)
    return_array[i] = &base_array[i*inner_dim];
  return return_array;
}

void free_2d_float_array(float** curr_array)
{
  free(curr_array[0]);
  free(curr_array);
}


int** malloc_2d_int_array(int outer_dim, int inner_dim)
{
  int * base_array = (int*)malloc(sizeof(int)*inner_dim*outer_dim);
  int i;
  int** return_array = (int**)malloc(sizeof(int*)*outer_dim);
  for( i = 0 ; i < outer_dim ; i++)
    return_array[i] = &base_array[i*inner_dim];
  return return_array;
}

void free_2d_int_array(int** curr_array)
{
  free(curr_array[0]);
  free(curr_array);
}

unsigned char** malloc_2d_int8_array(int outer_dim, int inner_dim)
{
  unsigned char * base_array = (unsigned char*)malloc(sizeof(unsigned char)*inner_dim*outer_dim);
  int i;
  unsigned char** return_array = (unsigned char**)malloc(sizeof(unsigned char*)*outer_dim);
  for( i = 0 ; i < outer_dim ; i++)
    return_array[i] = &base_array[i*inner_dim];
  return return_array;
}

void free_2d_int8_array(unsigned char** curr_array)
{
  free(curr_array[0]);
  free(curr_array);
}


short** malloc_2d_int16_array(int outer_dim, int inner_dim)
{
  short * base_array = (short*)malloc(sizeof(short)*inner_dim*outer_dim);
  int i;
  short** return_array = (short**)malloc(sizeof(short*)*outer_dim);
  for( i = 0 ; i < outer_dim ; i++)
    return_array[i] = &base_array[i*inner_dim];
  return return_array;
}

void free_2d_int16_array(short** curr_array)
{
  free(curr_array[0]);
  free(curr_array);
}

