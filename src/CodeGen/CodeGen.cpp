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
#include "CodeGen/CodeGen.hpp"
#include "AST/parser.hpp"

using namespace std;


///-----------------------------------------------------------------------------
void CodeGen::PrintCHeader
(const program_node* program, const program_options& opts)
{
  const vectorfn_defn_node* program_fn = program->get_body();
  const deque<vector_defn_node*> curr_args = program_fn->get_args();
  stringBuffer header_buffer;
  PrintCStructDefinition(header_buffer);
  header_buffer.buffer << "#ifdef __cplusplus\n";
  header_buffer.buffer << "extern \"C\" {\n";
  header_buffer.buffer << "#endif\n";

  /// Function signature : void <kernel name>( <inputs arguments>, <parameters>,
  /// <output buffer>);
  header_buffer.buffer << "void ";
  header_buffer.buffer << opts.kernel_name << "(";
  /// <input arguments>
  for( deque<vector_defn_node*>::const_iterator I = curr_args.begin() ,
         E = curr_args.end();
       I != E ; I++ ){
    header_buffer.buffer << get_string((*I)->get_data_type());
    header_buffer.buffer << ( (*I)->get_dim() == 0 ? " " : "* ") ;
    header_buffer.buffer << (*I)->get_name() << ", ";
  }

  /// parameters
  for( deque<pair<string,parameter_defn*> >::const_iterator
         I = global_params->begin(), E = global_params->end() ; I != E ; I++ )
    header_buffer.buffer << "int " << I->first << ", ";

  /// buffer for the output
  header_buffer.buffer <<
    get_string(program_fn->get_return_expr()->get_data_type());
  header_buffer.buffer << "* output);\n";

  /// Size of the output
  header_buffer.buffer << "///Size of the output : ";
  PrintCDomainSize(program_fn->get_expr_domain(),header_buffer.buffer);
  header_buffer.buffer <<"\n";

  header_buffer.buffer << "#ifdef __cplusplus\n";
  header_buffer.buffer<< "}\n";
  header_buffer.buffer << "#endif\n";

  FILE* outputFile = fopen(opts.header_file_name.c_str(),"a");
  fprintf(outputFile,"%s",header_buffer.buffer.str().c_str());
  fflush(outputFile);
  fclose(outputFile);
}


///-----------------------------------------------------------------------------
void CodeGen::PrintCStructDefinition
(stringBuffer& curr_buffer, bool define_cuda_structs)
{
  if( defined_types ){
    for( deque<pair<string,defined_data_type*> >::const_iterator
           it = defined_types->begin() ; it != defined_types->end() ; it++ ){
      if (!define_cuda_structs) {
        if (data_types::is_supported_cuda_type(it->first))
          continue;
      }
      curr_buffer.buffer << "struct " << it->first << "{" << endl;
      curr_buffer.increaseIndent();
      for( deque<defined_fields>::const_iterator jt =it->second->fields.begin();
           jt != it->second->fields.end() ; jt++){
        curr_buffer.indent();
        data_types field_data_type;
        field_data_type.assign(jt->field_type);
        curr_buffer.buffer << get_string(field_data_type) << " " <<
          jt->field_name << ";" ;
        curr_buffer.newline();
      }
      curr_buffer.decreaseIndent();
      curr_buffer.indent();
      curr_buffer.buffer << "};" ;
      curr_buffer.newline();
    }
  }
}


void CodeGen::PrintCDomainSize
(const domain_node* curr_domain, stringstream& curr_stream)
{
  const deque<range_coeffs> range_list = curr_domain->get_domain();
  if( range_list.size() != 0 ){
    deque<range_coeffs>::const_iterator it = range_list.begin();
    curr_stream << "(" ;
    parametric_exp* curr_size = parametric_exp::copy(it->ub);
    curr_size = curr_size->subtract(it->lb);
    curr_size = curr_size->add(1);
    PrintCParametricExpr(curr_size,curr_stream);
    delete curr_size;
    for( it++ ; it != range_list.end() ; it++ ){
      curr_stream << "*" ;
      curr_size = parametric_exp::copy(it->ub);
      curr_size = curr_size->subtract(it->lb);
      curr_size = curr_size->add(1);
      PrintCParametricExpr(curr_size,curr_stream);
      delete curr_size;
    }
    curr_stream << ")";
  }
  else{
    curr_stream << "(1)" ;
  }
}


///-----------------------------------------------------------------------------
void CodeGen::PrintCParametricExpr
(const parametric_exp* curr_expr, stringstream& curr_stream)
{
  switch(curr_expr->type){
  case P_INT:{
    int value = static_cast<const int_expr*>(curr_expr)->value;
    if( value < 0 )
      curr_stream << "(" << value << ")";
    else
      curr_stream << value;
  }
    break;
  case P_PARAM:
    curr_stream << static_cast< const param_expr*>(curr_expr)->param->param_id ;
    break;
  case P_ADD: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "(";
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "+" ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_SUBTRACT: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "(";
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "-" ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_MULTIPLY: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "(";
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "*" ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_DIVIDE: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "(";
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "/" ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_MAX: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "FORMA_MAX(" ;
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "," ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_MIN: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "FORMA_MIN(";
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "," ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  case P_CEIL: {
    const binary_expr* curr_binary =static_cast< const binary_expr*>(curr_expr);
    curr_stream << "FORMA_CEIL(";
    PrintCParametricExpr(curr_binary->lhs_expr,curr_stream);
    curr_stream << "," ;
    PrintCParametricExpr(curr_binary->rhs_expr,curr_stream);
    curr_stream << ")";
    break;
  }
  default:
    assert(0);
  }
}


///-----------------------------------------------------------------------------
void CodeGen::PrintCParametricDefines(stringBuffer& curr_buffer)
{
  curr_buffer.buffer << "#define FORMA_MAX(a,b) ( (a) > (b) ? (a) : (b) )";
  curr_buffer.newline();
  curr_buffer.buffer << "#define max(a,b) FORMA_MAX(a,b)";
  curr_buffer.newline();
  curr_buffer.buffer << "#define FORMA_MIN(a,b) ( (a) < (b) ? (a) : (b) )";
  curr_buffer.newline();
  curr_buffer.buffer << "#define min(a,b) FORMA_MIN(a,b)";
  curr_buffer.newline();
  curr_buffer.buffer <<
    "#define FORMA_CEIL(a,b) ( (a) % (b) == 0 ? (a) / (b) : ((a) / (b)) + 1 )";
  curr_buffer.newline();
}


///-----------------------------------------------------------------------------
void CodeGen::computeOffsets
(const fnid_expr_node *fn, deque<offset_hull> &offsets,
 const domainfn_node* outputIdxInfo, bool considerBdyInfo) const {
  offsets.clear();

  const fn_defn_node *fnDefn = fn->get_defn();
  const std::deque<vector_defn_node *> fnParams = fnDefn->get_args();
  unsigned numDims = fn->get_dim();

  if( outputIdxInfo ){
    // If the output index is to be scaled up, loop bounds might need
    // adjusting
    const domainfn_node* outputScaleInfo =
      static_cast<const domainfn_node*>(outputIdxInfo);
    for( unsigned i = 0 ; i != numDims ; i++ ){
      int lowerOffset =
        std::min(0,FLOOR(outputScaleInfo->scale_fn[i].offset,
                         outputScaleInfo->scale_fn[i].scale));
      int upperOffset =
        std::max(0, FLOOR(outputScaleInfo->scale_fn[i].offset,
                          outputScaleInfo->scale_fn[i].scale));
      offsets.push_back(offset_hull(lowerOffset,upperOffset));
    }
  }
  else{
    // Start with no non-zero offsets
    for (unsigned i = 0; i != numDims; ++i) {
      offsets.push_back(offset_hull(0, 0));
    }
  }

  // Run over each function argument
  auto fnParamsIt = fnParams.begin();
  auto fnParamEnd = fnParams.end();
  auto fnArgsIt = fn->get_args().begin();
  for (; fnParamsIt != fnParamEnd; ++fnParamsIt, fnArgsIt++) {
    if( (*fnParamsIt)->get_dim() != 0 ){

      /// Ignore those arguments that have a bdy_condn
      if( considerBdyInfo && fnArgsIt->bdy_condn->type != B_NONE )
        continue;

      // Run over the stencil access information
      auto stencilAccessInfo = (*fnParamsIt)->get_access_info();
      assert(stencilAccessInfo.size() == numDims && "Mismatched stencil info");
      for (unsigned dim = 0; dim != numDims; ++dim) {
        auto &hull = stencilAccessInfo[dim];
        ///Largest value of the iterator that indexes a valid point
        offsets[dim].max_positive =
          std::max
          (offsets[dim].max_positive,CEIL(hull.max_positive,hull.scale));
        offsets[dim].max_negetive =
          std::min
          (offsets[dim].max_negetive,FLOOR(hull.max_negetive,hull.scale));
      }
    }
  }
}

///-----------------------------------------------------------------------------
/// Apply an offset to a loop domain, shrinks the loop domain
void CodeGen::ApplyOffset
(domain_node* loop_domain, const deque<offset_hull>& boundary_offsets) const
{
  deque<offset_hull>::const_iterator offset_begin = boundary_offsets.begin();
  for(deque<range_coeffs>::iterator dim_begin = loop_domain->range_list.begin(),
        dim_end = loop_domain->range_list.end(); dim_begin != dim_end ;
      dim_begin++, offset_begin++){
    dim_begin->lb = dim_begin->lb->subtract(offset_begin->max_negetive);
    dim_begin->ub = dim_begin->ub->subtract(offset_begin->max_positive);
  }
}
