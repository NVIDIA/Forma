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
#ifndef __DATA_TYPES_HPP__
#define __DATA_TYPES_HPP__

#include <string>
#include "AST/symbol_table.hpp"

/** List of supported data types
 */
enum basic_data_types{
  T_STRUCT,
  T_DOUBLE,
  T_FLOAT,
  T_INT,
  T_INT16,
  T_INT8,
};

struct defined_fields{
  std::string field_name;
  basic_data_types field_type;
  defined_fields(const char* name, basic_data_types type) :
    field_name(name) , field_type(type) { }
};


struct defined_data_type{
  std::string name;
  std::deque<defined_fields> fields;
  void add_field(const char* new_name, basic_data_types new_type){
    if( new_type == T_STRUCT ){
      fprintf
        (stderr,
         "[ME] : Error! Field (%s) of a struct not a supported data type\n",
         new_name);
      exit(1);
    }
    for(std::deque<defined_fields>::const_iterator it = fields.begin() ;
        it != fields.end() ; it++ ){
      if( it->field_name.compare(new_name) == 0 ){
        fprintf
          (stderr,"[ME]: Error! Duplicate field %s in defined data type\n",
           new_name);
        exit(1);
      }
    }
    fields.push_back(defined_fields(new_name,new_type));
  }
  void set_name(const std::string& id_name){
    name.assign(id_name);
  }
  ~defined_data_type(){
    fields.clear();
  }
};

struct data_types{
  basic_data_types type;
  defined_data_type* struct_info;
  data_types() {
    struct_info = NULL ;
  }
  void assign(basic_data_types curr_type) {
    type = curr_type;
    struct_info = NULL;
  }
  void assign(const data_types& base){
    type = base.type;
    struct_info = base.struct_info;
  }
  static bool is_same_type(const data_types& lhs, const data_types& rhs){
    return ( lhs.type == rhs.type ) && ( lhs.struct_info == rhs.struct_info );
  }
  static void add_cuda_types(const std::string& name);
  static bool is_supported_cuda_type(const std::string& name);
};


///Function to convert string to data_types
extern basic_data_types get_basic_data_type(const int);

extern data_types get_data_type(const char*);

extern std::string get_string(const data_types&);

extern std::string get_forma_string(const data_types&);

extern int get_string_size(const data_types&);

extern int get_basic_type_size_in_bytes(const basic_data_types val);

extern int get_type_size_in_bytes(const data_types& );

typedef symbol_table<defined_data_type*> defined_types_table;

extern defined_types_table* defined_types;

#endif
