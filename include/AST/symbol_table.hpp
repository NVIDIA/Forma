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
#ifndef __SYMBOL_TABLE__
#define __SYMBOL_TABLE__

#include <cstdlib>
#include <string>
#include <deque>
#include <cstdio>
#include <cassert>

template <typename State> class ASTVisitor;

/** Base class used for symobl table operations
 */
template<typename T>
class symbol_table{
  
protected:
  ///The symbol table, no duplicates
  std::deque<std::pair<std::string,T> > symbols;

public:

  ///Method to add a symbol 
  int add_symbol(const char*,T);

  ///Method to find a symbol
  T find_symbol(const char*) const;

  ///Method to remove a symbol
  bool remove_symbol(const char* symbol_name) {
    for( typename std::deque<std::pair<std::string, T> >::iterator it = symbols.begin() ; it != symbols.end() ; it++ ){
      if( it->first.compare(symbol_name) == 0 ){
	symbols.erase(it);
	return true;
      }
    }
    return false;
  }

  ///Method to add a symbol without checking
  void add_new_symbol(const char*, T);

  ///Return the number of symbols
  inline int get_nsymbols(){
    return symbols.size();
  }

  inline typename std::deque<std::pair<std::string,T> >::const_iterator begin() const{
    return symbols.begin();
  }

  inline typename std::deque<std::pair<std::string,T> >::const_iterator end() const{
    return symbols.end();
  }

};

template<typename T>
int symbol_table<T>::add_symbol(const char* sym_name, T sym_value)
{
  std::string new_name(sym_name);
  for( typename std::deque<std::pair<std::string,T> >::iterator it = symbols.begin() ; it != symbols.end() ; it++ )
    if( it->first.compare(new_name) == 0 )
      return -1;
  symbols.push_back(std::pair<std::string,T>(new_name,sym_value));
  return 0;
}

template<typename T>
void symbol_table<T>::add_new_symbol(const char* sym_name, T sym_value)
{
  std::string new_name(sym_name);
  symbols.push_back(std::pair<std::string,T>(new_name,sym_value));
}

template<typename T>
T symbol_table<T>::find_symbol(const char* sym_name) const
{
  std::string find_name(sym_name);
  for( typename std::deque<std::pair<std::string,T> >::const_iterator it = symbols.begin(); it != symbols.end() ; it++ )
    if( it->first.compare(find_name) == 0)
      return it->second;
  return 0;
}

class expr_node;

class fn_defn_node;
class vector_expr_node;
class vector_defn_node;
class stmt_node;

/** Class to store vector symbols defined, either as input or assigned to
 */
class local_symbols : public symbol_table<vector_expr_node*> {

protected:

  // int pretty_print_size;
  
public:

  // inline void find_symbol(){
  //   return this->find_symbol();
  // }

  ///Function to add a vector symbol, checks for duplicates
  inline void add_local_symbol(const char* sym_name, vector_expr_node* sym_defn){
    if( this->add_symbol(sym_name,sym_defn) ){
      fprintf(stderr,"ERROR: Duplicate vector definitions: %s\n",sym_name);
      exit(1);
    }
  }

  ///Function to find a vector symbol definition
  inline vector_expr_node* find_local_symbol(const char* sym_name, bool assert_on_fail = true) const{
    vector_expr_node* vec_expr = find_symbol(sym_name);
    if( assert_on_fail && !vec_expr ){
      fprintf(stderr,"ERROR : Unknown Symbol %s\n",sym_name);
      fflush(stderr);
      exit(1);
    }
    return vec_expr;
  }

  ///Function to remove a symbol
  void remove_local_symbol(stmt_node*) ;

  ///Overloaded function for pretty printing
  void print_node(FILE*) const;

  ///Hack function to print symbols at program level
  void print_program(FILE*) const;

  ///Destructor
  ~local_symbols();

  friend class fn_defn_node;
};



#endif
