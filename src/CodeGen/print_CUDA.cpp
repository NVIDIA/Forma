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
#include "CodeGen/print_CUDA.hpp"

using namespace std;

///-----------------------------------------------------------------------------
int get_channel_size(basic_data_types curr_type)
{
  int base_channel_size;
  switch(curr_type){
  case T_INT8:
    base_channel_size = 8;
    break;
  case T_INT16:
    base_channel_size = 16;
    break;
  case T_INT:
  case T_FLOAT:
    base_channel_size = 32;
    break;
  default:
    assert(0);
    base_channel_size = 32;
  }
  return base_channel_size;
}


///-----------------------------------------------------------------------------
c_var_info* PrintCUDA::printMalloc
(std::string lhs, data_types elem_type, const domain_node* expr_domain)
{
  assert(expr_domain->get_dim() != 0 && "Calling malloc on a scalar");
  string elem_type_string = get_string(elem_type);
  cuda_var_info* new_var =
    new cuda_var_info(lhs,expr_domain,elem_type);
  def_vars.push_back(new_var);
  host_allocate->indent();
  host_allocate->buffer << elem_type_string << " * " << lhs << ";" ;
  host_allocate->newline();
  if( enable_texture && expr_domain->get_dim() > 1 ){
    stringstream pitch_stream;
    pitch_stream <<  lhs << "_pitch__" ;
    new_var->pitch = pitch_stream.str();
    host_allocate->indent();
    host_allocate->buffer << "size_t " << new_var->pitch << ";";
    host_allocate->newline();
    host_allocate->indent();
    host_allocate->buffer << "cudaMallocPitch(&" << lhs << ",&" <<
      new_var->pitch <<  ",sizeof(" << elem_type_string << ")*";
    deque<range_coeffs>::const_reverse_iterator it =
      expr_domain->get_domain().rbegin();
    host_allocate->buffer << "(" ;
    PrintCParametricExpr(it->ub,host_allocate->buffer);
    host_allocate->buffer << "-";
    PrintCParametricExpr(it->lb,host_allocate->buffer);
    host_allocate->buffer << "+1),";
    it++;
    host_allocate->buffer << "(" ;
    PrintCParametricExpr(it->ub,host_allocate->buffer);
    host_allocate->buffer << "-";
    PrintCParametricExpr(it->lb,host_allocate->buffer);
    host_allocate->buffer << "+1)";
    for( it++ ; it != expr_domain->get_domain().rend() ; it++ ){
      host_allocate->buffer << "*(" ;
      PrintCParametricExpr(it->ub,host_allocate->buffer);
      host_allocate->buffer << "-";
      PrintCParametricExpr(it->lb,host_allocate->buffer);
      host_allocate->buffer << "+1)";
    }
    host_allocate->buffer << ");";
    host_allocate->newline();
    host_allocate->indent();
    host_allocate->buffer << new_var->pitch << " /= sizeof(" <<
      elem_type_string << ");";
    host_allocate->newline();

    if( init_zero ){
      host_allocate->buffer << "cudaMemset2D(" << lhs << "," <<
        new_var->pitch << "*sizeof(" << elem_type_string << "),0,sizeof(" <<
        elem_type_string << ")*";
      it = expr_domain->get_domain().rbegin();
      host_allocate->buffer << "(";
      PrintCParametricExpr(it->ub,host_allocate->buffer);
      host_allocate->buffer << "-";
      PrintCParametricExpr(it->lb,host_allocate->buffer);
      host_allocate->buffer << "+1),";
      it++;
      host_allocate->buffer << "(";
      PrintCParametricExpr(it->ub,host_allocate->buffer);
      host_allocate->buffer << "-";
      PrintCParametricExpr(it->lb,host_allocate->buffer);
      host_allocate->buffer << "+1)";
      for( it++ ; it != expr_domain->get_domain().rend() ; it++ ){
        host_allocate->buffer << "*(";
        PrintCParametricExpr(it->ub,host_allocate->buffer);
        host_allocate->buffer <<"-";
        PrintCParametricExpr(it->lb,host_allocate->buffer);
        host_allocate->buffer <<"+1)";
      }
      host_allocate->buffer <<");";
      host_allocate->newline();
    }
  }
  else{
    if( use_single_malloc ){
      static int malloc_num = 0;
      stringstream malloc_size_var;
      malloc_size_var << "__malloc_" << malloc_num << "_size__";

      host_allocate_size->indent();
      host_allocate_size->buffer << "int " << malloc_size_var.str() <<
        " = sizeof(" << elem_type_string << ")*";
      PrintCDomainSize(expr_domain,host_allocate_size->buffer);
      host_allocate_size->buffer << ";";
      host_allocate_size->newline();

      host_allocate_size->indent();
      host_allocate_size->buffer << malloc_size_var.str() <<
        " = FORMA_CEIL(" << malloc_size_var.str() << ",16)*16;";
      host_allocate_size->newline();

      host_allocate_size->indent();
      host_allocate_size->buffer << globalMemOffset << " += " <<
        malloc_size_var.str() << ";";
      host_allocate_size->newline();

      host_allocate->indent();
      host_allocate->buffer << lhs << " = (" << elem_type_string << "*)(" <<
        globalMemVar << "+" << globalMemOffset << ");";
      host_allocate->newline();

      stringstream malloc_offset_var;
      malloc_offset_var << "__malloc_" << malloc_num << "_offset__";
      host_allocate->indent();
      host_allocate->buffer << "int " << malloc_offset_var.str() <<
        " = sizeof(" << elem_type_string << ")*";
      PrintCDomainSize(expr_domain,host_allocate->buffer);
      host_allocate->buffer << ";";
      host_allocate->newline();

      host_allocate->indent();
      host_allocate->buffer << malloc_offset_var.str() <<
        " = FORMA_CEIL(" << malloc_offset_var.str() << ",16)*16;";
      host_allocate->newline();

      host_allocate->indent();
      host_allocate->buffer << globalMemOffset << " += " <<
        malloc_offset_var.str() << ";";
      host_allocate->newline();

      malloc_num++;
    }
    else{
      host_allocate->indent();
      host_allocate->buffer << "cudaMalloc(&" << lhs << ",sizeof(" <<
        elem_type_string  << ")*";
      PrintCDomainSize(expr_domain,host_allocate->buffer);
      host_allocate->buffer << ");" ;
      host_allocate->newline();
      host_allocate->indent();
      host_allocate->buffer << "Check_CUDA_Error(\"Allocation Error!! : " <<
        lhs << "\\n\");" ;
      host_allocate->newline();
      if( init_zero ){
        host_allocate->indent();
        host_allocate->buffer << "cudaMemset(" << lhs << ",0,sizeof(" <<
          elem_type_string << ")*" ;
        PrintCDomainSize(expr_domain,host_allocate->buffer);
        host_allocate->buffer << ");" ;
        host_allocate->newline();
      }
    }
  }

  if( !use_single_malloc ){
    deallocate_buffer->indent();
    deallocate_buffer->buffer << "cudaFree(" << lhs << ");";
    deallocate_buffer->newline();
  }
  return new_var;
}

///-----------------------------------------------------------------------------
bool PrintCUDA::is_number(const string& curr_string) const
{
  int i = 0 ;
  for( ; i < (int)curr_string.size()-1 ; i++ ){
    if( isdigit(curr_string[i]) == 0 && curr_string[i] != '.' )
      return false;
  }
  if( isdigit(curr_string[i]) == 0 && curr_string[i] != 'f' )
    return false;
  return true;
}


///-----------------------------------------------------------------------------
bool PrintCUDA::is_duplicate
(const deque<string>& curr_arg_names, const string& curr_string)
{
  for( int i =0  ; i < (int)curr_arg_names.size() ; i++ ){
    if( curr_arg_names[i].compare(curr_string) == 0 ){
      return true;
    }
  }
  return false;
}

///-----------------------------------------------------------------------------
int texture_reference_info::counter = 0;

///-----------------------------------------------------------------------------
string PrintCUDA::get_texture_type_string(int curr_type)
{
  stringstream curr_stream;
  switch(curr_type){
  case 1:
    curr_stream << "unsigned char" ;
    break;
  case 2:
    curr_stream << "short" ;
    break;
  case 3:
    curr_stream << "int" ;
    break;
  case 4:
    curr_stream << "float" ;
    break;
  case 5:
    curr_stream << "uchar2" ;
    break;
  case 6:
    curr_stream << "short2" ;
    break;
  case 7:
    curr_stream << "int2" ;
    break;
  case 8:
    curr_stream << "float2" ;
    break;
  case 9:
    curr_stream << "uchar4" ;
    break;
  case 10:
    curr_stream << "short4" ;
    break;
  case 11:
    curr_stream << "int4" ;
    break;
  case 12:
    curr_stream << "float4" ;
    break;
  default:
    assert((0) && ("Unknown texture type"));
  }
  return curr_stream.str();
}

///-----------------------------------------------------------------------------
string PrintCUDA::get_channel_string(int curr_type)
{
  stringstream curr_stream;
  switch(curr_type){
  case 1:
    curr_stream << "__channel_uchar__" ;
    break;
  case 2:
    curr_stream << "__channel_short__" ;
    break;
  case 3:
    curr_stream << "__channel_int__" ;
    break;
  case 4:
    curr_stream << "__channel_float__" ;
    break;
  case 5:
    curr_stream << "__channel_uchar2__" ;
    break;
  case 6:
    curr_stream << "__channel_short2__" ;
    break;
  case 7:
    curr_stream << "__channel_int2__" ;
    break;
  case 8:
    curr_stream << "__channel_float2__" ;
    break;
  case 9:
    curr_stream << "__channel_uchar4__" ;
    break;
  case 10:
    curr_stream << "__channel_short4__" ;
    break;
  case 11:
    curr_stream << "__channel_int4__" ;
    break;
  case 12:
    curr_stream << "__channel_float4__" ;
    break;
  default:
    assert((0) && ("Unknown texture type"));
  }
  return curr_stream.str();
}


///-----------------------------------------------------------------------------
void PrintCUDA::init_channels()
{
  for( int i = 1 ; i < TEX_NTYPES ; i++ ){
    file_scope.indent();
    file_scope.buffer << "static cudaChannelFormatDesc "
                      << get_channel_string(i) << " = cudaCreateChannelDesc<"
                      << get_texture_type_string(i) << ">();";
    file_scope.newline();
  }
}


///-----------------------------------------------------------------------------
texture_type PrintCUDA::get_texture_type(const data_types& expr_type) const
{
  int nfields = 0;
  basic_data_types field_type;

  switch(expr_type.type){
  case T_INT8:
  case T_INT16:
  case T_INT:
  case T_FLOAT: {
    nfields = 1;
    field_type = expr_type.type;
  }
    break;
  case T_STRUCT: {
    const defined_data_type* struct_info = expr_type.struct_info;
    ///Struct with 3 fields dont work right now
    if( struct_info->fields.size() == 3 )
      return TEX_NONE;
    deque<defined_fields>::const_iterator it = struct_info->fields.begin();
    field_type = it->field_type;
    if( field_type == T_DOUBLE )
      return TEX_NONE;
    for( it++ ; it != struct_info->fields.end() ; it++ )
      ///If fields arent of the same type, then cant use textures
      if( it->field_type != field_type )
        return TEX_NONE;
    nfields = struct_info->fields.size();
  }
    break;
  default:
    return TEX_NONE;
  }
  texture_type curr_type;
  switch(nfields){
  case 1: {
    switch(field_type){
    case T_INT8:
      curr_type = TEX_UCHAR;
      break;
    case T_INT16:
      curr_type = TEX_SHORT;
      break;
    case T_INT:
      curr_type = TEX_INT;
      break;
    case T_FLOAT:
      curr_type = TEX_FLOAT;
      break;
    default:
      curr_type = TEX_NONE;
    }
  }
    break;
  case 2: {
    switch(field_type){
    case T_INT8:
      curr_type = TEX_UCHAR2;
      break;
    case T_INT16:
      curr_type = TEX_SHORT2;
      break;
    case T_INT:
      curr_type = TEX_INT2;
      break;
    case T_FLOAT:
      curr_type = TEX_FLOAT2;
      break;
    default:
      curr_type = TEX_NONE;
    }
  }
    break;
  case 4:{
    switch(field_type){
    case T_INT8:
      curr_type = TEX_UCHAR4;
      break;
    case T_INT16:
      curr_type = TEX_SHORT4;
      break;
    case T_INT:
      curr_type = TEX_INT4;
      break;
    case T_FLOAT:
      curr_type = TEX_FLOAT4;
      break;
    default:
      curr_type = TEX_NONE;
    }
  }
    break;
  default:
    curr_type = TEX_NONE;
  }

  return curr_type;
}


///-----------------------------------------------------------------------------
///Function to check if a texture can be used with an argument
bool PrintCUDA::isTextureAmenable
(const vector_expr_node* curr_argument, bdy_info* arg_bdy_condn,
 const vector_defn_node* curr_param) const
{
  if( enable_texture ){

    /// Only 1D and 2D textures supported
    if( curr_argument->get_expr_domain() == NULL ||
        curr_argument->get_expr_domain()->get_dim() == 0 ||
        curr_argument->get_expr_domain()->get_dim() > 2 )
      return false;

    if( (arg_bdy_condn->type == B_CONSTANT && arg_bdy_condn->value != 0 ) ||
        get_texture_type(curr_argument->get_data_type()) == TEX_NONE )
      return false;

    bool access_neighbors = false;
    const deque<offset_hull>& stencil_access_info =
      curr_param->get_access_info();
    for( deque<offset_hull>::const_iterator access_iter =
           stencil_access_info.begin(), access_end = stencil_access_info.end();
         access_iter != access_end ; access_iter++ )
      if( access_iter->max_negetive != 0 || access_iter->max_positive != 0 ){
        access_neighbors = true;
        break;
      }
    return access_neighbors;
  }
  return false;
}



///-----------------------------------------------------------------------------
void PrintCUDA::prepare_texture_reference
( c_symbol_info* curr_symbol, const bdy_info* curr_bdy_condn)
{
  assert(dynamic_cast<cuda_symbol_info*>(curr_symbol));
  cuda_symbol_info* curr_cuda_symbol =
    static_cast<cuda_symbol_info*>(curr_symbol);
  texture_type curr_texture_type =
    get_texture_type
    (static_cast<cuda_var_info*>(curr_cuda_symbol->var)->elem_type);
  assert((curr_texture_type != TEX_NONE) &&
         ("Assigning texture to a non texturizable object"));

  int curr_ndims = curr_symbol->var->expr_domain->get_dim();
  if( curr_ndims <= 2 ){
    ///Currently only 1D and 2D textures handled

    // printf("Variable %s mapped to
    // texture\n",curr_symbol->var->symbol_name.c_str());
    texture_reference_info* curr_bound_texture = NULL;
    for(deque<texture_reference_info*>::iterator it =texture_references.begin();
        it != texture_references.end() ; it++ )
      if( (*it)->curr_bound_var == NULL &&
          (*it)->texture_data_type == curr_texture_type &&
          (*it)->ndims == curr_ndims) {
        curr_bound_texture = (*it);
        break;
      }
    if( curr_bound_texture == NULL ){
      curr_bound_texture =
        new texture_reference_info
        (curr_ndims,curr_symbol->var->elem_type,curr_texture_type);
      // printf("New Texture of dimension %d, type :
      // %d\n",curr_ndims,curr_texture_type);
      texture_references.push_back(curr_bound_texture);
      file_scope.indent();
      file_scope.buffer << "static texture<"
                        << get_texture_type_string(curr_texture_type)
                        << ",cudaTextureType" << curr_ndims << "D> "
                        << curr_bound_texture->name << ";";
      file_scope.newline();

    }
    curr_cuda_symbol->texture_info = curr_bound_texture;
    curr_bound_texture->curr_bound_var =
      static_cast<cuda_var_info*>(curr_cuda_symbol->var);
    kernel_launch.indent();
    if( curr_ndims == 1 ){
      kernel_launch.buffer << "cudaBindTexture(NULL,&"
                           << curr_bound_texture->name << ","
                           <<  curr_symbol->var->symbol_name << ",&"
                           << get_channel_string(curr_texture_type) << "," ;
      PrintCDomainSize(curr_symbol->var->expr_domain,kernel_launch.buffer);
      kernel_launch.buffer << " * sizeof("
                           << get_string(curr_symbol->var->elem_type) << ")";
    }
    else{
      cuda_var_info* curr_cuda_var =
        static_cast<cuda_var_info*>(curr_symbol->var);
      assert(curr_cuda_var && curr_cuda_var->pitch.compare("") != 0 );
      kernel_launch.buffer << "cudaBindTexture2D(NULL,&"
                           << curr_bound_texture->name << ","
                           <<  curr_symbol->var->symbol_name << ",&"
                           << get_channel_string(curr_texture_type) << "," ;
      const domain_node* curr_expr_domain = curr_symbol->var->expr_domain;
      deque<range_coeffs>::const_reverse_iterator it =
        curr_expr_domain->get_domain().rbegin();
      kernel_launch.buffer << "(";
      PrintCParametricExpr(it->ub,kernel_launch.buffer);
      kernel_launch.buffer << "-" ;
      PrintCParametricExpr(it->lb,kernel_launch.buffer);
      kernel_launch.buffer << "+1),";
      it++;
      kernel_launch.buffer << "(";
      PrintCParametricExpr(it->ub,kernel_launch.buffer);
      kernel_launch.buffer << "-" ;
      PrintCParametricExpr(it->lb,kernel_launch.buffer);
      kernel_launch.buffer << "+1)";
      for( it++ ; it != curr_expr_domain->get_domain().rend() ; it++ ){
        kernel_launch.buffer << "*(";
        PrintCParametricExpr(it->ub,kernel_launch.buffer);
        kernel_launch.buffer << "-" ;
        PrintCParametricExpr(it->lb,kernel_launch.buffer);
        kernel_launch.buffer << "+1)";
      }
      kernel_launch.buffer << ",";
      kernel_launch.buffer << curr_cuda_var->pitch;
      kernel_launch.buffer << "*sizeof(";
      kernel_launch.buffer << get_string(curr_cuda_var->elem_type);
      kernel_launch.buffer <<")";
    }
    kernel_launch.buffer << ");" ;
    kernel_launch.newline();
    kernel_launch.indent();
    kernel_launch.buffer <<
      "Check_CUDA_Error(\"Texture Bind Error!! Error while binding Texture " <<
      curr_bound_texture->name <<  " to var " <<
      curr_symbol->var->symbol_name <<  "\\n\");" ;
    kernel_launch.newline();

    switch(curr_bdy_condn->type){
    case B_CONSTANT: {
      assert((curr_bdy_condn->value == 0) &&
             ("Use of texture with non-zero constant  invalid usage"));
      for( int i = 0 ; i < curr_ndims ; i++ ){
        kernel_launch.indent();
        kernel_launch.buffer << curr_bound_texture->name << ".addressMode["
                             << i << "] = cudaAddressModeBorder;";
        kernel_launch.newline();
      }
    }
      break;
    case B_WRAP: {
      for( int i = 0 ; i < curr_ndims ; i++ ){
        kernel_launch.indent();
        kernel_launch.buffer << curr_bound_texture->name << ".addressMode["
                             << i << "] = cudaAddressModeWrap;";
        kernel_launch.newline();
      }
      kernel_launch.indent();
      kernel_launch.buffer << curr_bound_texture->name << ".normalized = true;";
      kernel_launch.newline();
      curr_cuda_symbol->is_texture_normalized = true;
    }
      break;
    case B_MIRROR: {
      for( int i = 0 ; i < curr_ndims ; i++ ){
        kernel_launch.indent();
        kernel_launch.buffer << curr_bound_texture->name << ".addressMode["
                             << i << "] = cudaAddressModeMirror;";
        kernel_launch.newline();
      }
      kernel_launch.indent();
      kernel_launch.buffer << curr_bound_texture->name << ".normalized = true;";
      kernel_launch.newline();
      curr_cuda_symbol->is_texture_normalized = true;
    }
    default:
      break;
    }
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_array_access
(const c_var_info* curr_var, const deque<string>& access_exp,
 stringstream& curr_stream)
{
  assert(dynamic_cast<const cuda_var_info*>(curr_var));
  const cuda_var_info* curr_cuda_var =
    static_cast<const cuda_var_info*>(curr_var);
  if( curr_cuda_var->pitch.compare("") == 0 ){
    PrintC::print_array_access(curr_var,access_exp,curr_stream);
  }
  else{
    assert
      ((curr_var->expr_domain->get_dim() == (int)access_exp.size()) &&
       "[ME] WIthin code-generator dimension of expr and accesses dont match");
    deque<string>::const_reverse_iterator it = access_exp.rbegin();
    curr_stream << "[" << (*it) << "+ " << curr_cuda_var->pitch << "*(" ;
    it++;
    curr_stream << (*it) ;
    deque<range_coeffs>::const_reverse_iterator jt =
      curr_var->expr_domain->range_list.rbegin()+1 ;
    for( it++ ; it != access_exp.rend() ; it++,jt++){
      curr_stream << "+";
      parametric_exp* curr_size = parametric_exp::copy(jt->ub);
      curr_size = curr_size->subtract(jt->lb);
      curr_size = curr_size->add(1);
      PrintCParametricExpr(curr_size,curr_stream);
      curr_stream << "*" << "(" << (*it);
      delete curr_size;
    }
    for( int i = 0 ; i < (int)access_exp.size()-1 ; i++ )
      curr_stream << ")";
    curr_stream << "]";
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_texture_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 stringstream& curr_stream, bool is_texture_normalized)
{
  // int dim = iters.size() - scale_domain->get_dim();
  // assert(dim ==  0);
  assert((int)iters.size() == scale_domain->get_dim());
  assert(scale_domain->get_dim() == curr_var->expr_domain->get_dim());
  int dim = iters.size() - 1;
  deque<string>access_exp;
  deque<range_coeffs>::const_reverse_iterator jt =
    curr_var->expr_domain->get_domain().rbegin();
  for( deque<scale_coeffs>::const_reverse_iterator it =
         scale_domain->scale_fn.rbegin() ; it != scale_domain->scale_fn.rend() ;
       it++,dim--,jt++ ){
    curr_stream << "," ;
    if( is_texture_normalized )
      curr_stream << "(float)(" ;
    curr_stream  << "(" << iters[dim]->name  << ")" ;
    if( it->scale != 1 )
      curr_stream << "*" << it->scale;
    if( it->offset != 0 )
      curr_stream << "+(" << it->offset <<")";
    if( is_texture_normalized ){
      curr_stream << ")/(float)";
      parametric_exp* curr_size = parametric_exp::copy(jt->ub);
      curr_size->subtract(jt->lb);
      curr_size->add(1);
      PrintCParametricExpr(curr_size,curr_stream);
      delete curr_size;
    }
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_texture_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 const domain_node* sub_domain, stringstream& curr_stream,
 bool is_texture_normalized)
{
  // int dim = iters.size() - scale_domain->get_dim();
  // assert(dim ==  0);
  assert((int)iters.size() == scale_domain->get_dim());
  assert(scale_domain->get_dim() == sub_domain->get_dim());
  assert(curr_var->expr_domain->get_dim() == scale_domain->get_dim());
  int dim = iters.size() - 1 ;
  deque<string>access_exp;
  deque<scale_coeffs>::const_reverse_iterator it =
    scale_domain->scale_fn.rbegin() ;
  deque<range_coeffs>::const_reverse_iterator jt =
    sub_domain->range_list.rbegin();
  deque<range_coeffs>::const_reverse_iterator kt =
    curr_var->expr_domain->range_list.rbegin();
  for( ; it != scale_domain->scale_fn.rend() ; it++,jt++,dim-- ){
    stringstream new_stream;
    curr_stream << "," ;
    if( is_texture_normalized )
      curr_stream << "(float)(";
    curr_stream << iters[dim]->name;
    if( it->scale != 1 )
      curr_stream << "*" << it->scale;
    if( it->offset != 0 )
      curr_stream << "+(" << it->offset <<")";
    if( !jt->lb->is_default() ){
      curr_stream << "+" ;
      PrintCParametricExpr(jt->lb,curr_stream);
    }
    if( is_texture_normalized ){
      curr_stream << ")/(float)";
      parametric_exp* curr_size = parametric_exp::copy(jt->ub);
      curr_size->subtract(jt->lb);
      curr_size->add(1);
      PrintCParametricExpr(curr_size,curr_stream);
      delete curr_size;
    }
  }
}

///-----------------------------------------------------------------------------
/// Helper function to load using stencil operation
string PrintCUDA::print_stencil_op_helper
(const c_symbol_info* curr_symbol, const domainfn_node* scale_fn,
 const data_types& base_data_type, int field_num, bool handle_bdys)
{
  stringstream curr_stream;
  assert
    (dynamic_cast<const cuda_symbol_info*>(curr_symbol) &&
     "Variable not a cuda symbol in cuda codegenerator");
  const cuda_symbol_info* curr_cuda_symbol =
    static_cast< const cuda_symbol_info*>(curr_symbol);

  texture_reference_info* curr_texture_ref = curr_cuda_symbol->texture_info;
  if( curr_texture_ref ){
    curr_stream << "tex" << curr_texture_ref->ndims << "D("
                << curr_texture_ref->name;
    if( curr_cuda_symbol->offset_domain )
      print_texture_point
        (curr_cuda_symbol->var,scale_fn,
         curr_cuda_symbol->offset_domain,curr_stream,
         curr_cuda_symbol->is_texture_normalized);
    else
      print_texture_point
        (curr_cuda_symbol->var,scale_fn,curr_stream,
         curr_cuda_symbol->is_texture_normalized);
    curr_stream << ")";
    if( base_data_type.type == T_STRUCT ){
      if( field_num != -1 )
        switch(field_num){
        case 0:
          curr_stream << ".x" ;
          break;
        case 1:
          curr_stream << ".y" ;
          break;
        case 2:
          curr_stream << ".z";
          break;
        case 3:
          curr_stream << ".w";
          break;
        default:
          assert((0) && ("Error while accessing fields of textured data type"));
        }
    }
    return curr_stream.str();
  }
  else{
    return
      PrintC::print_stencil_op_helper
      (curr_symbol,scale_fn,base_data_type,field_num,handle_bdys);
  }
}

///-----------------------------------------------------------------------------
/// Initialize the actual block dimension variables, same as the block of the
/// output computed by the kernel
void PrintCUDA::InitializeOutputBlockSizeParams
(int grid_dim, stringBuffer* buffer, const deque<int>& curr_unroll_factors)
{
  ///Reduce the block dimensions by the "max halo size";
  deque<offset_hull> halo;

  for( int i = 0 ; i < grid_dim ; i++ ){
    buffer->indent();
    buffer->buffer << "int ";
    switch(grid_dim-1-i){
    case 0:
      buffer->buffer << "FORMA_BLOCKDIM_X = (int)(blockDim.x)";
      break;
    case 1:
      buffer->buffer << "FORMA_BLOCKDIM_Y = (int)(blockDim.y)";
      break;
    case 2:
      buffer->buffer << "FORMA_BLOCKDIM_Z = (int)(blockDim.z)";
      break;
    default:
      assert("Grid dimension specified is greater than that handled");
    }
    buffer->buffer << ";";
    buffer->newline();
  }
  for (int i = 0 ; i < grid_dim ; i++) {
    int num_unroll_factors = curr_unroll_factors.size();
    int curr_unroll_factor =
      curr_unroll_factors[num_unroll_factors - grid_dim + i];
    if (curr_unroll_factor > 1) {
      buffer->indent();
      switch(grid_dim-1-i) {
        case 0:
          buffer->buffer << "FORMA_BLOCKDIM_X";
          break;
        case 1:
          buffer->buffer << "FORMA_BLOCKDIM_Y";
          break;
        case 2:
          buffer->buffer << "FORMA_BLOCKDIM_Z";
          break;
      default:
        assert("Grid dimension specified is greater than that handled");
      }
      buffer->buffer << " *= " << curr_unroll_factor << ";";
      buffer->newline();
    }
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_kernel_signature
(deque<c_symbol_info*>& input_exprs, const c_var_info* output_var,
 const string& kernel_name)
{
  output_buffer->indent();
  output_buffer->buffer << "__global__ void " << kernel_name << "(";
  deque<string> curr_arg_names;
  int nscalar_args = 0;
  for( deque<c_symbol_info*>::iterator it = input_exprs.begin();
       it != input_exprs.end() ; it++ ){
    const c_symbol_info* curr_symbol = (*it);
    const cuda_symbol_info* curr_cuda_symbol =
      static_cast<const cuda_symbol_info*>(curr_symbol);
    if( curr_cuda_symbol->texture_info )
      continue;
    if( curr_symbol->var->expr_domain == NULL ){
      string curr_symbol_name;
      if( is_number(curr_symbol->var->symbol_name ) ){
        stringstream curr_scalar_arg_stream;
        curr_scalar_arg_stream << "__scalar_arg_" << nscalar_args++ << "__";
        curr_symbol_name = curr_scalar_arg_stream.str();
      }
      else
        curr_symbol_name = curr_symbol->var->symbol_name ;

      if( !is_duplicate(curr_arg_names,curr_symbol_name) ){
        output_buffer->buffer << get_string(curr_symbol->var->elem_type)
                              << " " << curr_symbol_name  << ", ";
        curr_arg_names.push_back(curr_symbol_name);
      }
    }
    else{
      if( !is_duplicate(curr_arg_names,curr_symbol->var->symbol_name ) ){
        output_buffer->buffer << get_string(curr_symbol->var->elem_type) <<
          " * __restrict__ "  << curr_symbol->var->symbol_name << ", ";
        cuda_var_info* curr_cuda_var =
          static_cast<cuda_var_info*>(curr_symbol->var);
        if( curr_cuda_var->pitch.compare("") != 0 ){
          output_buffer->buffer << "size_t " << curr_cuda_var->pitch << ", ";
        }
        curr_arg_names.push_back(curr_symbol->var->symbol_name);
      }
    }
  }

  ///Add parameters as arguments
  for( deque<pair<string,parameter_defn*> >::const_iterator it =
         global_params->begin() ; it != global_params->end() ; it++ ){
    output_buffer->buffer << "int " << it->first << ", ";
  }
  output_buffer->buffer << get_string(output_var->elem_type) << " * "
                       <<  output_var->symbol_name ;
  const cuda_var_info* cuda_output_var =
    static_cast<const cuda_var_info*>(output_var);
  if( cuda_output_var->pitch.compare("") != 0 ){
    output_buffer->buffer << ", size_t " << cuda_output_var->pitch ;
  }
  output_buffer->buffer << "){";
  output_buffer->newline();
  output_buffer->increaseIndent();
}

///-----------------------------------------------------------------------------
void PrintCUDA::print_kernel_header
(int grid_dim, const deque<int>& curr_unroll_factors) {
  /// Initialize the "actual" block size
  InitializeOutputBlockSizeParams
    (grid_dim, output_buffer, curr_unroll_factors);
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_kernel_launch
(int grid_dim, const domain_node* curr_domain,
 deque<c_symbol_info*>& input_exprs, const c_var_info* output_var,
 const string& kernel_name, const deque<int>& curr_unroll_factors)
{
  assert(grid_dim <= MAX_GRID_DIM &&
         "Launch dimension greater than currently supported");
  assert( curr_domain->range_list.size() >= grid_dim &&
          "Launch dimension smaller than current domain size");

  /// Compute the size of the halo if any
  deque<offset_hull> halo;

  /// Compute the size of the domain
  const deque<range_coeffs>& range_list = curr_domain->range_list;
  int curr_dim = 0 ;
  deque<string> loop_dimensions;
  for(deque<range_coeffs>::const_reverse_iterator dimIter = range_list.rbegin();
      curr_dim < grid_dim ; dimIter++, curr_dim++){
    stringstream loop_dim_size;
    loop_dim_size << "__size_" << curr_dim << "_" << kernel_name ;
    loop_dimensions.push_back(loop_dim_size.str());

    kernel_launch.indent();
    kernel_launch.buffer << "int " << loop_dim_size.str() << " = (";
    PrintCParametricExpr(dimIter->ub,kernel_launch.buffer) ;
    kernel_launch.buffer << " - " ;
    PrintCParametricExpr(dimIter->lb,kernel_launch.buffer);
    kernel_launch.buffer << " ) + 1;";
    kernel_launch.newline();
  }

  /// Compute the max block size for this kernel
  stringstream max_block_size;
  max_block_size << "__max_occupancy_blocksize_" << kernel_name;
  kernel_launch.indent();
  kernel_launch.buffer << "int " << max_block_size.str() << ";";
  kernel_launch.newline();
  stringstream min_grid_size;
  min_grid_size << "_max_occupancy_gridsize_" << kernel_name;
  kernel_launch.indent();
  kernel_launch.buffer << "int " << min_grid_size.str() << ";";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaOccupancyMaxPotentialBlockSize(&" <<
    min_grid_size.str() << ",&" << max_block_size.str() << ",(const void*)" <<
    kernel_name << ",0,0);";
  kernel_launch.newline();

  deque<string> block_dimensions;
  /// Block dimensions are a minimum of (1,1,32) and
  /// (FORMA_MAX_BLOCKDIM_1,FORMA_MAX_BLOCKDIM_0)
  for( int i = 0 ; i < grid_dim ; i++ ){

    int num_unroll_factors = curr_unroll_factors.size();
    int curr_unroll_factor = curr_unroll_factors[num_unroll_factors-1-i];

    stringstream max_block_size_dim;
    max_block_size_dim << max_block_size.str() << "_" << i;
    kernel_launch.indent();
    if (i != grid_dim - 1)
      kernel_launch.buffer << "int " << max_block_size_dim.str() <<
        " = pow((double)" << max_block_size.str() <<
        ", (double)(1.0/(double)" << (grid_dim-i) << "));";
    else
      kernel_launch.buffer << "int " << max_block_size_dim.str() << " = " <<
        max_block_size.str() << ";";
    kernel_launch.newline();

    if (i == 0) {
      /// Get max block size to multiple of warp size
      kernel_launch.indent();
      kernel_launch.buffer << max_block_size_dim.str() << " = FORMA_MAX(" <<
        max_block_size_dim.str() << "/32, 1)*32;";
      kernel_launch.newline();
    }

    stringstream block_dim_size;
    block_dim_size << "__block_" << i << "_" << kernel_name ;
    block_dimensions.push_back(block_dim_size.str());

    kernel_launch.indent();
    kernel_launch.buffer << "int " << block_dim_size.str() << " = " ;
    if( i == 0 ){
      kernel_launch.buffer<< "FORMA_MAX(FORMA_MIN(FORMA_MIN(" <<
        max_block_size_dim.str() << ",FORMA_MAX((" << loop_dimensions[i] <<
        "/" << curr_unroll_factor <<")/32,1)*32" << "),FORMA_MAX_BLOCKDIM_" <<
        i << ")," ;
      kernel_launch.buffer << "1";
      kernel_launch.buffer << ");";
    }
    else{
      kernel_launch.buffer << "FORMA_MAX(FORMA_MIN(FORMA_MIN(" <<
        max_block_size_dim.str() << "," << loop_dimensions[i] << "/" <<
        curr_unroll_factor << "),FORMA_MAX_BLOCKDIM_" << i << ")," ;
      kernel_launch.buffer << "1";
      kernel_launch.buffer << ");";
    }
    kernel_launch.newline();

    kernel_launch.indent();
    kernel_launch.buffer << max_block_size.str() << " /= " <<
      block_dim_size.str() <<";";
    kernel_launch.newline();
  }

  stringstream blockConfig;
  blockConfig << "__blockConfig_" << kernel_name;
  kernel_launch.indent();
  kernel_launch.buffer << "dim3 " << blockConfig.str() << "(";
  for( int i = 0 ; i < grid_dim ; i++ ){
    kernel_launch.buffer << block_dimensions[i] ;
    if( i != grid_dim -1 )
      kernel_launch.buffer << ",";
  }
  kernel_launch.buffer << ");";
  kernel_launch.newline();


  /// Shared memory size;
  stringstream shared_memory_size;
  shared_memory_size << "__SMemSize_" << kernel_name ;
  kernel_launch.indent();
  kernel_launch.buffer << "int " << shared_memory_size.str() << " = 0;";
  kernel_launch.newline();

  deque<string> grid_dimensions;
  ///Grid dimensions - FIXME :: Doesnt account for  device limits
  for( int i = 0 ; i < grid_dim ; i++ ){
    int num_unroll_factors = curr_unroll_factors.size();
    int curr_unroll_factor = curr_unroll_factors[num_unroll_factors-1-i];

    stringstream grid_dim_size;
    grid_dim_size << "__grid_" << i << "_" << kernel_name;
    grid_dimensions.push_back(grid_dim_size.str());

    kernel_launch.indent();
    kernel_launch.buffer << "int " << grid_dim_size.str() << " = ";
    kernel_launch.buffer<< "FORMA_CEIL(" << loop_dimensions[i] << "," <<
      block_dimensions[i] << "*" << curr_unroll_factor << ");";
    kernel_launch.newline();
  }
  stringstream gridConfig;
  gridConfig << "__gridConfig_" << kernel_name ;
  kernel_launch.indent();
  kernel_launch.buffer << "dim3 " << gridConfig.str() << "(";
  for( int i = 0 ; i < grid_dim ; i++) {
    kernel_launch.buffer << grid_dimensions[i];
    if( i != grid_dim-1 )
      kernel_launch.buffer << ",";
  }
  kernel_launch.buffer << ");";
  kernel_launch.newline();

  kernel_launch.indent();
  kernel_launch.buffer <<kernel_name <<"<<<" << gridConfig.str() << "," <<
    blockConfig.str() << "," << shared_memory_size.str() << ">>>(";

  deque<string> curr_arg_names;
  for( deque<c_symbol_info*>::iterator it = input_exprs.begin() ;
       it != input_exprs.end() ; it++ ){
    const c_symbol_info* curr_symbol = (*it);
    assert(dynamic_cast<const cuda_symbol_info*>(curr_symbol));
    const cuda_symbol_info* curr_cuda_symbol =
      static_cast<const cuda_symbol_info*>(curr_symbol);
    if( curr_cuda_symbol->texture_info == NULL )
      if( !is_duplicate(curr_arg_names,curr_symbol->var->symbol_name ) ){
        kernel_launch.buffer <<  curr_symbol->var->symbol_name << ", ";
        cuda_var_info* curr_cuda_var =
          static_cast<cuda_var_info*>(curr_symbol->var);
        if( curr_cuda_var->pitch.compare("")!= 0){
          kernel_launch.buffer << curr_cuda_var->pitch <<", ";
        }
        curr_arg_names.push_back(curr_symbol->var->symbol_name);
      }
  }


  for( deque<pair<string,parameter_defn*> >::const_iterator
         it = global_params->begin() ; it != global_params->end() ; it++ ){
    kernel_launch.indent();
    kernel_launch.buffer <<  it->first << ",";
  }

  kernel_launch.buffer << output_var->symbol_name ;
  const cuda_var_info* cuda_output_var =
    static_cast<const cuda_var_info*>(output_var);
  if( cuda_output_var->pitch.compare("") != 0 ){
    kernel_launch.buffer << "," << cuda_output_var->pitch;
  }
  kernel_launch.buffer << ");";
  kernel_launch.newline();

  kernel_launch.indent();
  kernel_launch.buffer << "Check_CUDA_Error(\"Kernel Launch Error!! : "
                       << kernel_name << "\\n\");" ;
  kernel_launch.newline();
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_block_loop
(string& iterator, int curr_dim, int grid_dim, parametric_exp* curr_lb,
 parametric_exp* curr_ub, int unrollFactor)
{
  if( curr_dim == 0 ){
    output_buffer->indent();
    output_buffer->buffer << iterator <<
      " = (int)(blockIdx.x)*(int)(FORMA_BLOCKDIM_X) + " ;
    output_buffer->buffer << "(int)(threadIdx.x";
    // if (unrollFactor > 1)
    //   output_buffer->buffer << "*" << unrollFactor;
    output_buffer->buffer << ") + ";
    PrintCParametricExpr(curr_lb,output_buffer->buffer);
    output_buffer->buffer << ";";
    output_buffer->newline();
  }
  else if (curr_dim == 1){
    output_buffer->indent();
    output_buffer->buffer << iterator <<
      " = (int)(blockIdx.y)*(int)(FORMA_BLOCKDIM_Y) + ";
    output_buffer->buffer << "(int)(threadIdx.y";
    if (unrollFactor > 1)
      output_buffer->buffer << "*" << unrollFactor;
    output_buffer->buffer << ") + ";
    PrintCParametricExpr(curr_lb,output_buffer->buffer);
    output_buffer->buffer << ";";
    output_buffer->newline();
  }
  else if (curr_dim == 2){
    output_buffer->indent();
    output_buffer->buffer << iterator <<
      " = (int)(blockIdx.z)*(int)(FORMA_BLOCKDIM_Z) + ";
    output_buffer->buffer << "(int)(threadIdx.z";
    if (unrollFactor > 1)
      output_buffer->buffer << "*" << unrollFactor;
    output_buffer->buffer << ") + ";
    PrintCParametricExpr(curr_lb,output_buffer->buffer);
    output_buffer->buffer << ";";
    output_buffer->newline();
  }
  else {
    assert(0 && "Invalid grid dimension");
  }

  output_buffer->indent();
  output_buffer->buffer << "if(" << iterator  << " <= ";
  PrintCParametricExpr(curr_ub,output_buffer->buffer);
  output_buffer->buffer << "){" ;
  output_buffer->newline();
  output_buffer->increaseIndent();
  launch_iters.push_back
    (new c_iterator
     (iterator,parametric_exp::copy(curr_lb),parametric_exp::copy(curr_ub)));
}

///-----------------------------------------------------------------------------
/// Function that decides if the current dimension is expanded or not
bool PrintCUDA::isExpandedDimension
(const offset_hull& offsetInfo, int dim, int loop_dim) {
  if (offsetInfo.scale != 1)
    return false;
  if (MAX_GRID_DIM > 2 && loop_dim - dim == MAX_GRID_DIM) {
    int netOffset = offsetInfo.max_positive - offsetInfo.max_negetive;
    if (netOffset > 2)
      return false;
  }
  return true;
}

///-----------------------------------------------------------------------------
/// Overloaded method to generate kernel launches that iterate over the domain
/// when a scaling function is used at the output, generates both the kernel
/// function and the kernel launch
int PrintCUDA::print_domain_loops_header
(const domain_node* curr_domain, deque<c_symbol_info*>& input_exprs,
 const c_var_info* output_var, const domainfn_node* curr_domainfn,
 const deque<int>& curr_unroll_factors)
{
  string kernel_name = get_new_kernel_var();
  domain_node* loop_domain = compute_loop_domain(curr_domain, curr_domainfn);

  const deque<range_coeffs>& range_list = loop_domain->range_list;
  int grid_dim =
    (range_list.size() < MAX_GRID_DIM ? range_list.size() : MAX_GRID_DIM);
  int n_thread_loops = range_list.size() - MAX_GRID_DIM;
  n_thread_loops = (n_thread_loops < 0 ? 0 : n_thread_loops);

  print_kernel_signature(input_exprs, output_var, kernel_name);
  print_kernel_header(grid_dim, curr_unroll_factors);

  int curr_dim = 0;
  deque<range_coeffs>::const_reverse_iterator it = range_list.rbegin();
  for(it = range_list.rbegin();
      it != range_list.rend() && curr_dim < grid_dim; it++, curr_dim++){
    string new_var = get_new_iterator(output_buffer);
    print_block_loop(new_var, curr_dim, grid_dim, it->lb, it->ub, 1);
  }

  curr_dim = 0;
  for(deque<range_coeffs>::const_iterator jt = range_list.begin();
      jt != range_list.end() ; jt++ ){
    if( curr_dim >= n_thread_loops )
      break;
    c_iterator* new_iter = printForHeader(jt->lb, jt->ub, 1, false);
    iters.push_back(new_iter);
    curr_dim++;
  }

  for (deque<c_iterator*>::reverse_iterator
         launch_iter_rbegin = launch_iters.rbegin(),
         launch_iter_rend = launch_iters.rend();
       launch_iter_rbegin != launch_iter_rend; launch_iter_rbegin++)
    iters.push_back(*launch_iter_rbegin);
  launch_iters.clear();

  print_kernel_launch
    (grid_dim, curr_domain, input_exprs, output_var, kernel_name,
     curr_unroll_factors);

  delete loop_domain;
  return 0;
}


///-----------------------------------------------------------------------------
/// Generate the body of the unrolled code for cuda programs, same as the
/// print_unrolled_code_body of C but specialized for
void PrintCUDA::print_unrolled_code_body_helper
(int loop_dim, int num_loop_dims, deque<int>& curr_unroll_factors,
 const fnid_expr_node* curr_fn, deque<c_symbol_info*>& input_exprs,
 c_symbol_info* output_symbol, bool is_bdy)
{
  if (loop_dim < num_loop_dims) {
    int num_unroll_generated = 0;
    c_iterator* curr_iterator = iters[iters.size() - num_loop_dims + loop_dim];
    std::string orig_iterator_name = curr_iterator->name;
    do {
      std::stringstream next_iterator;
      next_iterator << "(" << orig_iterator_name << "+";
      if (curr_unroll_factors[loop_dim] > 1 && num_unroll_generated != 0 &&
          loop_dim == num_loop_dims-1)
        next_iterator << " (FORMA_BLOCKDIM_X / " <<
          curr_unroll_factors[loop_dim] << ")  * " << num_unroll_generated;
      else
        next_iterator <<  num_unroll_generated;
      next_iterator << ")";
      curr_iterator->name = next_iterator.str();
      print_unrolled_code_body_helper
        (loop_dim+1, num_loop_dims, curr_unroll_factors, curr_fn, input_exprs,
         output_symbol, is_bdy);
      num_unroll_generated++;
    } while(num_unroll_generated < curr_unroll_factors[loop_dim]);
    curr_iterator->name = orig_iterator_name;
  }
  else {
    print_stencilfn_body(curr_fn, input_exprs, output_symbol, is_bdy);
  }
}


///-----------------------------------------------------------------------------
/// Generate the body of the unrolled code
void PrintCUDA::print_unrolled_code_body
(int loop_dim, int num_loop_dims, deque<int>& curr_unroll_factors,
 const fnid_expr_node* curr_fn, deque<c_symbol_info*>& input_exprs,
 c_symbol_info* output_symbol, bool is_bdy)
{
  /// Re-arrange iterators to put the launch iterators at the end of iterator
  /// list
  int num_launch_iters = launch_iters.size();
  for (deque<c_iterator*>::reverse_iterator
         launch_iter_rbegin = launch_iters.rbegin(),
         launch_iter_rend = launch_iters.rend();
       launch_iter_rbegin != launch_iter_rend; launch_iter_rbegin++)
    iters.push_back(*launch_iter_rbegin);
  launch_iters.clear();
  print_unrolled_code_body_helper
    (loop_dim, num_loop_dims, curr_unroll_factors, curr_fn, input_exprs,
     output_symbol, is_bdy);

  ///Remove the launch iterators from the iterator list
  for (int i = 0; i < num_launch_iters; i++) {
    c_iterator* curr_launch_iter = iters.back();
    launch_iters.push_back(curr_launch_iter);
    iters.pop_back();
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_unrolled_code_helper
(int curr_dim, int grid_dim, const fnid_expr_node* curr_fn,
 deque<c_symbol_info*>& input_exprs, c_symbol_info* output_symbol,
 domain_node* loop_domain, bool is_bdy, deque<int>& curr_unroll_factors)
{
  const deque<range_coeffs>& range_list = loop_domain->range_list;
  if (curr_dim >= (int)range_list.size() - grid_dim) {
    int curr_unroll_factor = curr_unroll_factors[curr_dim];

    /// Print the block loop for this dimension
    string new_var = get_new_iterator(output_buffer);
    print_block_loop
      (new_var, range_list.size()-1-curr_dim, grid_dim, range_list[curr_dim].lb,
       range_list[curr_dim].ub, curr_unroll_factor);

    if (curr_unroll_factor > 1 ) {
      output_buffer->indent();
      output_buffer->buffer << "if(" << new_var << " + ";
      if (curr_dim == range_list.size()-1 )
        output_buffer->buffer << " (FORMA_BLOCKDIM_X / " <<
          curr_unroll_factor << ") * " << curr_unroll_factor -1 ;
      else
        output_buffer->buffer << curr_unroll_factor - 1;
      output_buffer->buffer << " <= ";
      PrintCParametricExpr(range_list[curr_dim].ub, output_buffer->buffer);
      output_buffer->buffer << "){";
      output_buffer->newline();
      output_buffer->increaseIndent();
    }

    /// Notice the wierd recursion pattern since the innermost dimensions are
    /// the launch dimensions
    print_unrolled_code_helper
      (curr_dim-1, grid_dim, curr_fn, input_exprs, output_symbol, loop_domain,
       is_bdy, curr_unroll_factors);

    /// Generate the remainder
    if (curr_unroll_factor > 1) {
      /// Finalize the if block
      output_buffer->decreaseIndent();
      output_buffer->indent();
      output_buffer->buffer << "}";
      output_buffer->newline();
      output_buffer->indent();
      output_buffer->buffer << "else {";
      output_buffer->newline();
      output_buffer->increaseIndent();

      /// Loop for the remainder since we dont know how much is remainder
      print_remainder_loop_header
        (new_var, range_list[curr_dim].ub, curr_unroll_factor);

      /// Recurse for the rest of the code
      curr_unroll_factors[curr_dim] = 1;
      print_unrolled_code_helper
        (curr_dim-1, grid_dim, curr_fn, input_exprs, output_symbol, loop_domain,
         is_bdy, curr_unroll_factors);

      /// Finish the remainder loop
      print_remainder_loop_footer();
      curr_unroll_factors[curr_dim] = curr_unroll_factor;

      /// Finish the else block
      output_buffer->decreaseIndent();
      output_buffer->indent();
      output_buffer->buffer << "}";
      output_buffer->newline();
    }
    /// Finalize the block loops
    output_buffer->decreaseIndent();
    output_buffer->indent();
    output_buffer->buffer << "}";
    output_buffer->newline();
    c_iterator* curr_iterator = launch_iters.back();
    launch_iters.pop_back();
    delete curr_iterator;
  }
  else{
    PrintC::print_unrolled_code
      (0, range_list.size() - grid_dim, curr_fn, input_exprs, output_symbol,
       loop_domain, is_bdy, curr_unroll_factors);
  }
}

///-----------------------------------------------------------------------------
/// Generate unrolled code for CUDA kernels
void PrintCUDA::print_unrolled_code
(int curr_dim, int loop_dim, const fnid_expr_node* curr_fn,
 deque<c_symbol_info*>& input_exprs, c_symbol_info* output_symbol,
 domain_node* loop_domain, bool is_bdy, deque<int>& curr_unroll_factors)
{
  assert
    (curr_dim == 0 && "Wrong control flow if reach here with curr_dim != 0");

  /// Initialize the kernel code
  string kernel_name = get_new_kernel_var();

  int grid_dim = (loop_dim < MAX_GRID_DIM ? loop_dim : MAX_GRID_DIM);
  int n_thread_loops = loop_dim - grid_dim;
  n_thread_loops = (n_thread_loops < 0 ? 0 : n_thread_loops);

  print_kernel_signature(input_exprs, output_symbol->var, kernel_name);
  InitializeOutputBlockSizeParams
    (grid_dim, output_buffer, curr_unroll_factors);

  print_unrolled_code_helper
    (loop_dim-1, grid_dim, curr_fn, input_exprs, output_symbol, loop_domain,
     is_bdy, curr_unroll_factors);

  output_buffer->decreaseIndent();
  output_buffer->indent();
  output_buffer->buffer << "}";
  output_buffer->newline();

  print_kernel_launch
    (grid_dim, loop_domain, input_exprs, output_symbol->var, kernel_name,
     curr_unroll_factors);
}

///-----------------------------------------------------------------------------
void PrintCUDA::print_domain_loops_footer
(const domain_node* loop_domain, const deque<int>& curr_unroll_factors)
{
  assert(loop_domain->get_dim() == (int)iters.size());
  PrintC::print_domain_loops_footer(loop_domain, curr_unroll_factors);
  output_buffer->decreaseIndent();
  output_buffer->indent();
  output_buffer->buffer << "}";
  output_buffer->newline();
}


///-----------------------------------------------------------------------------
/// Function that initializes arguments, maps symbols to textures if possible
void PrintCUDA::init_arg_info
(const vector_expr_node* curr_argument, bdy_info* arg_bdy_condn,
 const vector_defn_node* curr_param, c_symbol_table& fn_bindings,
 deque<c_symbol_info*>& input_exprs, deque<const domain_node*>& arg_domains)
{
  PrintC::init_arg_info
    (curr_argument,arg_bdy_condn,curr_param,fn_bindings,
     input_exprs,arg_domains);

  cuda_symbol_info* arg_symbol =
    static_cast<cuda_symbol_info*>(input_exprs.back());
  assert(arg_symbol && "Arg symbol used is not a cuda symbol");

  if( isTextureAmenable(curr_argument,arg_bdy_condn,curr_param) ){
    prepare_texture_reference(arg_symbol,arg_bdy_condn);
    assert(arg_symbol->texture_info && "Failed to allocate texture reference");
  }
}

///-----------------------------------------------------------------------------
void PrintCUDA::computeOffsets
(const fnid_expr_node* fn, deque<offset_hull>& offsets,
 const domainfn_node* output_scale_domain, bool considerBdyInfo)
{
  offsets.clear();
  const fn_defn_node* fn_defn = fn->get_defn();
  assert(dynamic_cast<const stencilfn_defn_node*>(fn_defn) != NULL
         && "Computing offsets for non-stencil function args");
  int ndims = fn->get_dim();

  if( output_scale_domain ){
    assert(output_scale_domain->get_dim() == ndims &&
           "Output var scale info mismatch with loop nest dim");
    //Adjust loop bounds to access valid points of the output domain, when
    //scaling
    for( int i = 0 ; i < ndims ; i++ ){
      int lower_offset =
        MIN(0,FLOOR(output_scale_domain->scale_fn[i].offset,
                    output_scale_domain->scale_fn[i].scale));
      int upper_offset =
        MAX(0,FLOOR(output_scale_domain->scale_fn[i].offset,
                    output_scale_domain->scale_fn[i].scale));
      offsets.push_back(offset_hull(lower_offset,upper_offset));
    }
  }
  else{
    ///Start with 0 offsets for all dimensions
    for( int i = 0 ; i < ndims ; i++)
      offsets.push_back(offset_hull(0,0));
  }

  ///For all inputs, make sure we access a valid point
  const deque<vector_defn_node*>& fn_params = fn_defn->get_args();
  deque<arg_info>::const_iterator arg_jt  = fn->get_args().begin();
  for( deque<vector_defn_node*>::const_iterator it = fn_params.begin() ;
       it != fn_params.end() ; it++,arg_jt++ ) {
    /// Dont need to do anything when the argument is a scalar, or has a direct
    /// access
    if( (*it)->get_dim() != 0 && !(*it)->get_direct_access() ){

      /// Ignore those arguments that have a boundary condition, used while
      /// computing the exteriors of the computation domain on stencils
      if( considerBdyInfo && arg_jt->bdy_condn->type != B_NONE )
        continue;
    }
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::deleteArgSymbols(deque<c_symbol_info*>& arg_symbols)
{
 /// Delete the input symbols passed as well, they arent needed now
  for( deque<c_symbol_info*>::iterator it= arg_symbols.begin();
       it != arg_symbols.end() ; it++){
    assert
      (dynamic_cast<cuda_symbol_info*>(*it)
       && "Deleteing a non cuda symbol in cuda codegenerator");
    cuda_symbol_info* curr_symbol = static_cast<cuda_symbol_info*>(*it);
    if( curr_symbol->texture_info ){
      kernel_launch.indent();
      kernel_launch.buffer << "cudaUnbindTexture(&"
                           << curr_symbol->texture_info->name <<");";
      kernel_launch.newline();
      kernel_launch.indent();
      kernel_launch.buffer
        << "Check_CUDA_Error(\"Texture Unbind Error!!"
        <<" Error while unbinding Texture "
        << curr_symbol->texture_info->name << ", previously bound to "
        << curr_symbol->var->symbol_name << "\\n\");";
      kernel_launch.newline();
      curr_symbol->texture_info->curr_bound_var = NULL;
      curr_symbol->texture_info = NULL;
      curr_symbol->is_texture_normalized = false;
    }
    if( curr_symbol->var->expr_domain == NULL ){
      /// THis is a scalar argument
      delete curr_symbol->var;
    }
    delete curr_symbol;
  }
}

///-----------------------------------------------------------------------------
/// Function to generate the body of the vector function
void PrintCUDA::print_vectorfn_body
(const fnid_expr_node* curr_fn, deque<c_symbol_info*>& input_exprs,
 deque<const domain_node*>& arg_domains, c_symbol_info* curr_output_symbol)
{
  fn_defn_node* fn_defn = curr_fn->get_defn();
  assert
    (dynamic_cast<vectorfn_defn_node*>(fn_defn) &&
     "Calling generator of vector fn body for non-vector function");
  vectorfn_defn_node* curr_vector_fn= static_cast<vectorfn_defn_node*>(fn_defn);
  const deque<vector_defn_node*> fn_params = curr_vector_fn->get_args();
  assert
    (input_exprs.size() == fn_params.size()
     && "Number of input symbols doesnt match number of vector fn params");
  /// Add the input exprs to the symbol table of the vector fn body
  cuda_symbol_table new_fn_bindings;
  deque<vector_defn_node*>::const_iterator fn_params_iter = fn_params.begin();
  for( deque<c_symbol_info*>::iterator symbol_iter = input_exprs.begin(),
         symbol_end = input_exprs.end() ; symbol_iter != symbol_end ;
       symbol_iter++, fn_params_iter++){
    assert
      (dynamic_cast<cuda_symbol_info*>(*symbol_iter) &&
       "Found a non-cuda symbol while looking for a CUDA symbol");
    new_fn_bindings.symbol_table.insert
      (make_pair(*fn_params_iter,static_cast<cuda_symbol_info*>(*symbol_iter)));
  }

  ///Need to recompute the domain of expressions within the vector
  ///function based ont he domain of the arguments
  curr_vector_fn->compute_domain(arg_domains);
  assert
    (dynamic_cast<cuda_var_info*>(curr_output_symbol->var) &&
     "Found non-cuda variable while looking for CUDA variable");
  new_fn_bindings.AddSymbol
    (static_cast<const vectorfn_defn_node*>(fn_defn)->get_return_expr(),
     static_cast<cuda_var_info*>(curr_output_symbol->var),
     curr_output_symbol->scale_domain, curr_output_symbol->offset_domain,
     curr_output_symbol->field_name);
  print_vectorfn_body_helper(curr_vector_fn,new_fn_bindings,true);

  /// Remove the input_exprs from the symbol table, they are deallocated later
  fn_params_iter = fn_params.begin();
  for(deque<vector_defn_node*>::const_iterator fn_params_end= fn_params.end();
      fn_params_iter != fn_params_end ; fn_params_iter++ ){
    map<const vector_expr_node*,c_symbol_info*>::iterator var = new_fn_bindings.symbol_table.find(*fn_params_iter);
    new_fn_bindings.symbol_table.erase(*fn_params_iter);
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_program_body
(const program_node* curr_program, c_symbol_table& fn_bindings,
 string& output_var)
{
  const vectorfn_defn_node* program_fn = curr_program->get_body();
  const deque<vector_defn_node*> curr_args = program_fn->get_args();
  ///Allocate memory for the input on the GPU and copy from CPU to GPU
  for( deque<vector_defn_node*>::const_iterator it = curr_args.begin() ;
       it != curr_args.end() ; it++ ){
    //print_vector_defn(*it,fn_bindings);
    if( (*it)->get_dim() != 0 ){
      c_var_info* arg_var =
        printMalloc
        ((*it)->get_name(),(*it)->get_data_type(),(*it)->get_expr_domain());
      domain_node* sub_domain = NULL;
      if( (*it)->get_sub_domain())
        sub_domain = new domain_node((*it)->get_sub_domain());
      fn_bindings.AddSymbol(*it,arg_var,NULL,sub_domain,"");

      host_allocate->indent();
      host_allocate->buffer << "cudaPointerAttributes ptrAttrib_h_"
                           << arg_var->symbol_name << ";";
      host_allocate->newline();
      host_allocate->indent();
      host_allocate->buffer << "cudaMemcpyKind memcpy_kind_h_"
                           << arg_var->symbol_name
                           << " = cudaMemcpyHostToDevice;";
      host_allocate->newline();
      host_allocate->indent();
      host_allocate->buffer << "if (cudaPointerGetAttributes(&ptrAttrib_h_"
                           << arg_var->symbol_name << ", h_"
                           << arg_var->symbol_name << ") == cudaSuccess)";
      host_allocate->newline();
      host_allocate->indent();
      host_allocate->buffer << "  if (ptrAttrib_h_" << arg_var->symbol_name
                           << ".memoryType == cudaMemoryTypeDevice)";
      host_allocate->newline();
      host_allocate->indent();
      host_allocate->buffer << "    memcpy_kind_h_" << arg_var->symbol_name
                           << " = cudaMemcpyDeviceToDevice;";
      host_allocate->newline();
      host_allocate->indent();
      host_allocate->buffer << "cudaGetLastError();";
      host_allocate->newline();

      cuda_var_info* cuda_arg_var = static_cast<cuda_var_info*>(arg_var);
      host_allocate->indent();
      if( cuda_arg_var->pitch.compare("") != 0 ){
        host_allocate->buffer << "cudaMemcpy2D(" << cuda_arg_var->symbol_name
                             << "," << cuda_arg_var->pitch << "*sizeof("
                             << get_string(cuda_arg_var->elem_type) << "),h_"
                             << arg_var->symbol_name << ",";
        ///Pitch of source
        host_allocate->buffer << "sizeof(" <<
          get_string(cuda_arg_var->elem_type) << ")" ;
        deque<range_coeffs>::const_reverse_iterator it =
          cuda_arg_var->expr_domain->get_domain().rbegin();
        host_allocate->buffer << "*(";
        PrintCParametricExpr(it->ub,host_allocate->buffer);
        host_allocate->buffer << "-";
        PrintCParametricExpr(it->lb,host_allocate->buffer);
        host_allocate->buffer << "+1),";
        ///Wdith of source
        host_allocate->buffer << "sizeof("
                             << get_string(cuda_arg_var->elem_type) << ")" ;
        host_allocate->buffer << "*(";
        PrintCParametricExpr(it->ub,host_allocate->buffer);
        host_allocate->buffer << "-";
        PrintCParametricExpr(it->lb,host_allocate->buffer);
        host_allocate->buffer << "+1),";
        ///Height of source
        it++;
        host_allocate->buffer << "(";
        PrintCParametricExpr(it->ub,host_allocate->buffer);
        host_allocate->buffer << "-";
        PrintCParametricExpr(it->lb,host_allocate->buffer);
        host_allocate->buffer << "+1)";
        for( it++;it != cuda_arg_var->expr_domain->get_domain().rend() ; it++ ){
          host_allocate->buffer << "*(";
          PrintCParametricExpr(it->ub,host_allocate->buffer);
          host_allocate->buffer << "-";
          PrintCParametricExpr(it->lb,host_allocate->buffer);
          host_allocate->buffer << "+1)";
        }
        host_allocate->buffer << ", memcpy_kind_h_" << arg_var->symbol_name
                             << ");" ;
        host_allocate->newline();
      }
      else{
        host_allocate->buffer << "if( memcpy_kind_h_"<< arg_var->symbol_name
                             <<" != cudaMemcpyDeviceToDevice ){";
        host_allocate->newline();
        host_allocate->increaseIndent();
        host_allocate->indent();
        host_allocate->buffer << "cudaMemcpy(" << arg_var->symbol_name << ",h_"
                             << arg_var->symbol_name << ",sizeof("
                             << get_string(arg_var->elem_type) << ")*" ;
        PrintCDomainSize(arg_var->expr_domain,host_allocate->buffer);
        host_allocate->buffer << ", memcpy_kind_h_" << arg_var->symbol_name
                             << ");" ;
        host_allocate->newline();
        host_allocate->decreaseIndent();
        host_allocate->indent();
        host_allocate->buffer << "}";
        host_allocate->newline();
      }
    }
    else{
      cuda_var_info* arg_var =
        new cuda_var_info((*it)->get_name(),NULL,(*it)->get_data_type());
      fn_bindings.AddSymbol(*it,arg_var,NULL,NULL,"");
    }
  }

  string result_var = get_new_var();
  const vector_expr_node* return_expr = program_fn->get_return_expr();
  domain_node* return_expr_domain =
    new domain_node(return_expr->get_expr_domain());
  if( return_expr->get_sub_domain() )
    return_expr_domain->compute_intersection(return_expr->get_sub_domain());
  c_var_info* return_var =
    printMalloc(result_var,return_expr->get_data_type(),return_expr_domain);
  fn_bindings.AddSymbol(return_expr,return_var,NULL,NULL,"");

  print_vectorfn_body_helper(curr_program->get_body(),fn_bindings,false);

  kernel_launch.indent();
  kernel_launch.buffer << "cudaPointerAttributes ptrAttrib_" << output_var
                       << ";";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaMemcpyKind memcpy_kind_" << output_var
                       << " = cudaMemcpyDeviceToHost;";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "if (cudaPointerGetAttributes(&ptrAttrib_"
                       << output_var<< ", "<< output_var << ") == cudaSuccess)";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "  if (ptrAttrib_" << output_var
                       << ".memoryType == cudaMemoryTypeDevice)";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "    memcpy_kind_" << output_var
                       << " = cudaMemcpyDeviceToDevice;";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaGetLastError();";
  kernel_launch.newline();

  kernel_launch.indent();
  cuda_var_info* cuda_return_var = static_cast<cuda_var_info*>(return_var);
  if( cuda_return_var->pitch.compare("") != 0 ){
    kernel_launch.buffer << "cudaMemcpy2D(" << output_var << ",sizeof("
                         << get_string(return_expr->get_data_type()) << ")*" ;
    deque<range_coeffs>::const_reverse_iterator it =
      return_expr_domain->get_domain().rbegin();
    kernel_launch.buffer << "(";
    PrintCParametricExpr(it->ub,kernel_launch.buffer);
    kernel_launch.buffer << "-";
    PrintCParametricExpr(it->lb,kernel_launch.buffer);
    kernel_launch.buffer << "+1),";
    kernel_launch.buffer << result_var << ",sizeof("
                         << get_string(return_expr->get_data_type()) << ")*"
                         << cuda_return_var->pitch << ",sizeof("
                         << get_string(return_expr->get_data_type()) << ")*" ;
    kernel_launch.buffer << "(";
    PrintCParametricExpr(it->ub,kernel_launch.buffer);
    kernel_launch.buffer << "-";
    PrintCParametricExpr(it->lb,kernel_launch.buffer);
    kernel_launch.buffer << "+1),";
    it++;
    kernel_launch.buffer << "(";
    PrintCParametricExpr(it->ub,kernel_launch.buffer);
    kernel_launch.buffer << "-";
    PrintCParametricExpr(it->lb,kernel_launch.buffer);
    kernel_launch.buffer << "+1)";
    for( it++ ; it != return_expr_domain->get_domain().rend() ; it++ ){
      kernel_launch.buffer << "*(";
      PrintCParametricExpr(it->ub,kernel_launch.buffer);
      kernel_launch.buffer << "-";
      PrintCParametricExpr(it->lb,kernel_launch.buffer);
      kernel_launch.buffer << "+1)";
    }
  }
  else{
    kernel_launch.buffer << "cudaMemcpy(" << output_var << "," <<
      result_var << ", sizeof(" <<
      get_string(program_fn->get_return_expr()->get_data_type()) << ")*" ;
    PrintCDomainSize(return_expr_domain,kernel_launch.buffer);
  }
  kernel_launch.buffer << ", memcpy_kind_" << output_var << ");";
  kernel_launch.newline();

  delete return_expr_domain;

  fn_bindings.RemoveSymbol(return_expr);


}


///-----------------------------------------------------------------------------
void PrintCUDA::init_parameters()
{
  for( deque<pair<string,parameter_defn*> >::const_iterator it =
         global_params->begin() ; it != global_params->end() ; it++ ){
    output_buffer->indent();
    output_buffer->buffer << "int " << it->first << ";";
    output_buffer->newline();
  }
}


///-----------------------------------------------------------------------------
void PrintCUDA::GenerateCode
(const program_node* curr_program, const program_options& command_opts)
{
  if( command_opts.use_texture ){
    printf("Textures enabled\n");
    enable_texture = true;
    init_channels();
  }
  init_zero = command_opts.init_zero;

  if (command_opts.generate_unroll_code) {
    generate_unroll_code = true;
    unroll_factors.insert
      (unroll_factors.begin(), command_opts.unroll_factors.begin(),
       command_opts.unroll_factors.end());
  }

  PrintCStructDefinition(*output_buffer, false);
  //init_parameters();

  kernel_name_append = command_opts.kernel_name;
  ///Generate the function header
  const vectorfn_defn_node* program_fn = curr_program->get_body();
  const deque<vector_defn_node*> curr_args = program_fn->get_args();

  function_header = new stringBuffer;

  if( command_opts.kernel_name.compare("") != 0 ){
    function_header->buffer << "extern \"C\" void " << command_opts.kernel_name
                         <<"(" ;
  }
  else{
    function_header->buffer << "extern \"C\" void __forma_kernel__(" ;
  }

  for( deque<vector_defn_node*>::const_iterator it = curr_args.begin() ;
       it != curr_args.end() ; it++ ){
    if( (*it)->get_dim() != 0 ) {
      function_header->buffer << get_string((*it)->get_data_type()) <<
        " * h_" << (*it)->get_name() << ", ";
    }
    else{
      function_header->buffer << get_string((*it)->get_data_type()) << " " <<
        (*it)->get_name() << ", ";
    }
  }
  ///Add parameters as arguments
  for( deque<pair<string,parameter_defn*> >::const_iterator it =
         global_params->begin() ; it != global_params->end() ; it++ ){
    function_header->buffer << "int " << it->first << ", ";
  }
  string output_var = get_new_var();
  function_header->buffer <<
    get_string(program_fn->get_return_expr()->get_data_type()) << " * " <<
    output_var << "){";
  function_header->newline();

  host_allocate->increaseIndent();
  kernel_launch.increaseIndent();
  deallocate_buffer->increaseIndent();

  ///get device specific information
  if( command_opts.use_single_malloc ){
    use_single_malloc = true;
    globalMemVar = "__FORMA_GLOBAL_MEMORY_VAR__";
    globalMemOffset = "__FORMA_GLOBAL_MEMORY_VAR_OFFSET__";
    host_allocate_size = new stringBuffer;
    host_allocate_size->increaseIndent();

    host_allocate_size->indent();
    host_allocate_size->buffer << "int " << globalMemOffset << " = 0;";
    host_allocate_size->newline();

    host_allocate->indent();
    host_allocate->buffer << globalMemOffset << " = 0;";
    host_allocate->newline();
  }

  cuda_symbol_table fn_bindings;

  kernel_launch.buffer << "#ifdef _TIMER_" << endl;
  kernel_launch.indent();

  kernel_launch.buffer << "cudaEvent_t _forma_timer_start_,_forma_timer_stop_;";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventCreate(&_forma_timer_start_);";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventCreate(&_forma_timer_stop_);";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventRecord(_forma_timer_start_,0);";
  kernel_launch.newline();

  kernel_launch.buffer << "#endif" << endl;

  print_program_body(curr_program,fn_bindings,output_var);

  kernel_launch.buffer << "#ifdef _TIMER_" << endl;
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventRecord(_forma_timer_stop_,0);" ;
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventSynchronize(_forma_timer_stop_);" ;
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "float elapsedTime;" ;
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventElapsedTime("
                       << "&elapsedTime,_forma_timer_start_,_forma_timer_stop_"
                       <<");" ;
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "printf(\"[FORMA] Computation Time(ms) : %lf\\n\""
                       <<",elapsedTime);";
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventDestroy(_forma_timer_start_);" ;
  kernel_launch.newline();
  kernel_launch.indent();
  kernel_launch.buffer << "cudaEventDestroy(_forma_timer_stop_);" ;
  kernel_launch.newline();
  kernel_launch.buffer << "#endif" << endl;

  if( use_single_malloc ){
    host_allocate_size->indent();
    host_allocate_size->buffer << "char* " << globalMemVar << ";";
    host_allocate_size->newline();

    host_allocate_size->indent();
    host_allocate_size->buffer << "if( " << globalMemOffset << " > 0 ){";
    host_allocate_size->newline();
    host_allocate_size->increaseIndent();

    host_allocate_size->indent();
    host_allocate_size->buffer << "cudaMalloc(&" << globalMemVar << "," <<
      globalMemOffset << ");";
    host_allocate_size->newline();

    host_allocate_size->indent();
    host_allocate_size->buffer << "Check_CUDA_Error(\"Allocation Error!! : " <<
      globalMemVar << "\\n\");" ;
    host_allocate_size->newline();

    if( init_zero ){
      host_allocate_size->indent();
      host_allocate_size->buffer << "cudaMemset(" << globalMemVar << ",0," <<
        globalMemOffset << ");";
      host_allocate_size->newline();
    }

    host_allocate_size->decreaseIndent();
    host_allocate_size->indent();
    host_allocate_size->buffer << "}";
    host_allocate_size->newline();

    deallocate_buffer->indent();
    deallocate_buffer->buffer << "if( " << globalMemOffset << " > 0 )";
    deallocate_buffer->newline();
    deallocate_buffer->increaseIndent();

    deallocate_buffer->indent();
    deallocate_buffer->buffer << "cudaFree(" << globalMemVar << ");";
    deallocate_buffer->newline();
    deallocate_buffer->decreaseIndent();
  }

  if( use_single_malloc )
    host_allocate_size->decreaseIndent();
  host_allocate->decreaseIndent();
  kernel_launch.decreaseIndent();
  deallocate_buffer->decreaseIndent();
  deallocate_buffer->indent();
  deallocate_buffer->buffer << "}" ;
  deallocate_buffer->newline();

  print_string(curr_program, command_opts,output_var);
}

///-----------------------------------------------------------------------------
void PrintCUDA::print_string
(const program_node* curr_program, const program_options& command_opts,
 string& output_var)
{
  FILE* CodeGenFile = fopen(command_opts.cuda_output_file.c_str(),"w");

  print_header_info(CodeGenFile);
  fprintf
    (CodeGenFile,"/*Texture references */\n%s",file_scope.buffer.str().c_str());
  if( sharedMemVar.size() > 0 )
    fprintf(CodeGenFile,"/*Shared Memory Variable */\n"
            "extern __shared__ char %s[];\n",sharedMemVar.c_str());
  fprintf
    (CodeGenFile,"/* Device code Begin */\n%s/*Device code End */\n",
     output_buffer->buffer.str().c_str());
  fprintf
    (CodeGenFile,"/* Host Code Begin */\n%s\n",
     function_header->buffer.str().c_str());
  if( use_single_malloc ){
    fprintf
      (CodeGenFile,
       "/* Host allocation size Begin */\n%s/*Host allocation size End */\n",
       host_allocate_size->buffer.str().c_str());
  }
  fprintf
    (CodeGenFile,"/* Host allocation Begin */\n%s/*Host Allocation End */\n",
     host_allocate->buffer.str().c_str());
  fprintf
    (CodeGenFile,"/* Kernel Launch Begin */\n%s/*Kernel Launch End */\n",
     kernel_launch.buffer.str().c_str());
  fprintf
    (CodeGenFile,"/* Host Free Begin */\n%s/*Host Free End*/\n",
     deallocate_buffer->buffer.str().c_str());

  fflush(CodeGenFile);
  fclose(CodeGenFile);
}


///-----------------------------------------------------------------------------
void PrintCUDA::print_header_info(FILE* outfile)
{
  fprintf(outfile,"#include \"cuda.h\"\n");
  fprintf(outfile,"#ifdef _TIMER_\n#include \"cuda_profiler_api.h\"\n#endif\n");
  fprintf(outfile, "#include \"stdio.h\"\n\n");
  stringBuffer formaDefines;
  PrintCParametricDefines(formaDefines);
  fprintf(outfile,"%s\n",formaDefines.buffer.str().c_str());
  fprintf(outfile,"#ifndef FORMA_MAX_BLOCKDIM_0\n"
          "#define FORMA_MAX_BLOCKDIM_0 1024\n"
          "#endif\n");
  fprintf(outfile,"#ifndef FORMA_MAX_BLOCKDIM_1\n"
          "#define FORMA_MAX_BLOCKDIM_1 1024\n"
          "#endif\n");
  fprintf(outfile,"#ifndef FORMA_MAX_BLOCKDIM_2\n"
          "#define FORMA_MAX_BLOCKDIM_2 1024\n"
          "#endif\n");
  fprintf
    (outfile,"template<typename T>"
     "\n__global__ void  __kernel_init__(T* input, T value)\n"
     "{\n  int loc = (int)(blockIdx.x)*(int)(blockDim.x)+(int)(threadIdx.x);\n"
     "  input[loc] = value;\n}\n\n\ntemplate<typename T>\n"
     "void initialize_array(T* d_input, int size, T value)\n{"
     "\n  dim3 init_grid(FORMA_CEIL(size,FORMA_MAX_BLOCKDIM_0));\n"
     "  dim3 init_block(FORMA_MAX_BLOCKDIM_0);\n"
     "  __kernel_init__<<<init_grid,init_block>>>(d_input,value);\n}\n\n\n"
     "void Check_CUDA_Error(const char* message);\n");
}
