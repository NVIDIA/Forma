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
#include "AST/data_types.hpp"
#include <sstream>
#include <cstdio>
#include <cassert>
#include <vector>

using namespace std;

basic_data_types get_basic_data_type(const int num)
{
  basic_data_types curr_type;
  switch(num){
  case 1:
    curr_type = T_DOUBLE;
    break;
  case 2:
    curr_type = T_FLOAT;
    break;
  case 3:
    curr_type = T_INT;
    break;
  case 4:
    curr_type = T_INT16;
    break;
  case 5:
    curr_type = T_INT8;
    break;
  default:{
    assert(0 && "Unknown data type");
    curr_type = T_INT;
  }
  }
  return curr_type;
}


data_types get_data_type(const char* tn){
  data_types ret_val;
  string type_name(tn);
  if( type_name.compare("double") == 0 ){
    ret_val.assign(T_DOUBLE);
  }
  else if( type_name.compare("float") == 0 ){
    ret_val.assign(T_FLOAT);
  }
  else if( type_name.compare("int") == 0 ){
    ret_val.assign(T_INT);
  }
  else if( type_name.compare("int8") == 0 ){
    ret_val.assign(T_INT8);
  }
  else if( type_name.compare("int16") == 0 ){
    ret_val.assign(T_INT16);
  }
  else{
    defined_data_type* user_def_type = defined_types->find_symbol(tn);
    if( user_def_type == 0 ){
      fprintf(stderr,"[ME]: Error! Unkown data type %s\n",tn);
      exit(1);
    }
    ret_val.assign(T_STRUCT);
    ret_val.struct_info = user_def_type;
  }
  return ret_val;
}


static std::vector<std::string> supported_cuda_structs = {
  "float4"
};


static void add_supported_cuda_type(const std::string& name)
{
  if (name.compare("float4") == 0) {
    defined_data_type* float4_type = defined_types->find_symbol("float4");
    if (float4_type == NULL) {
      float4_type = new defined_data_type;
      float4_type->add_field("x", T_FLOAT);
      float4_type->add_field("y", T_FLOAT);
      float4_type->add_field("z", T_FLOAT);
      float4_type->add_field("w", T_FLOAT);
      float4_type->set_name("float4");
    }
    defined_types->add_new_symbol("float4", float4_type);
    return;
  }
  fprintf(stderr,"[ME} : Error! Unhandled cuda type %s\n",name.c_str());
  exit(1);
}


void data_types::add_cuda_types(const std::string& name)
{
  for (int i = 0 ; i < supported_cuda_structs.size(); i++) {
    if (supported_cuda_structs[i].compare(name) == 0)
      return add_supported_cuda_type(name);
  }
  fprintf(stderr,"[ME} : Error! Unhandled cuda type %s\n",name.c_str());
  exit(1);
}

bool data_types::is_supported_cuda_type(const std::string& name)
{
  for (int i = 0 ; i < supported_cuda_structs.size(); i++)
    if (supported_cuda_structs[i].compare(name) == 0)
      return true;
  return false;
}


int get_basic_type_size_in_bytes(const basic_data_types val)
{
  int size = 0;
  switch(val){
  case T_FLOAT:
    size = sizeof(float);
    break;
  case T_DOUBLE:
    size = sizeof(double);
    break;
  case T_INT:
    size = sizeof(int);
    break;
  case T_INT8:
    size = sizeof(unsigned char);
    break;
  case T_INT16:
    size = sizeof(short);
    break;
  case T_STRUCT:
    assert(0 && "Found struct when expecting basic type");
    break;
  default:
    ;
  }
  return size;
}

int get_type_size_in_bytes( const data_types& val){
  int size =0;
  if( val.type == T_STRUCT ){
    for( deque<defined_fields>::iterator
           fields_iter = val.struct_info->fields.begin(),
           fields_end = val.struct_info->fields.end();
         fields_iter != fields_end ; fields_iter++) {
      size += get_basic_type_size_in_bytes(fields_iter->field_type);
    }
  }
  else
    size = get_basic_type_size_in_bytes(val.type);
  return size;
}

string get_string(const data_types& val)
{
  string ret_string;
  switch(val.type){
  case T_FLOAT:
    ret_string.assign("float");
    break;
  case T_DOUBLE:
   ret_string.assign("double");
   break;
  case T_INT:
    ret_string.assign("int");
    break;
  case T_INT8:
    ret_string.assign("unsigned char");
    break;
  case T_INT16:
    ret_string.assign("short");
    break;
  case T_STRUCT: {
    assert(val.struct_info);
    stringstream curr_stream;
    curr_stream << "struct " << val.struct_info->name;
    ret_string = curr_stream.str();
    break;
  }
  default:
    printf("[ME] : Error! Undefined data type\n");
    assert(0);
  };

  return ret_string;
}

string get_forma_string(const data_types& val)
{
  string ret_string;
  switch(val.type){
  case T_FLOAT:
    ret_string.assign("float");
    break;
  case T_DOUBLE:
   ret_string.assign("double");
   break;
  case T_INT:
    ret_string.assign("int");
    break;
  case T_INT8:
    ret_string.assign("int8");
    break;
  case T_INT16:
    ret_string.assign("int16");
    break;
  case T_STRUCT: {
    assert(0 && "Currently cant handle structs in forma string type");
    break;
  }
  default:
    printf("[ME] : Error! Undefined data type\n");
    assert(0);
  };

  return ret_string;
}


int get_string_size(const data_types& val)
{
  int string_len;
  switch(val.type){
  case T_FLOAT:
    string_len = 5;
    break;
  case T_DOUBLE:
    string_len = 6;
   break;
  case T_INT:
    string_len = 3;
    break;
  case T_INT8:
    string_len = 13;
    break;
  case T_INT16:
    string_len = 5;
    break;
  case T_STRUCT:{
    assert(val.struct_info);
    stringstream curr_stream;
    curr_stream << "struct " << val.struct_info->name;
    string_len = curr_stream.str().size();
  }
  default: {
    fprintf(stderr,"[ME] : Error! Undefined data type\n");
    string_len = 0;
  }
  };
  return string_len;
}
