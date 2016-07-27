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
#ifndef __PRETTY_PRINT_HPP__
#define __PRETTY_PRINT_HPP__

#include <sstream>
#include <cassert>
#include <stack>
#include <iostream>
#include <cmath>

#define PSTRING(a,b) PrettyPrinter::instance()->print(PPTokenString(a),b)
#define PBREAK(a,b,c) PrettyPrinter::instance()->print(PPTokenBreak(a,b),c)
#define PBEGIN(a,b,c) PrettyPrinter::instance()->print(PPTokenBegin(a,b),c)
#define PEND PrettyPrinter::instance()->print(PPTokenEnd(),0)
// #define NUM_STRING_SIZE(a) ( a > 0 ? (int)log((double)a) : ( a < 0 ? (int)log((double)a)+1 : 1 ) )

int NUM_STRING_SIZE(int);

struct PPToken
{
  PPToken () {}
  virtual ~PPToken () {}
};

struct PPTokenString : public PPToken
{
  PPTokenString (const std::string& str) : str(str) {}
  std::string str;
};

struct PPTokenBreak : public PPToken
{
  PPTokenBreak (int blankSpace = 1, int offset = 0)
    : blankSpace(blankSpace), offset(offset) {}
  int blankSpace;
  int offset;
};

struct PPTokenBegin : public PPToken
{
  PPTokenBegin (int offset = 2, bool consistent = false)
    : offset(offset), consistent(consistent) {}
  int offset;
  bool consistent;
};

struct PPTokenEnd : public PPToken
{
  PPTokenEnd () {}
};

struct PPTokenEOF : public PPToken
{
  PPTokenEOF () {}
};


enum PrintStackBreak
  {
    PSB_FITS,
    PSB_INCONSISTENT,
    PSB_CONSISTENT
  };

struct PrintStackEntry
{
  PrintStackEntry (int o, PrintStackBreak psb) : offset(o), psBreak(psb) {}
  int offset;
  PrintStackBreak psBreak;
};


class PrettyPrinterBase{

public : 

  inline void print_string(){
    std::cout << outstream.str() << std::endl;
  }
  
  virtual ~PrettyPrinterBase();

  void print (PPTokenString x, const int l);
  void print (PPTokenBegin x, const int l);
  void print (PPTokenEnd x,const int l);
  void print (PPTokenBreak x, const int l);
  void print (PPTokenEOF x, const int l) {}

protected:

  std::stringstream outstream;

  PrettyPrinterBase(int width);
  
  // algorithm variables
  int space, margin;
  int left, right;
  PPToken ** token;
  int * size;
  int arraysize;
  static const int sizeInfinity = 9999999;
  int leftTotal, rightTotal;
  std::stack <PrintStackEntry> printStack;

  void printNewLine (int amount);
  void indent (int amount);

};


class PrettyPrinter : public PrettyPrinterBase
{
public:

  static PrettyPrinter* instance(int width){
    if( pretty_printer == NULL ){
      pretty_printer = new PrettyPrinter(width);
    }
    return pretty_printer;
  }

  static PrettyPrinter* instance(){
    assert(pretty_printer);
    return pretty_printer;
  }

  ///Destructor
  ~PrettyPrinter() { }

private:

  PrettyPrinter(int width) : PrettyPrinterBase(width) { }

  static PrettyPrinter* pretty_printer;
};


#endif
