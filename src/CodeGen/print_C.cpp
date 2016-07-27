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
#include "CodeGen/print_C.hpp"

using namespace std;

///-----------------------------------------------------------------------------
/// Constructor for c_iterator
c_iterator::c_iterator
(string name, const parametric_exp* lb, const parametric_exp* ub):
  name(name), lb(lb), ub(ub) {
  if(parametric_exp::is_equal(lb,ub) )
    unit_trip_count = true;
  else
    unit_trip_count = false;
}

///-----------------------------------------------------------------------------
/// Constructor that initializes the C codegenerator state
PrintC::PrintC
(bool gen_omp, bool gen_icc):
  generate_omp_pragmas(gen_omp),
  generate_icc_pragmas(gen_icc)
{
  init_zero = false;

  generate_affine = false;
  supported_affine_dim = 2;
  ntemp_variables = 0;
  niter_variables = 0;

  /// Initialize the buffers
  output_buffer = new stringBuffer();
  deallocate_buffer = new stringBuffer();
  host_allocate = new stringBuffer();

  use_single_malloc = false;
}

///-----------------------------------------------------------------------------
///Destructor
PrintC::~PrintC()
{
  for( deque<c_var_info*>::iterator it = def_vars.begin(); it != def_vars.end();
       it++)
    delete (*it);

  delete output_buffer;
  delete deallocate_buffer;
  delete host_allocate;
  if( use_single_malloc )
    delete host_allocate_size;
}

///-----------------------------------------------------------------------------
std::string PrintC::get_new_iterator(stringBuffer* curr_buffer)
{
  stringstream var_name_stream;
  var_name_stream << "__iter_" << niter_variables++ << "__" ;
  if( generate_affine ){
    host_allocate->indent();
    host_allocate->buffer << "int " << var_name_stream.str() << ";";
    host_allocate->newline();
  }
  else{
    curr_buffer->indent();
    curr_buffer->buffer << "int " <<  var_name_stream.str() << ";";
    curr_buffer->newline();
  }
  return var_name_stream.str();
}



///-----------------------------------------------------------------------------
std::string
PrintC::get_new_temp_var(const data_types& curr_type, stringBuffer* curr_buffer)
{
  stringstream var_name_stream;
  var_name_stream << "__temp_" << ntemp_variables++ << "__" ;
  if( generate_affine ){
    host_allocate->indent();
    host_allocate->buffer << get_string(curr_type) << " "
                         << var_name_stream.str() << ";";
    host_allocate->newline();
  }
  else{
    curr_buffer->indent();
    curr_buffer->buffer << get_string(curr_type) << " "
                       <<  var_name_stream.str() << ";";
    curr_buffer->newline();
  }
  return var_name_stream.str();
}


///-----------------------------------------------------------------------------
///Print the for loop header
c_iterator* PrintC::printForHeader
(const parametric_exp* lb, const parametric_exp* ub, int unroll_factor,
 bool isInnerMost){
  string new_var = get_new_iterator(output_buffer);
  c_iterator* new_iter =
    new c_iterator
    (new_var, parametric_exp::copy(lb), parametric_exp::copy(ub));

  if( generate_omp_pragmas && !new_iter->is_unit_trip_count() ){
    bool addpragma = true;
    for( deque<c_iterator*>::const_iterator itersIter = iters.begin(),
           itersEnd = iters.end() ; itersIter != itersEnd ; itersIter++){
      if( !((*itersIter)->is_unit_trip_count()) ){
        addpragma = false;
        break;
      }
    }

    if( addpragma ){
      output_buffer->buffer << "#pragma omp for schedule(static) private(" <<
        new_var << ")" ;
      output_buffer->newline();
    }
  }

  if( generate_icc_pragmas && isInnerMost && unroll_factor == 1 ){
    output_buffer->buffer << "#ifdef __ICC";
    output_buffer->newline();
    output_buffer->indent();
    output_buffer->buffer << "#pragma vector always";
    output_buffer->newline();
    output_buffer->indent();
    output_buffer->buffer << "#pragma ivdep";
    output_buffer->newline();
    output_buffer->buffer << "#endif";
    output_buffer->newline();
  }
  output_buffer->indent();
  output_buffer->buffer << "for (" << new_var << " = ";
  PrintCParametricExpr(lb,output_buffer->buffer);
  output_buffer-> buffer << "; " << new_var;
  output_buffer->buffer << " <= ";
  PrintCParametricExpr(ub,output_buffer->buffer);
  output_buffer->buffer << " - " << unroll_factor -1 << "; " << new_var;
  if (unroll_factor != 1)
    output_buffer->buffer << "+= " << unroll_factor;
  else
    output_buffer->buffer << "++";
  output_buffer->buffer << ")";

  output_buffer->buffer << "{";
  output_buffer->newline();
  output_buffer->increaseIndent();

  return new_iter;
}


///-----------------------------------------------------------------------------
/// Print the footer of the for loop
void PrintC::printForFooter()
{
  output_buffer->decreaseIndent();
  output_buffer->indent();
  output_buffer->buffer << "}";
  output_buffer->newline();
  c_iterator* inner_most = iters.back();
  delete inner_most;
  iters.pop_back();
}


///-----------------------------------------------------------------------------
void PrintC::print_pointer(int ndim, stringstream& curr_stream)
{
  for( int i = 0 ; i < ndim ; i++ )
    curr_stream << "*";
}


///-----------------------------------------------------------------------------
c_var_info* PrintC::printMalloc
(string lhs,data_types elem_type, const domain_node* expr_domain)
{
  string elem_type_string = get_string(elem_type);
  c_var_info* new_var = new c_var_info(lhs,expr_domain,elem_type);
  def_vars.push_back(new_var);
  if(expr_domain->get_dim() != 0 ){
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
      host_allocate->buffer << elem_type_string << " * " << lhs << " = (" <<
        elem_type_string << "*)(" << globalMemVar << "+" <<
        globalMemOffset << ");";
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
      if( generate_affine && expr_domain->get_dim() > 1 &&
          expr_domain->get_dim() <= supported_affine_dim  ){
        host_allocate->indent();
        host_allocate->buffer << elem_type_string << " ";
        print_pointer(expr_domain->get_dim(),host_allocate->buffer);
        host_allocate->buffer << " " << lhs << " = " ;

        host_allocate->buffer << "(" << elem_type_string ;
        print_pointer(expr_domain->get_dim(),host_allocate->buffer);
        host_allocate->buffer << ")malloc_" << expr_domain->get_dim() <<
          "d_" <<  get_forma_string(elem_type) << "_array(";
        const deque<range_coeffs>& domain_size = expr_domain->get_domain();
        for( deque<range_coeffs>::const_iterator it = domain_size.begin() ;
             it != domain_size.end() ; it++ ){
          parametric_exp* curr_size = parametric_exp::copy(it->ub);
          curr_size->subtract(it->lb);
          curr_size->add(1);
          PrintCParametricExpr(curr_size,host_allocate->buffer);
          if(it != domain_size.end()-1 )
            host_allocate->buffer << "," ;
          delete curr_size;
        }
        host_allocate->buffer << ");" ;
        host_allocate->newline();
      }
      else{
        host_allocate->indent();
        host_allocate->buffer << elem_type_string << " * " << lhs << " = " ;
        host_allocate->buffer << "(" << elem_type_string << "*)malloc(sizeof("
                              << elem_type_string << ")*"  ;
        PrintCDomainSize(expr_domain,host_allocate->buffer);
        host_allocate->buffer << ");" ;
        host_allocate->newline();
      }
      if( init_zero ){
        host_allocate->indent();
        host_allocate->buffer << "memset(" << lhs << ",0,sizeof("
                              << elem_type_string << ")*" ;
        PrintCDomainSize(expr_domain,host_allocate->buffer);
        host_allocate->buffer << ");";
        host_allocate->newline();
      }
    }
  }
  else{
    host_allocate->indent();
    host_allocate->buffer << elem_type_string << " " << lhs << ";" ;
    host_allocate->newline();
  }

  deallocate_buffer->indent();
  if( expr_domain->get_dim() != 0 ){
    if( !use_single_malloc ){
      if( generate_affine && expr_domain->get_dim() > 1 &&
          expr_domain->get_dim() <= supported_affine_dim )
        deallocate_buffer->buffer << "free_" << expr_domain->get_dim() <<
          "d_" << get_forma_string(elem_type) << "_array("  << lhs << ");";
      else
        deallocate_buffer->buffer << "free(" << lhs << ");";
    }
    deallocate_buffer->newline();
    return new_var;
  }
}

///-----------------------------------------------------------------------------
c_symbol_info* PrintC::FindDefnSymbol
(const vector_expr_node* curr_expr, c_symbol_table& fn_bindings) const
{
  if( fn_bindings.IsPresent(curr_expr) ){
    return fn_bindings.GetSymbolInfo(curr_expr);
  }
  else if( curr_expr->get_type() == VEC_ID ){
    const vec_id_expr_node* curr_vec_id =
      dynamic_cast<const vec_id_expr_node*>(curr_expr);
    const set<vector_expr_node*> curr_defn_set = curr_vec_id->get_defn();
    assert(curr_defn_set.size() == 1);
    const vector_expr_node* curr_defn = *(curr_defn_set.begin());
    return fn_bindings.GetSymbolInfo(curr_defn);
  }
  else{
    return NULL;
  }
}


///-----------------------------------------------------------------------------
/// Compute the loop domain based on the domain of the current variable, and the
/// scale domain
domain_node* PrintC::compute_loop_domain
(const domain_node* curr_domain, const domainfn_node* scale_domain) {
  domain_node* loop_domain = new domain_node(curr_domain);
  if (scale_domain) {
    deque<range_coeffs>& range_list = loop_domain->range_list;
    const deque<scale_coeffs>& scale_fn = scale_domain->scale_fn;
    assert((scale_fn.size() == range_list.size()) &&
           "[ME] Within Code-gen, domain_fn and scale_fn not the same dim\n");
    deque<scale_coeffs>::const_iterator jt = scale_fn.begin();
    for(deque<range_coeffs>::iterator it_begin = range_list.begin(),
          it_end = range_list.end(); it_begin != it_end; it_begin++, jt++) {
      int lb_offset = MIN(0, FLOOR(jt->offset, jt->scale));
      int ub_offset = MAX(0, FLOOR(jt->offset, jt->scale));
      it_begin->lb = it_begin->lb->add(-lb_offset);
      it_begin->ub = it_begin->ub->subtract(ub_offset);
    }
  }
  return loop_domain;
}


///-----------------------------------------------------------------------------
/// Generate loop nests to iterate over a domain, here the output has a scaling
/// factor that is to be used on the LHS
int PrintC::print_domain_loops_header
(const domain_node* curr_domain, deque<c_symbol_info*>& input_exprs,
 const c_var_info* output_var, const domainfn_node* scale_domain,
 const deque<int>& curr_unroll_factors)
{
  domain_node* loop_domain = compute_loop_domain(curr_domain, scale_domain);
  const deque<range_coeffs>& range_list = loop_domain->range_list;

  int loop_dim = 0;
  for( deque<range_coeffs>::const_iterator it_begin = range_list.begin() ,
         it_end = range_list.end() ; it_begin != it_end ;
       it_begin++ , loop_dim++ ){
    c_iterator* new_iter =
      printForHeader
      (it_begin->lb, it_begin->ub, 1, loop_dim == range_list.size()-1);
    iters.push_back(new_iter);
  }
  delete loop_domain;
  return 0;
}


///-----------------------------------------------------------------------------
void PrintC::check_index_value
(string& curr_index, string& is_bdy_var, parametric_exp* curr_lb,
 parametric_exp* curr_ub, const bdy_info* curr_bdy)
{
  stringstream lb_fix, ub_fix;

  switch(curr_bdy->type){
  case B_CONSTANT:{
    // For constant is_bdy_var is actually a boolean var, set it to 1;
    lb_fix << is_bdy_var << " = 1;";
    ub_fix << is_bdy_var << " = 1;";
  }
    break;
  case B_WRAP: {
    lb_fix << curr_index << " += " ;
    PrintCParametricExpr(curr_ub,lb_fix);
    lb_fix << " - " ;
    PrintCParametricExpr(curr_lb,lb_fix);
    lb_fix << " + 1;";

    ub_fix << curr_index << " -= " ;
    PrintCParametricExpr(curr_ub,ub_fix);
    ub_fix << " - " ;
    PrintCParametricExpr(curr_lb,ub_fix);
    ub_fix << " + 1;";
  }
    break;
  case B_EXTEND:
  case B_CLAMPED : {
    lb_fix << curr_index << " = " ;
    PrintCParametricExpr(curr_lb,lb_fix);
    lb_fix << ";";

    ub_fix << curr_index << " = " ;
    PrintCParametricExpr(curr_ub,ub_fix);
    ub_fix << ";";
  }
    break;
  case B_MIRROR : {
    lb_fix << curr_index << " =  2*(" ;
    PrintCParametricExpr(curr_lb,lb_fix);
    lb_fix << ") - " << curr_index << ";";

    ub_fix << curr_index << " =  2*(" ;
    PrintCParametricExpr(curr_ub,ub_fix);
    ub_fix << ") - " << curr_index << ";";
  }
    break;
  case B_NONE:
  default:
    return;
  }


  output_buffer->indent();
  output_buffer->buffer << "if( " << curr_index << " < " ;
  PrintCParametricExpr(curr_lb,output_buffer->buffer);
  output_buffer->buffer << ")";
  output_buffer->newline();
  output_buffer->increaseIndent() ;

  output_buffer->indent();
  output_buffer->buffer << lb_fix.str();
  output_buffer->newline();
  output_buffer->decreaseIndent();

  output_buffer->indent();
  output_buffer->buffer << "else if( " << curr_index << " > " ;
  PrintCParametricExpr(curr_ub,output_buffer->buffer) ;
  output_buffer->buffer << ")" ;
  output_buffer->newline();
  output_buffer->increaseIndent();

  output_buffer->indent();
  output_buffer->buffer << ub_fix.str();
  output_buffer->newline();
  output_buffer->decreaseIndent();

  return;
}

///-----------------------------------------------------------------------------
void PrintC::print_array_access
(const c_var_info* curr_var, const deque<string>& access_exp,
 stringstream& curr_stream)
{
  assert
    ((curr_var->expr_domain->get_dim() == (int)(access_exp.size())) &&
     "[ME] WIthin code-generator,dimension of expr and accesses dont match");
  if( generate_affine &&
      curr_var->expr_domain->get_dim() <= supported_affine_dim ){
    for( deque<string>::const_iterator it = access_exp.begin() ;
         it != access_exp.end(); it++ )
      curr_stream << "[" << (*it) << "]";
  }
  else{
    deque<string>::const_reverse_iterator it = access_exp.rbegin() ;
    curr_stream << "[" << (*it);
    it++;
    for( deque<range_coeffs>::const_reverse_iterator jt =
           curr_var->expr_domain->range_list.rbegin() ;
         it != access_exp.rend() ; it++,jt++ ){
      curr_stream << "+" ;
      parametric_exp* curr_size = parametric_exp::copy(jt->ub);
      curr_size = curr_size->subtract(jt->lb);
      curr_size = curr_size->add(1);
      PrintCParametricExpr(curr_size,curr_stream);
      curr_stream << "*" << "(" <<  (*it) ;
      delete curr_size;
    }
    for( int i = 0 ; i < (int)access_exp.size()-1 ; i++ )
      curr_stream << ")";
    curr_stream << "]";

  }
}


///-----------------------------------------------------------------------------
void PrintC::print_domain_loops_footer
(const domain_node* loop_domain, const deque<int>& curr_unroll_factors)
{
  for( int i = 0 ; i < loop_domain->get_dim() ; i++ )
    printForFooter();
}


///-----------------------------------------------------------------------------
void
PrintC::print_domain_point(const c_var_info* curr_var,stringstream& curr_stream)
{
  curr_stream << curr_var->symbol_name;
  if( curr_var->expr_domain->get_dim() != 0 ){
    deque<string> access_exp;
    for( deque<c_iterator*>::const_iterator it = iters.begin() ;
         it != iters.end() ; it++ ){
      access_exp.push_back((*it)->name);
    }
    print_array_access(curr_var,access_exp,curr_stream);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_domain_point
(const c_var_info* curr_var, stringstream& curr_stream,
 const domain_node* sub_domain)
{
  assert( sub_domain->get_dim() == curr_var->expr_domain->get_dim());
  curr_stream << curr_var->symbol_name;
  if( curr_var->expr_domain->get_dim() != 0 ){
    deque<parametric_exp*> access_domain;
    for( deque<range_coeffs>::const_iterator it= sub_domain->range_list.begin(),
           kt = curr_var->expr_domain->range_list.begin() ;
         it != sub_domain->range_list.end() ; it++ , kt++ ){
      parametric_exp* low = ( it->lb->is_default() ? kt->lb : it->lb );
      access_domain.push_back(low);
    }
    deque<string> reverse_access_exp;
    deque<parametric_exp*>::const_reverse_iterator it = access_domain.rbegin();
    deque<c_iterator*>::const_reverse_iterator jt = iters.rbegin();
    for( ; jt != iters.rend() && it != access_domain.rend() ; it++, jt++ ){
      stringstream new_stream;
      new_stream << (*jt)->name << "+";
      PrintCParametricExpr(*it,new_stream);
      reverse_access_exp.push_back(new_stream.str());
    }
    while( it != access_domain.rend()) {
      stringstream new_stream;
      PrintCParametricExpr(*it,new_stream);
      reverse_access_exp.push_back(new_stream.str());
      it++;
    }
    deque<string> access_exp;
    for( deque<string>::reverse_iterator jt = reverse_access_exp.rbegin() ;
         jt != reverse_access_exp.rend() ; jt++ ){
      access_exp.push_back(*jt);
    }
    print_array_access(curr_var,access_exp,curr_stream);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_stencil_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 stringstream& curr_stream, const bdy_info* curr_bdy_condn){
  if( curr_var->expr_domain && curr_var->expr_domain->get_dim() != 0 ){
    int dim = iters.size() - scale_domain->get_dim();
    assert(dim ==  0 && "Number of iterators not equal to specified offset");
    deque<string>access_exp;
    data_types int_type;
    int_type.assign(T_INT);

    ///For const. bdy condn use a boolean to check if we are in boundary
    string is_bdy_var;
    if(curr_bdy_condn && curr_bdy_condn->type == B_CONSTANT ){
      is_bdy_var = get_new_temp_var(int_type,output_buffer);
      output_buffer->indent();
      output_buffer->buffer << is_bdy_var << " = 0;";
      output_buffer->newline();
    }
    else{
      is_bdy_var = "";
    }

    deque<range_coeffs>::const_iterator jt =
      curr_var->expr_domain->range_list.begin();
    for(deque<scale_coeffs>::const_iterator it = scale_domain->scale_fn.begin();
         it != scale_domain->scale_fn.end() ; it++,dim++,jt++ ){
      stringstream curr_index_stream;
      curr_index_stream << iters[dim]->name;
      if( it->scale != 1 )
        curr_index_stream << "*" << it->scale;
      if( it->offset != 0 )
        curr_index_stream << "+(" << it->offset << ")";

      string curr_index;
      if( curr_bdy_condn ){
        assert(curr_bdy_condn &&
               "Checking Boundary conditions when bdy condn is not specified");
        curr_index  = get_new_temp_var(int_type,output_buffer);
        output_buffer->indent();
        output_buffer->buffer << curr_index << " = "
                             << curr_index_stream.str() << ";";
        output_buffer->newline();
        check_index_value(curr_index,is_bdy_var,jt->lb,jt->ub,curr_bdy_condn);
      }
      else{
        curr_index = curr_index_stream.str();
      }
      access_exp.push_back(curr_index);
    }

    ///For const, bdy condn use check if we are in the boundary and use const
    ///value if true
    if( curr_bdy_condn && curr_bdy_condn->type == B_CONSTANT ){
      curr_stream << "(" << is_bdy_var << " == 1 ? "
                  << curr_bdy_condn->value << " : " << curr_var->symbol_name;
      print_array_access(curr_var,access_exp,curr_stream);
      curr_stream << " )";
    }
    else{
      curr_stream << curr_var->symbol_name;
      print_array_access(curr_var,access_exp,curr_stream);
    }
  }
}

void PrintC::print_stencil_point
 (const c_var_info* curr_var, const domainfn_node* scale_domain,
  const domain_node* sub_domain, stringstream& curr_stream,
  const bdy_info* curr_bdy_condn)
{
  if( curr_var->expr_domain && curr_var->expr_domain->get_dim() != 0 ){
    int dim = iters.size() - scale_domain->get_dim();
    assert(dim ==  0 && "Number of iterators not equal to offset dim");
    assert(scale_domain->get_dim() == sub_domain->get_dim());
    data_types int_type;
    int_type.assign(T_INT);

    ///For const. bdy condn use a boolean to check if we are in boundary
    string is_bdy_var;
    if(curr_bdy_condn && curr_bdy_condn->type == B_CONSTANT ){
      is_bdy_var = get_new_temp_var(int_type,output_buffer);
      output_buffer->indent();
      output_buffer->buffer << is_bdy_var << " = 0;";
      output_buffer->newline();
    }
    else{
      is_bdy_var = "";
    }

    deque<string>access_exp;
    deque<scale_coeffs>::const_iterator ScaleFactorIter =
      scale_domain->scale_fn.begin() ;
    deque<range_coeffs>::const_iterator OffsetIter =
      sub_domain->range_list.begin();
    deque<range_coeffs>::const_iterator BoundsIter;
    BoundsIter = sub_domain->range_list.begin();
    deque<range_coeffs>::const_iterator SizeIter =
      curr_var->expr_domain->range_list.begin();

    for( ; ScaleFactorIter != scale_domain->scale_fn.end() ;
         ScaleFactorIter++, OffsetIter++, SizeIter++,dim++ ){

      stringstream curr_index_stream;
      curr_index_stream << iters[dim]->name;
      if( ScaleFactorIter->scale != 1 )
        curr_index_stream << "*" << ScaleFactorIter->scale;
      if( ScaleFactorIter->offset != 0 )
        curr_index_stream << "+(" << ScaleFactorIter->offset <<")";

      string curr_index;
      if( curr_bdy_condn ){
        parametric_exp* curr_lb =
          ( BoundsIter->lb && BoundsIter->lb->is_default() ?
            SizeIter->lb : BoundsIter->lb);
        parametric_exp* curr_ub =
          ( BoundsIter->ub &&
            BoundsIter->ub->is_default() ? SizeIter->ub : BoundsIter->ub);

        curr_index  = get_new_temp_var(int_type,output_buffer);
        output_buffer->indent();
        output_buffer->buffer << curr_index << " = "
                             << curr_index_stream.str() << ";";
        output_buffer->newline();
        check_index_value(curr_index,is_bdy_var,curr_lb,curr_ub,curr_bdy_condn);
      }
      else{
        curr_index = curr_index_stream.str();
      }

      if( !OffsetIter->lb->is_default() ){
        curr_index.append("+");
        stringstream append_stream;
        PrintCParametricExpr(OffsetIter->lb,append_stream);
        curr_index.append(append_stream.str());
      }
      access_exp.push_back(curr_index);
    }
    ///For const, bdy condn use check if we are in the boundary and use const
    ///value if true
    if( curr_bdy_condn && curr_bdy_condn->type == B_CONSTANT ){
      curr_stream << "(" << is_bdy_var << " == 1 ? " << curr_bdy_condn->value
                  << " : " << curr_var->symbol_name;
      print_array_access(curr_var,access_exp,curr_stream);
      curr_stream << " )";
    }
    else{
      curr_stream << curr_var->symbol_name;
      print_array_access(curr_var,access_exp,curr_stream);
    }
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_interpolate_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 stringstream& curr_stream)
{
  if( curr_var->expr_domain && curr_var->expr_domain->get_dim() != 0 ){
    int dim = iters.size() - scale_domain->get_dim();
    assert(dim ==  0);
    deque<string>access_exp;
    for(deque<scale_coeffs>::const_iterator it = scale_domain->scale_fn.begin();
        it != scale_domain->scale_fn.end() ; it++,dim++ ){
      stringstream new_stream ;
      new_stream  << "(" << iters[dim]->name
                  << ( it->offset < 0 ? "+1" : "" ) << ")" ;
      if( it->scale != 1 )
        new_stream << "*" << it->scale;
      if( it->offset != 0 )
        new_stream << "+(" << it->offset << ")";
      access_exp.push_back(new_stream.str());
    }
    print_array_access(curr_var,access_exp,curr_stream);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_interpolate_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 const domain_node* sub_domain, stringstream& curr_stream)
{
  if( curr_var->expr_domain && curr_var->expr_domain->get_dim() != 0 ){
    int dim = iters.size() - scale_domain->get_dim();
    assert(dim ==  0);
    assert(scale_domain->get_dim() == sub_domain->get_dim());
    deque<string>access_exp;
    deque<scale_coeffs>::const_iterator it = scale_domain->scale_fn.begin() ;
    deque<range_coeffs>::const_iterator jt = sub_domain->range_list.begin();
    for( ; it != scale_domain->scale_fn.end() ; it++,jt++,dim++ ){
      stringstream new_stream;
      if( jt->lb->is_default() ||
          ( jt->lb->type == P_INT &&
            static_cast<int_expr*>(jt->lb)->value == 0 ) ){
        new_stream  << iters[dim]->name;
      }
      else{
        new_stream << "(" << iters[dim]->name << "+" ;
        PrintCParametricExpr(jt->lb,new_stream);
        new_stream << (it->offset < 0 ? "+1" : "" ) <<  ")" ;
      }
      if( it->scale != 1 )
        new_stream << "*" << it->scale;
      if( it->offset != 0 )
        new_stream << "+(" << it->offset << ")";
      access_exp.push_back(new_stream.str());
    }
    print_array_access(curr_var,access_exp,curr_stream);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_extrapolate_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 stringstream& curr_stream)
{
  curr_stream << curr_var->symbol_name;
  if( curr_var->expr_domain->get_dim() != 0 ){
    int dim = iters.size() - scale_domain->get_dim();
    assert(dim ==  0);
    deque<string>access_exp;
    for(deque<scale_coeffs>::const_iterator it = scale_domain->scale_fn.begin();
        it != scale_domain->scale_fn.end() ; it++,dim++ ){
      stringstream new_stream ;
      new_stream  << "(" << iters[dim]->name  << ")" ;
      if( it->scale != 1 )
        new_stream << "*" << it->scale;
      if( it->offset != 0 )
        new_stream << "+(" << it->offset << ")";
      access_exp.push_back(new_stream.str());
    }
    print_array_access(curr_var,access_exp,curr_stream);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_extrapolate_point
(const c_var_info* curr_var, const domainfn_node* scale_domain,
 const domain_node* offset_domain, stringstream& curr_stream)
{
  curr_stream << curr_var->symbol_name;
  if( curr_var->expr_domain->get_dim() != 0 ){
    assert( scale_domain->get_dim() == (int)iters.size() );
    assert( offset_domain->get_dim() >= scale_domain->get_dim());
    deque<string> reverse_access_exp;
    deque<scale_coeffs>::const_reverse_iterator it =
      scale_domain->scale_fn.rbegin();
    deque<c_iterator*>::const_reverse_iterator jt = iters.rbegin();
    deque<range_coeffs>::const_reverse_iterator kt
      = offset_domain->range_list.rbegin();
    for( ; jt != iters.rend() ; it++, jt++ , kt++ ){
      stringstream new_stream ;
      new_stream  << "(" <<  (*jt)  << ")" ;
      if( it->scale != 1 )
        new_stream << "*" << it->scale;
      if( it->offset != 0 )
        new_stream << "+(" << it->offset << ")";
      if( !kt->lb->is_default() ){
        new_stream << "+" ;
        PrintCParametricExpr(kt->lb,new_stream);
      }
      reverse_access_exp.push_back(new_stream.str());
    }
    while( kt != offset_domain->range_list.rbegin() ) {
      stringstream new_stream;
      PrintCParametricExpr(kt->lb,new_stream);
      reverse_access_exp.push_back(new_stream.str());
      kt++;
    }
    deque<string> access_exp;
    for( deque<string>::reverse_iterator jt = reverse_access_exp.rbegin() ;
         jt != reverse_access_exp.rend() ; jt++ ){
      access_exp.push_back(*jt);
    }
    print_array_access(curr_var,access_exp,curr_stream);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_assignment_stmt
(const c_symbol_info* curr_symbol, std::string& rhs_expr,
 std::stringstream& curr_stream)
{
  const c_var_info* curr_var = curr_symbol->var;
  assert(curr_var->expr_domain);
  if( curr_var->expr_domain->get_dim() != 0 ){
    if( curr_symbol->offset_domain ){
      print_domain_point(curr_var,curr_stream,curr_symbol->offset_domain);
    }
    else if( curr_symbol->scale_domain ){
      if( curr_symbol->offset_domain){
        print_extrapolate_point
          (curr_var,curr_symbol->scale_domain,
           curr_symbol->offset_domain,curr_stream);
      }
      else{
        print_extrapolate_point(curr_var,curr_symbol->scale_domain,curr_stream);
      }
    }
    else{
      print_domain_point(curr_var,curr_stream);
    }
  }
  else{
    curr_stream << "*(" << curr_var->symbol_name << ")";
  }
  if( curr_symbol->field_name.compare("") != 0 )
    curr_stream << "." << curr_symbol->field_name ;

  curr_stream << " = " << rhs_expr << ";";
}


///-----------------------------------------------------------------------------
void PrintC::init_rhs
(const vector_expr_node* curr_expr, const domain_node* expr_domain,
 stringstream& curr_stream, c_symbol_table& fn_bindings,
 const domainfn_node* scale_domain, const domain_node* offset_domain)
{
  ///Need a new variable even if accessing a struct of the curr_expr, the
  ///field_name is not transmitted, it is handled later in print_rhs_var
  string field_name = "";
  if( curr_expr->get_base_type().type == T_STRUCT &&
      curr_expr->get_access_field() != -1 ){
    field_name.assign
      (curr_expr->get_base_type().
       struct_info->fields[curr_expr->get_access_field()].field_name);
  }

  ///Create a symbol only if the RHS is not vec_id. If it is , use previous
  ///symbol
  if( ( curr_expr->get_type() != VEC_ID && curr_expr->get_dim() != 0 )
      || scale_domain != NULL || offset_domain != NULL ||
      field_name.compare("") != 0 ){
    string rhs_var = get_new_var();
    c_var_info* new_var =
      printMalloc(rhs_var,curr_expr->get_data_type(),expr_domain);
    domain_node* sub_domain = NULL;
    if( offset_domain )
      sub_domain = new domain_node(offset_domain);
    fn_bindings.AddSymbol(curr_expr,new_var,scale_domain,sub_domain,"");
    print_vector_expr(curr_expr,fn_bindings);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_rhs_var
(const vector_expr_node* curr_expr,stringstream& curr_stream,
 c_symbol_table& fn_bindings, const domain_node* sub_domain,
 const domainfn_node* scale_domain)
{
  ///Check if the symbol is already present, true for all vec_id expressions
  if( !fn_bindings.IsPresent(curr_expr) ){
    const vec_id_expr_node* curr_vec_id =
      dynamic_cast<const vec_id_expr_node*>(curr_expr);
    assert(curr_vec_id);
    const set<vector_expr_node*> curr_defn_set = curr_vec_id->get_defn();
    assert(curr_defn_set.size() == 1);
    const vector_expr_node* curr_defn = *(curr_defn_set.begin());

    const c_symbol_info* defn_symbol = fn_bindings.GetSymbolInfo(curr_defn);
    ///Check if the argument is a scalar, for scalars the symbol name is the one
    ///to be used.
    if( curr_defn->get_dim() !=  0 ){
      const domain_node* defn_sub_domain = defn_symbol->offset_domain ;
      if( defn_sub_domain ){ ///If definition has a sub-domain, already, need to
                             ///compute a new sub-domain in case current vec_id
                             ///expression has a sub-domain
        domain_node* curr_sub_domain = new domain_node(sub_domain);
        if( sub_domain ){
          curr_sub_domain->compute_intersection(sub_domain);
        }
        if( scale_domain ){
          print_stencil_point
            (defn_symbol->var,scale_domain,curr_sub_domain,curr_stream,NULL);
        }
        else{
          print_domain_point(defn_symbol->var,curr_stream,curr_sub_domain);
        }
        delete curr_sub_domain;
        return ;
      }
      else{
        if( scale_domain ){
          if( sub_domain ){
            print_stencil_point
              (defn_symbol->var,scale_domain,sub_domain,curr_stream,NULL);
          }
          else{
            print_stencil_point(defn_symbol->var,scale_domain,curr_stream,NULL);
          }
        }
        else{
          if( sub_domain )
            print_domain_point(defn_symbol->var,curr_stream,sub_domain);
          else
            print_domain_point(defn_symbol->var,curr_stream);
        }
      }
    }
  }
  else{
    const c_symbol_info* curr_symbol = fn_bindings.GetSymbolInfo(curr_expr);
    if( scale_domain ){
      if( sub_domain ){
        print_stencil_point
          (curr_symbol->var,scale_domain,sub_domain,curr_stream,NULL);
      }
      else{
        print_stencil_point(curr_symbol->var,scale_domain,curr_stream,NULL);
      }
    }
    else{
      if( sub_domain )
        print_domain_point(curr_symbol->var,curr_stream,sub_domain);
      else
        print_domain_point(curr_symbol->var,curr_stream);
    }
    if( curr_expr->get_base_type().type == T_STRUCT &&
        curr_expr->get_access_field() != -1 ){
      curr_stream << "." <<
        curr_expr->get_base_type().
        struct_info->fields[curr_expr->get_access_field()].field_name;
    }
    fn_bindings.RemoveSymbol(curr_expr);
  }
}


///-----------------------------------------------------------------------------
string PrintC::print_mathfn_expr
(const math_fn_expr_node* curr_mathfn, c_symbol_table& fn_bindings,
 bool handle_bdys)
{
  stringstream curr_stream;
  curr_stream << curr_mathfn->get_name() << "(" ;
  const deque<expr_node*> fn_args = curr_mathfn->get_args();
  if( fn_args.size() != 0 ){
    deque<expr_node*>::const_iterator it = curr_mathfn->get_args().begin();
    curr_stream << print_expr(*it,fn_bindings,handle_bdys);
    for( it++ ; it != curr_mathfn->get_args().end() ; it++){
      curr_stream << "," << print_expr(*it,fn_bindings,handle_bdys) ;
    }
  }
  curr_stream << ")" ;
  if( generate_affine )
    return curr_stream.str();
  else{
    string lhs_var=get_new_temp_var(curr_mathfn->get_data_type(),output_buffer);
    output_buffer->indent();
    output_buffer->buffer << lhs_var << " = " << curr_stream.str() << ";";
    output_buffer->newline();
    return lhs_var;
  }
}


///-----------------------------------------------------------------------------
string PrintC::print_ternary_exp
(const ternary_expr_node* curr_expr, c_symbol_table& fn_bindings,
 bool handle_bdys)
{
  stringstream curr_stream;
  if( generate_affine ){
    curr_stream << "(" <<
      print_expr(curr_expr->get_bool_expr(),fn_bindings,handle_bdys) << " ? " <<
      print_expr(curr_expr->get_true_expr(),fn_bindings,handle_bdys) << " : " <<
      print_expr(curr_expr->get_false_expr(),fn_bindings,handle_bdys) << ")";
    return curr_stream.str();
  }
  else{
    string output_var =
      get_new_temp_var(curr_expr->get_data_type(),output_buffer);

    curr_stream << "if(" <<
      print_expr(curr_expr->get_bool_expr(),fn_bindings,handle_bdys) << "){" ;
    output_buffer->indent();
    output_buffer->buffer << curr_stream.str();
    output_buffer->newline();

    output_buffer->increaseIndent();
    curr_stream.str("");
    curr_stream << output_var << " = " <<
      print_expr(curr_expr->get_true_expr(),fn_bindings,handle_bdys) << ";" ;
    output_buffer->indent();
    output_buffer->buffer << curr_stream.str();
    output_buffer->newline();

    output_buffer->decreaseIndent();
    output_buffer->indent();
    output_buffer->buffer << "}";
    output_buffer->newline() ;
    output_buffer->indent();
    output_buffer->buffer << "else {";
    output_buffer->newline();
    output_buffer->increaseIndent();
    curr_stream.str("");
    curr_stream << output_var << " = " <<
      print_expr(curr_expr->get_false_expr(),fn_bindings,handle_bdys) << ";" ;
    output_buffer->indent();
    output_buffer->buffer << curr_stream.str();
    output_buffer->newline();

    output_buffer->decreaseIndent();
    output_buffer->indent();
    output_buffer->buffer << "}";
    output_buffer->newline();
    return output_var;
  }
}


///-----------------------------------------------------------------------------
string PrintC::print_expr_op
(const expr_op_node* curr_expr, c_symbol_table& fn_bindings, bool handle_bdys)
{
  stringstream curr_stream;
  string lhs_string =
    print_expr(curr_expr->get_lhs_expr(),fn_bindings,handle_bdys);
  string rhs_string =
    print_expr(curr_expr->get_rhs_expr(),fn_bindings,handle_bdys);
  curr_stream << "(" << lhs_string  << " " <<
    get_op_string(curr_expr->get_op()) << " " << rhs_string << ")" ;
  if( generate_affine )
    return curr_stream.str();
  else{
    string lhs_var = get_new_temp_var(curr_expr->get_data_type(),output_buffer);
    output_buffer->indent();
    output_buffer->buffer << lhs_var << " = " << curr_stream.str() << ";" ;
    output_buffer->newline();
    return lhs_var;
  }
}


///-----------------------------------------------------------------------------
/// Helper function to print stencil operations
string PrintC::print_stencil_op_helper
(const c_symbol_info* curr_symbol, const domainfn_node* scale_fn,
 const data_types& base_data_type, int field_num, bool handle_bdys)
{
  stringstream curr_stream;
  if ( curr_symbol->var->expr_domain == NULL ||
       curr_symbol->var->expr_domain->get_dim() == 0 )
    curr_stream << curr_symbol->var->symbol_name;
  else if( curr_symbol->offset_domain )
    print_stencil_point
      (curr_symbol->var,scale_fn,curr_symbol->offset_domain,curr_stream,
       ( handle_bdys ? curr_symbol->bdy_condn : NULL ) );
  else
    print_stencil_point
      (curr_symbol->var,scale_fn,curr_stream,
       ( handle_bdys ? curr_symbol->bdy_condn : NULL ) );

  if( base_data_type.type == T_STRUCT ){
    if( field_num != -1 ){
      assert
        (base_data_type.struct_info != NULL &&
         field_num < (int)(base_data_type.struct_info->fields.size())  );
      curr_stream << "." <<
        base_data_type.struct_info->fields[field_num].field_name;
    }
  }
  return curr_stream.str();
}


///-----------------------------------------------------------------------------
string PrintC::print_stencil_op
(const stencil_op_node* curr_expr, c_symbol_table& fn_bindings,bool handle_bdys)
{
  stringstream curr_stream;
  const vector_expr_node* curr_var = curr_expr->get_var();

  const c_symbol_info* curr_var_symbol = fn_bindings.GetSymbolInfo(curr_var);
  return
    print_stencil_op_helper
    (curr_var_symbol,curr_expr->get_scale_fn(),curr_expr->get_base_type(),
     curr_expr->get_access_field(),handle_bdys);
}


string PrintC::print_value_expr(const expr_node* curr_expr)
{
  stringstream curr_stream;
  curr_stream.setf(ios::fixed);
  switch (curr_expr->get_data_type().type) {
  case T_DOUBLE: {
    double val = static_cast<const value_node<double>*>(curr_expr)->get_value();
    if( val < 0.0 )
      curr_stream << "(" << val << ")";
    else
      curr_stream << val ;
  }
    break;
  case T_FLOAT: {
    float val = static_cast<const value_node<float>*>(curr_expr)->get_value();
    if( val < 0.0 )
      curr_stream << "(" << val  <<"f)" ;
    else
      curr_stream << val << "f" ;
  }
    break;
  case T_INT: {
    int value = static_cast<const value_node<int>*>(curr_expr)->get_value();
    if( value < 0 )
      curr_stream << "(" << value << ")" ;
    else
      curr_stream << value ;
  }
    break;
  default:
    assert((0) && ("[ME] : Within Code-generator : Unknown value type"));
  }
  return curr_stream.str();
}

string PrintC::print_id_expr(const id_expr_node* curr_expr )
{
  string curr_var_name = curr_expr->get_name();
  c_scalar_symbol curr_symbol = scalar_symbols.find(curr_var_name);
  assert((curr_symbol != scalar_symbols.end()) &&
         "Undefined scalar symbol used");
  return  curr_symbol->second.name;
  // return curr_expr->get_name();
}


///-----------------------------------------------------------------------------
string PrintC::print_array_expr
(const array_access_node* curr_expr, c_symbol_table& fn_bindings,
 bool handle_bdys )
{
  stringstream curr_stream;
  const vector_defn_node* curr_var = curr_expr->get_var();

  const c_symbol_info* curr_var_symbol = fn_bindings.GetSymbolInfo(curr_var);
  curr_stream << curr_var_symbol->var->symbol_name;

  deque<string> curr_indices;
  const deque<expr_node*>& index_exprs = curr_expr->get_index_exprs();
  for( deque<expr_node*>::const_iterator it = index_exprs.begin() ;
       it != index_exprs.end() ; it++){
    string index = print_expr(*it, fn_bindings, handle_bdys);
    curr_indices.push_back(index);
  }
  print_array_access(curr_var_symbol->var,curr_indices,curr_stream);
  return curr_stream.str();
}


///-----------------------------------------------------------------------------
string PrintC::print_expr
(const expr_node* curr_expr, c_symbol_table& fn_bindings, bool handle_bdys)
{
  stringstream curr_stream;
  switch(curr_expr->get_s_type()){
  case S_ID:
    curr_stream << print_id_expr(static_cast<const id_expr_node*>(curr_expr));
    break;
  case S_VALUE:
    curr_stream << print_value_expr(curr_expr);
    break;
  case S_UNARYNEG: {
    curr_stream << "(-" <<
      print_expr
      (static_cast<const unary_neg_expr_node*>(curr_expr)->get_base_expr(),
       fn_bindings,handle_bdys) << ")" ;
    break;
  }
  case S_MATHFN: {
    curr_stream <<
      print_mathfn_expr
      (static_cast<const math_fn_expr_node*>(curr_expr),fn_bindings,
       handle_bdys);
    break;
  }
  case S_TERNARY:{
    curr_stream <<
      print_ternary_exp
      (static_cast<const ternary_expr_node*>(curr_expr),fn_bindings,
       handle_bdys);
    break;
  }
  case S_BINARYOP: {
    curr_stream <<
      print_expr_op
      (static_cast<const expr_op_node*>(curr_expr),fn_bindings,handle_bdys);
    break;
  }
  case S_STENCILOP: {
    curr_stream <<
      print_stencil_op
      (static_cast<const stencil_op_node*>(curr_expr),fn_bindings,handle_bdys);
    break;
  }
  case S_ARRAYACCESS: {
    curr_stream <<
      print_array_expr
      (static_cast<const array_access_node*>(curr_expr),fn_bindings,
       handle_bdys);
    break;
  }
  default:
    assert((0) && ("[ME] : Within Code-generator : Unkown expr_type"));
  }
  return curr_stream.str();
}


///-----------------------------------------------------------------------------
void PrintC::print_pt_stencil
(const pt_struct_node* curr_expr, c_symbol_table& fn_bindings,
 deque<string>& return_exprs, bool handle_bdys)
{
  const deque<expr_node*>& field_exprs = curr_expr->get_field_exprs();
  for( deque<expr_node*>::const_iterator it = field_exprs.begin() ;
       it != field_exprs.end() ; it++ ){
    return_exprs.push_back(print_expr((*it),fn_bindings,handle_bdys));
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_stencil_fn
(const stencilfn_defn_node* curr_stencil_fn,
 c_symbol_table& fn_bindings, deque<string>& return_vals, bool handle_bdys)
{
  const deque<pt_stmt_node*> fn_body = curr_stencil_fn->get_body();
  for( deque<pt_stmt_node*>::const_iterator it = fn_body.begin() ;
       it != fn_body.end() ; it++ ){
    string curr_var_name = (*it)->get_lhs();
    c_scalar_symbol curr_scalar_symbol = scalar_symbols.find(curr_var_name);
    if( curr_scalar_symbol == scalar_symbols.end() )
      scalar_symbols.insert
        (make_pair
         (curr_var_name, print_expr((*it)->get_rhs(),fn_bindings,handle_bdys)));
    else
      curr_scalar_symbol->second.name =
        print_expr((*it)->get_rhs(),fn_bindings,handle_bdys);

  }
  const expr_node* return_expr = curr_stencil_fn->get_return_expr();
  if( return_expr->get_s_type() == S_STRUCT ){
    print_pt_stencil
      (static_cast<const pt_struct_node*>(return_expr),
       fn_bindings,return_vals,handle_bdys);
  }
  else{
    return_vals.push_back( print_expr(return_expr,fn_bindings,handle_bdys));
  }

  if(!generate_affine)
    scalar_symbols.clear();
}


///-----------------------------------------------------------------------------
/// The following functions
/// 1) Evaluates the arguments to create a new symbol for it
/// 2) Adds the symbol to input_expr_symbols
/// 3) Computes domains of each argument used later in print_fn_body
void PrintC::init_arg_info
(const vector_expr_node* curr_argument, bdy_info* arg_bdy_condn,
 const vector_defn_node* curr_param, c_symbol_table& fn_bindings,
 deque<c_symbol_info*>& input_expr_symbols,
 deque<const domain_node*>& arg_domains) {

  domain_node* arg_expr_domain =
    new domain_node(curr_argument->get_expr_domain());

  domain_node* sub_domain = NULL;
  if( curr_argument->get_dim() != 0 ){
    /// Evaluate the current argument expression
    init_rhs(curr_argument,arg_expr_domain,output_buffer->buffer,fn_bindings);

    if( !fn_bindings.IsPresent(curr_argument) ){
      /// For vec_id expressions, there is no symbol, track back to the symbol
      /// associated with the definition
      const vec_id_expr_node* curr_vec_id =
        dynamic_cast<const vec_id_expr_node*>(curr_argument);
      const set<vector_expr_node*> curr_defn_set = curr_vec_id->get_defn();
      assert(curr_defn_set.size() == 1 &&
             "Multiple reaching defintions to a domain reference");

      /// Get the defn that reaches this reference
      const vector_expr_node* curr_defn = *(curr_defn_set.begin());
      const c_symbol_info* defn_symbol = fn_bindings.GetSymbolInfo(curr_defn);
      const domain_node* defn_sub_domain = defn_symbol->offset_domain;
      if( defn_sub_domain )
        sub_domain  = new domain_node(defn_sub_domain);
      if( curr_argument->get_sub_domain() ){
        if( sub_domain )
          sub_domain->compute_intersection(curr_argument->get_sub_domain());
        else
          sub_domain = new domain_node(curr_argument->get_sub_domain());
      }
      input_expr_symbols.push_back
        (getNewSymbol(defn_symbol->var,NULL,sub_domain,"",arg_bdy_condn));
    }
    else{
      const c_symbol_info* curr_arg_symbol =
        fn_bindings.GetSymbolInfo(curr_argument);
      if( curr_argument->get_sub_domain() ){
        sub_domain = new domain_node(curr_argument->get_sub_domain());
      }
      input_expr_symbols.push_back
        (getNewSymbol
         (curr_arg_symbol->var,NULL,sub_domain,curr_arg_symbol->field_name,
          arg_bdy_condn));

      /// Collect data structures associated with prev temporary symbol
      fn_bindings.RemoveSymbol(curr_argument);
    }
  }
  else{
    string curr_value;
    if( curr_argument->get_type() == VEC_ID ){
      c_symbol_info* defn_symbol = FindDefnSymbol(curr_argument,fn_bindings);
      assert(defn_symbol && "Couldnt Find Defn symbol of scalar argument");
      curr_value = defn_symbol->var->symbol_name;
    }
    else{
      assert
        (curr_argument->get_type() == VEC_SCALAR &&
         "Scalar argument is not a number");
      curr_value =
        print_value_expr(static_cast<const expr_node*>(curr_argument));
    }
    c_var_info* new_value_var =
      init_value_var(curr_value,curr_argument->get_data_type());
    input_expr_symbols.push_back
      (getNewSymbol(new_value_var,NULL,NULL,""));
  }
  if( curr_argument->get_sub_domain() ){
    arg_expr_domain->compute_intersection(curr_argument->get_sub_domain());
  }
  arg_expr_domain->realign_domain();
  arg_domains.push_back(arg_expr_domain);
}


///-----------------------------------------------------------------------------
/// Evaluate the function arguments and create symbols to be used for the params
bool PrintC::init_fn_args
(const fnid_expr_node* curr_fn, c_symbol_table& fn_bindings,
 deque<c_symbol_info*>& arg_symbols,
 deque<const domain_node*>& arg_domains){
  const fn_defn_node* fn_defn = curr_fn->get_defn();

  const deque<vector_defn_node*> fn_params = fn_defn->get_args();
  deque<vector_defn_node*>::const_iterator jt = fn_params.begin();
  const deque<arg_info>& curr_args = curr_fn->get_args();

  printf("Function : %s\n",curr_fn->get_name());
  for( deque<arg_info>::const_iterator it_begin = curr_args.begin(),
         it_end = curr_args.end() ; it_begin != it_end ; it_begin++, jt++){
    vector_expr_node* curr_argument = it_begin->arg_expr;

    /// By default do what is done for untiled code
    init_arg_info
      (it_begin->arg_expr,it_begin->bdy_condn,*jt,fn_bindings,
       arg_symbols,arg_domains);
  }
  return false;
}


///-----------------------------------------------------------------------------
///Function to delete the symbols allocated during init_arg_info
void PrintC::deleteArgSymbols(deque<c_symbol_info*>& arg_symbols)
{
  /// Delete the input symbols passed as well, they arent needed now
  for( deque<c_symbol_info*>::iterator symbol_iter = arg_symbols.begin(),
         symbol_end = arg_symbols.end() ; symbol_iter != symbol_end ;
       symbol_iter++){
    if( (*symbol_iter)->var->expr_domain == NULL ){
      /// THis is a scalar
      delete (*symbol_iter)->var;
    }
    delete *symbol_iter;
  }
  arg_symbols.clear();
}

///-----------------------------------------------------------------------------
/// extends the loop domain by the specified offsets
void PrintC::ExpandDomain
(domain_node* loop_domain, const deque<offset_hull>& offsets) const
{
  deque<offset_hull>::const_iterator offset_iter = offsets.begin();
  for(deque<range_coeffs>::iterator dim_iter = loop_domain->range_list.begin(),
        dim_end = loop_domain->range_list.end(); dim_iter != dim_end ;
      dim_iter++, offset_iter++){
    dim_iter->lb = dim_iter->lb->multiply(offset_iter->scale);
    dim_iter->lb = dim_iter->lb->add(offset_iter->max_negetive);
    dim_iter->ub = dim_iter->ub->multiply(offset_iter->scale);
    dim_iter->ub = dim_iter->ub->add(offset_iter->max_positive);
  }
}


///-----------------------------------------------------------------------------
/// Generate the body of the unrolled code
void PrintC::print_unrolled_code_body
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
      next_iterator << "(" << orig_iterator_name << "+" <<
        num_unroll_generated << ")";
      curr_iterator->name = next_iterator.str();
      PrintC::print_unrolled_code_body
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
/// Generate the Header for the remainder loop
void PrintC::print_remainder_loop_header
(string& iterator, parametric_exp* ub, int unrollFactor)
{
  assert
    (unrollFactor >= 2 &&
     "Illegal remainder loop generation for Unroll Factor < 2");
  if (unrollFactor > 2) {
    output_buffer->indent();
    output_buffer->buffer << "for ( ; " << iterator << " <= ";
    PrintCParametricExpr(ub, output_buffer->buffer);
    output_buffer->buffer << "; " << iterator << "++) {";
    output_buffer->newline();
  }
  else {
    output_buffer->indent();
    output_buffer->buffer << "if (" << iterator << " <= ";
    PrintCParametricExpr(ub, output_buffer->buffer);
    output_buffer->buffer <<" ) {";
    output_buffer->newline();
  }
  output_buffer->increaseIndent();
}

///-----------------------------------------------------------------------------
void PrintC::print_remainder_loop_footer()
{
  output_buffer->decreaseIndent();
  output_buffer->indent();
  output_buffer->buffer << "}";
  output_buffer->newline();
}


///-----------------------------------------------------------------------------
/// Generate unrolled code
void PrintC::print_unrolled_code
(int curr_dim, int loop_dim, const fnid_expr_node* curr_fn,
 deque<c_symbol_info*>& input_exprs, c_symbol_info* output_symbol,
 domain_node* loop_domain, bool is_bdy, deque<int>& curr_unroll_factors) {
   const deque<range_coeffs>& range_list = loop_domain->range_list;
   if (curr_dim < loop_dim) {
      int curr_unroll_factor = curr_unroll_factors[curr_dim];
      if (parametric_exp::is_equal
          (range_list[curr_dim].lb,range_list[curr_dim].ub)) {
        curr_unroll_factors[curr_dim] = 1;
        curr_unroll_factor = 1;
      }
      c_iterator* iterator =
        printForHeader
        (range_list[curr_dim].lb, range_list[curr_dim].ub,
         curr_unroll_factor, curr_dim == range_list.size() -1);
      std::string orig_iterator = iterator->name;
      iters.push_back(iterator);

      /// Generate the code for the unrolled iterations
      PrintC::print_unrolled_code
        (curr_dim+1, loop_dim, curr_fn, input_exprs, output_symbol,
         loop_domain, is_bdy, curr_unroll_factors);

      /// Close the previous loop but dont remove the iterator
      output_buffer->decreaseIndent();
      output_buffer->indent();
      output_buffer->buffer << "}";
      output_buffer->newline();

      /// Generate remainder code if unroll factor > 1
      if (curr_unroll_factor > 1){
        bool addpragma = false;
        if (generate_omp_pragmas && !iterator->is_unit_trip_count()) {
          addpragma = true;
          for(deque<c_iterator*>::const_iterator itersIter = iters.begin();
                *itersIter != iterator; itersIter++){
            if( !((*itersIter)->is_unit_trip_count()) ){
              addpragma = false;
              break;
            }
          }
        }

        if (addpragma) {
          output_buffer->buffer << "#ifdef _OPENMP";
          output_buffer->newline();
          output_buffer->indent();
          output_buffer->buffer <<
            "if (omp_get_thread_num() == omp_get_num_threads() - 1) {";
          output_buffer->newline();
          output_buffer->buffer << "#endif";
          output_buffer->newline();
          output_buffer->increaseIndent();
        }

        output_buffer->indent();
        output_buffer->buffer << orig_iterator << " = ";
        parametric_exp* remainder_lb =
          parametric_exp::copy(range_list[curr_dim].ub);
        remainder_lb = remainder_lb->subtract(range_list[curr_dim].lb);
        remainder_lb = remainder_lb->add(1);
        remainder_lb = remainder_lb->divide(curr_unroll_factor);
        remainder_lb = remainder_lb->multiply(curr_unroll_factor);
        remainder_lb = remainder_lb->add(range_list[curr_dim].lb);
        PrintCParametricExpr(remainder_lb, output_buffer->buffer);
        output_buffer->buffer << ";";
        output_buffer->newline();
        delete remainder_lb;

        /// Remainder loop header
        print_remainder_loop_header
          (orig_iterator, range_list[curr_dim].ub, curr_unroll_factor);

        curr_unroll_factors[curr_dim] = 1;
        PrintC::print_unrolled_code
          (curr_dim+1, loop_dim, curr_fn, input_exprs, output_symbol,
           loop_domain, is_bdy, curr_unroll_factors);

        /// Remainder loop footer
        print_remainder_loop_footer();
        curr_unroll_factors[curr_dim] = curr_unroll_factor;

        if (addpragma) {
          output_buffer->decreaseIndent();
          output_buffer->buffer << "#ifdef _OPENMP";
          output_buffer->newline();
          output_buffer->indent();
          output_buffer->buffer << "}";
          output_buffer->newline();
          output_buffer->buffer << "#pragma omp barrier";
          output_buffer->newline();
          output_buffer->buffer << "#endif";
          output_buffer->newline();
        }
      }

      /// Delete the iterator
      c_iterator* last_iter = iters.back();
      delete last_iter;
      iters.pop_back();
   }
   else {
     print_unrolled_code_body
       (0, loop_domain->get_dim(), curr_unroll_factors, curr_fn, input_exprs,
        output_symbol, is_bdy);
   }
}


///-----------------------------------------------------------------------------
/// Generate untiled code for each patch
void PrintC::print_patch_untiled
(const fnid_expr_node* curr_fn, deque<c_symbol_info*>& input_exprs,
 c_symbol_info* output_symbol, domain_node* loop_domain,
 bool is_bdy, bool unrolled_code, deque<int>& curr_unroll_factors ) {
  if (unrolled_code) {
    domain_node* curr_loop_domain =
      compute_loop_domain(loop_domain, output_symbol->scale_domain);
    print_unrolled_code
      (0, curr_loop_domain->get_dim(), curr_fn, input_exprs, output_symbol,
       curr_loop_domain, is_bdy, curr_unroll_factors);
    delete curr_loop_domain;
  }
  else {
    /// Print the loops
    print_domain_loops_header
      (loop_domain, input_exprs, output_symbol->var,
       output_symbol->scale_domain, curr_unroll_factors);

    // ///Inline the stencil fn expression
    print_stencilfn_body(curr_fn, input_exprs, output_symbol, is_bdy);

    ///Close the loops and add the result variable to the symbol table
    print_domain_loops_footer(loop_domain, curr_unroll_factors);
  }
}


///-----------------------------------------------------------------------------
void PrintC::getCurrUnrollFactors(int ndims, deque<int>& curr_unroll_factors) {
  int num_loops = curr_unroll_factors.size();
  assert
    (num_loops == ndims && "Mismatch in loop dimension and num unroll factors");
  for (int i = 0 ; i < unroll_factors.size(); i++ ) {
    curr_unroll_factors[num_loops - 1 - i] = unroll_factors[i];
  }
}


///-----------------------------------------------------------------------------
/// Function to generate the loop nest that iterates over the difference of two
/// domains
void PrintC::printPatches
(const fnid_expr_node* curr_argument, deque<c_symbol_info*>& input_exprs,
 c_symbol_info* output_symbol, domain_node* loop_domain,
 const domain_node* outer_domain, const domain_node* inner_domain,
 int dim, int ndims, bool isBdy)
{
  if( dim < ndims ){
    /// Check if the outer domains inner edge is same as inner domains inner
    /// edge, nothing to do if thats the case
    if( !parametric_exp::is_equal
        (outer_domain->range_list[dim].lb,inner_domain->range_list[dim].lb)){
      parametric_exp* lb =
        parametric_exp::copy(outer_domain->range_list[dim].lb);
      parametric_exp* ub =
        parametric_exp::copy(inner_domain->range_list[dim].lb);
      ub = ub->subtract(1);
      loop_domain->add_range(lb,ub);
      /// Recursive call to compute patches in this range
      printPatches
        (curr_argument,input_exprs,output_symbol,loop_domain,
         outer_domain,inner_domain,dim+1,ndims,true);
      loop_domain->range_list.pop_back();
      delete lb;
      delete ub;
    }
    /// Generate the ptaches within the interior domain
    parametric_exp* inner_lb =
      parametric_exp::copy(inner_domain->range_list[dim].lb);
    parametric_exp* inner_ub =
      parametric_exp::copy(inner_domain->range_list[dim].ub);
    loop_domain->add_range(inner_lb,inner_ub);
    printPatches
      (curr_argument,input_exprs,output_symbol,loop_domain,
       outer_domain,inner_domain,dim+1,ndims,isBdy);
    loop_domain->range_list.pop_back();
    delete inner_lb;
    delete inner_ub;
    /// Check if the outer domains outer edge is same as inner domains outer
    /// edge, nothing to do if thats the case
    if( !parametric_exp::is_equal
        (outer_domain->range_list[dim].ub,inner_domain->range_list[dim].ub)){
      parametric_exp* lb =
        parametric_exp::copy(inner_domain->range_list[dim].ub);
      lb = lb->add(1);
      parametric_exp* ub =
        parametric_exp::copy(outer_domain->range_list[dim].ub);
      loop_domain->add_range(lb,ub);
      printPatches
        (curr_argument,input_exprs,output_symbol,loop_domain,
         outer_domain,inner_domain,dim+1,ndims,true);
      loop_domain->range_list.pop_back();
      delete lb;
      delete ub;
    }
  }
  else{
    /// End of recursion, actually generate code for whatever we have in loop
    /// domain
    deque<int> curr_unroll_factors(loop_domain->get_dim(), 1);
    if (generate_unroll_code)
      getCurrUnrollFactors(loop_domain->get_dim(), curr_unroll_factors);
    print_patch_untiled
      (curr_argument, input_exprs, output_symbol, loop_domain, isBdy,
       generate_unroll_code, curr_unroll_factors);
  }
}


///-----------------------------------------------------------------------------
/// function to print the stencil function body
void PrintC::print_stencilfn_body
( const fnid_expr_node* curr_fn, deque<c_symbol_info*>& input_exprs,
  c_symbol_info* output_symbol, bool is_bdy)
{
  const stencilfn_defn_node* curr_stencil =
    dynamic_cast<const stencilfn_defn_node*>(curr_fn->get_defn());
  assert(curr_stencil && "Wrong function type while printing stencil body");

  const deque<vector_defn_node*>& fn_params = curr_stencil->get_args();
  assert
    (input_exprs.size() == fn_params.size() &&
     "No. of input args not equal to no. of function params in stencil fn");

  c_symbol_table fn_bindings;
  deque<vector_defn_node*>::const_iterator param_iter = fn_params.begin();
  for( deque<c_symbol_info*>::iterator symbol_iter = input_exprs.begin(),
         symbol_end = input_exprs.end() ; symbol_iter != symbol_end ;
       symbol_iter++, param_iter++)
    fn_bindings.symbol_table.insert(make_pair(*param_iter,*symbol_iter));

  ///Inline the stencil fn expression
  deque<string> stencil_return_exprs;
  print_stencil_fn(curr_stencil,fn_bindings,stencil_return_exprs,is_bdy);
  if( curr_fn->get_data_type().type == T_STRUCT ){
    data_types elem_type = curr_fn->get_data_type();
    assert
      (elem_type.struct_info &&
       elem_type.struct_info->fields.size()==stencil_return_exprs.size());
    for( int i =0 ; i < (int)stencil_return_exprs.size() ; i++ ){
      output_buffer->indent();
      output_symbol->field_name =
        elem_type.struct_info->fields[i].field_name;
      print_assignment_stmt
        (output_symbol,stencil_return_exprs[i],output_buffer->buffer);
      output_buffer->newline();
    }
  }
  else{
    assert(stencil_return_exprs.size() == 1 );
    output_buffer->indent();
    print_assignment_stmt
      (output_symbol,stencil_return_exprs[0],output_buffer->buffer);
    output_buffer->newline();
  }

  /// Not de-allocating the symbols since they need to be reused.
  fn_bindings.symbol_table.clear();
}


///-----------------------------------------------------------------------------
/// Function to generate the body of the vector function
void PrintC::print_vectorfn_body
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
  c_symbol_table new_fn_bindings;
  deque<vector_defn_node*>::const_iterator fn_params_iter = fn_params.begin();
  for( deque<c_symbol_info*>::iterator symbol_iter = input_exprs.begin(),
         symbol_end = input_exprs.end() ; symbol_iter != symbol_end ;
       symbol_iter++, fn_params_iter++){
    new_fn_bindings.symbol_table.insert
      (make_pair(*fn_params_iter,*symbol_iter));
  }

  ///Need to recompute the domain of expressions within the vector
  ///function based ont he domain of the arguments
  curr_vector_fn->compute_domain(arg_domains);
  new_fn_bindings.AddSymbol
    (static_cast<const vectorfn_defn_node*>(fn_defn)->get_return_expr(),
     curr_output_symbol->var, curr_output_symbol->scale_domain,
     curr_output_symbol->offset_domain, curr_output_symbol->field_name);
  print_vectorfn_body_helper(curr_vector_fn,new_fn_bindings,true);

  /// Remove the input_exprs from the symbol table, they are deallocated later
  fn_params_iter = fn_params.begin();
  for(deque<vector_defn_node*>::const_iterator fn_params_end= fn_params.end();
      fn_params_iter != fn_params_end ; fn_params_iter++ ){
    map<const vector_expr_node*,c_symbol_info*>::iterator var =
      new_fn_bindings.symbol_table.find(*fn_params_iter);
    new_fn_bindings.symbol_table.erase(*fn_params_iter);
  }
}



///-----------------------------------------------------------------------------
/// Generate code for function applications
void PrintC::print_fn_body
(const fnid_expr_node* curr_fn, deque<c_symbol_info*>& input_exprs,
 deque<const domain_node*>& arg_domains, c_symbol_info* curr_output_symbol)
{
  fn_defn_node* fn_defn = curr_fn->get_defn();

  if( dynamic_cast<const stencilfn_defn_node*>(fn_defn) ){

    ////Stencil fns are inlined by default.
    stencilfn_defn_node* curr_stencil =
      dynamic_cast<stencilfn_defn_node*>(fn_defn);

    ///The domain of the stencil, the function compute_domain sets the domain of
    ///all parameters to those in arg_domains
    const domain_node* stencil_domain =
      curr_stencil->compute_domain(arg_domains);

    /// Compute the interior of the loop domain
    deque<offset_hull> interior_offset;
    computeOffsets(curr_fn,interior_offset,curr_output_symbol->scale_domain);
    domain_node* interior_loop_domain = new domain_node(stencil_domain);
    ApplyOffset(interior_loop_domain,interior_offset);

    /// Compute the exterior of the loop domain
    deque<offset_hull> exterior_offset;
    computeOffsets
      (curr_fn,exterior_offset,curr_output_symbol->scale_domain,true);
    domain_node* exterior_loop_domain = new domain_node(stencil_domain);
    ApplyOffset(exterior_loop_domain,exterior_offset);

    domain_node* loop_domain = new domain_node();
    printPatches
      (curr_fn,input_exprs,curr_output_symbol,loop_domain,exterior_loop_domain,
       interior_loop_domain, 0, stencil_domain->get_dim(), false);
    delete loop_domain;
    delete exterior_loop_domain;
    delete interior_loop_domain;
  }
  else{
    print_vectorfn_body(curr_fn,input_exprs,arg_domains,curr_output_symbol);
  }
}



///-----------------------------------------------------------------------------
/// Generate code for a function application expressions
void PrintC::print_fnid_expr
(const fnid_expr_node* curr_fn,c_symbol_table& fn_bindings){
  deque<c_symbol_info*> arg_symbols;
  deque<const domain_node*> arg_domains;

  /// Initialize the arguments
  bool is_tiled =
    init_fn_args(curr_fn,fn_bindings,arg_symbols,arg_domains);

  /// Generate code for the function body
  c_symbol_info* output_symbol = fn_bindings.GetSymbolInfo(curr_fn);
  print_fn_body(curr_fn,arg_symbols,arg_domains,output_symbol);
  for( deque<const domain_node*>::iterator it = arg_domains.begin() ;
       it != arg_domains.end() ; it++ )
    delete (*it);
  arg_domains.clear();
  deleteArgSymbols(arg_symbols);
}


///-----------------------------------------------------------------------------
void PrintC::print_domainfn_expr
(const vec_domainfn_node* curr_domainfn, c_symbol_table& fn_bindings)
{
  ///Get the output symbol to be used for the RHS
  const c_symbol_info* curr_output_symbol =
    fn_bindings.GetSymbolInfo(curr_domainfn);
  const domain_node* expr_domain = curr_output_symbol->var->expr_domain;

  ///Precompute the base expression
  const vector_expr_node* base_expr = curr_domainfn->get_base_expr();
  init_rhs
    (base_expr,curr_domainfn->get_expr_domain(),
     output_buffer->buffer,fn_bindings);
  c_symbol_info* base_symbol = FindDefnSymbol(base_expr,fn_bindings);
  assert(base_symbol != NULL &&
         "Couldnt find symbol for base of domainfn expr");

  const domainfn_node* scale_domain = curr_domainfn->get_scale_fn();

  //Compute the loop domain
  domain_node* loop_domain = new domain_node(curr_domainfn->get_expr_domain());
  // printf("Loop Domain Before : ");
  // loop_domain->print_node(stdout);
  // printf("\n");
  // printf("Scale Domain : ");
  // scale_domain->print_node(stdout);
  // printf("\n");
  // printf("Expr Domain : ");
  // curr_domainfn->get_expr_domain()->print_node(stdout);
  // printf("\n");
  deque<range_coeffs>::iterator it = loop_domain->range_list.begin();
  deque<scale_coeffs>::const_iterator jt = scale_domain->scale_fn.begin();
  for( ; it != loop_domain->range_list.end() ; it++,jt++ ){
    it->lb = it->lb->add(-MIN(0,FLOOR(jt->offset,jt->scale)));
    it->ub = it->ub->subtract(MAX(0,FLOOR(jt->offset,jt->scale)));
  }
  // printf("Loop Domain : ");
  // loop_domain->print_node(stdout);
  // printf("\n");

  ///Print the surrounding loops, iterate through points of the result domain in
  ///this case
  deque<c_symbol_info*> input_exprs;
  input_exprs.push_back(base_symbol);
  deque<int> curr_unroll_factors(loop_domain->get_dim(), 1);
  print_domain_loops_header
    (loop_domain, input_exprs, curr_output_symbol->var,
     curr_output_symbol->scale_domain, curr_unroll_factors);

  ///Print the LHS reference
  output_buffer->indent();
  stringstream rhs_stream;
  print_rhs_var
    (base_expr,rhs_stream,fn_bindings,curr_domainfn->get_sub_domain(),
     scale_domain);
  string rhs_string = rhs_stream.str();
  print_assignment_stmt(curr_output_symbol,rhs_string,output_buffer->buffer);
  output_buffer->newline();

  ///Finish the loop
  print_domain_loops_footer(loop_domain, curr_unroll_factors);

  delete loop_domain;
}


///-----------------------------------------------------------------------------
void PrintC::print_make_struct
(const make_struct_node* curr_struct_node, c_symbol_table& fn_bindings)
{
  const c_symbol_info* curr_output_symbol =
    fn_bindings.GetSymbolInfo(curr_struct_node);
  const deque<vector_expr_node*>& field_inputs =
    curr_struct_node->get_field_inputs();

  int field_num = 0;
  for( deque<vector_expr_node*>::const_iterator it = field_inputs.begin() ;
       it != field_inputs.end() ; it++, field_num++ ){
    assert(curr_output_symbol->field_name.compare("") == 0 &&
           curr_struct_node->get_base_type().type == T_STRUCT &&
           curr_struct_node->get_base_type().struct_info);

    ///Set the output variable of the RHS variable, with the same information as
    ///the LHS, but updated field_num
    fn_bindings.AddSymbol
      (*it,curr_output_symbol->var, curr_output_symbol->scale_domain,
       curr_output_symbol->offset_domain,
       curr_struct_node->get_base_type().
       struct_info->fields[field_num].field_name);
    print_vector_expr(*it,fn_bindings);
    fn_bindings.RemoveSymbol(*it);
  }

}


///-----------------------------------------------------------------------------
void PrintC::print_compose_expr
(const compose_expr_node* curr_compose_expr, c_symbol_table& fn_bindings)
{
  const c_symbol_info* curr_output_symbol =
    fn_bindings.GetSymbolInfo(curr_compose_expr);
  const deque<pair<domain_desc_node*,vector_expr_node*> >& expr_list =
    curr_compose_expr->get_expr_list();

  for( deque<pair<domain_desc_node*,vector_expr_node*> >::const_iterator it =
         expr_list.begin() ; it != expr_list.end() ; it++ ){
    const vector_expr_node* curr_expr = it->second;
    ///if the rhs expression is a scalar, craete a loop to iterate throught the
    ///specified domain
    if( curr_expr->get_dim() == 0 ){
      const domain_node* curr_domain =
        dynamic_cast<const domain_node*>(it->first);
      assert(curr_domain);
      c_symbol_info* curr_expr_symbol = FindDefnSymbol(curr_expr,fn_bindings);

      deque<c_symbol_info*> input_exprs;
      input_exprs.push_back(curr_expr_symbol);
      deque<int> curr_unroll_factors(curr_domain->get_dim(), 1);
      print_domain_loops_header
        (curr_domain, input_exprs, curr_output_symbol->var,
         curr_output_symbol->scale_domain, curr_unroll_factors);

      output_buffer->indent();
      string rhs_string;
      if( it->second->get_type() == VEC_SCALAR )
        rhs_string =
          print_value_expr(static_cast<const expr_node*>(it->second));
      else{
        assert
          (it->second->get_type() == VEC_ID &&
           "Scalar expression is not a number or an ID");
        rhs_string = curr_expr_symbol->var->symbol_name;
      }
      print_assignment_stmt
        (curr_output_symbol,rhs_string,output_buffer->buffer);
      output_buffer->newline();
      print_domain_loops_footer(curr_domain, curr_unroll_factors);
    }
    else{
      ///The the output symbol already has a scale domain, cant carry forward
      ///the symbol, compute the expression here
      if( curr_output_symbol->scale_domain ){

        ///Precompute the RHS expression and set the scale or offset domain
        ///appropriately
        if( dynamic_cast<const domainfn_node*>(it->first))
          init_rhs
            (it->second,it->second->get_expr_domain(),output_buffer->buffer,
             fn_bindings,static_cast<const domainfn_node*>(it->first),NULL);
        else
          init_rhs
            (it->second,it->second->get_expr_domain(),output_buffer->buffer,
             fn_bindings,NULL,static_cast<const domain_node*>(it->first));

        ///The loop domain is the domain of the RHS
        domain_node* loop_domain = new domain_node();
        loop_domain->init_domain(it->second->get_expr_domain());
        if( it->second->get_sub_domain())
          loop_domain->compute_intersection(it->second->get_sub_domain());
        loop_domain->realign_domain();

        ///Now read the value from just computed expression and place it in the
        ///appropriate location on the LHS
        c_symbol_info* curr_expr_symbol = FindDefnSymbol(curr_expr,fn_bindings);

        deque<c_symbol_info*> input_exprs;
        input_exprs.push_back(curr_expr_symbol);
        deque<int> curr_unroll_factors(loop_domain->get_dim(), 1);
        print_domain_loops_header
          (loop_domain, input_exprs, curr_output_symbol->var,
           curr_output_symbol->scale_domain, curr_unroll_factors);

        output_buffer->indent();
        stringstream rhs_stream;
        print_rhs_var
          (it->second,rhs_stream,fn_bindings,it->second->get_sub_domain(),NULL);
        string rhs_string = rhs_stream.str();
        print_assignment_stmt
          (curr_output_symbol,rhs_string,output_buffer->buffer);
        output_buffer->newline();

        print_domain_loops_footer(loop_domain, curr_unroll_factors);
        delete loop_domain;
      }
      else{
        ///If there is no scale function on the output side, and the current
        ///expression is scaled, move the output variable along
        if( dynamic_cast<const domainfn_node*>(it->first) ){
          fn_bindings.AddSymbol
            (it->second, curr_output_symbol->var,
             static_cast<const domainfn_node*>(it->first),
             curr_output_symbol->offset_domain, curr_output_symbol->field_name);
          ///Compute the vector expression
          print_vector_expr(it->second,fn_bindings);
          ///Remove the current rhs from the fn symbol table
          fn_bindings.RemoveSymbol(it->second);
        }
        else{
          ///When the current function is an offset function, compute the offset
          ///to be used in the RHS
          domain_node* curr_offset =
            new domain_node(static_cast<const domain_node*>(it->first));

          /// if the output symbol already has an offset, compute a new offset
          /// domain
          if( curr_output_symbol->offset_domain ){
            assert(curr_output_symbol->offset_domain->get_dim() >=
                   curr_offset->get_dim());
            domain_node* new_offset = new domain_node();
            deque<range_coeffs>::const_reverse_iterator kt =
              curr_output_symbol->offset_domain->range_list.rbegin();
            for( deque<range_coeffs>::const_reverse_iterator jt =
                   curr_offset->range_list.rbegin() ;
                 jt != curr_offset->range_list.rend() ; jt++, kt++ ){
              if( jt->lb->is_default() ){
                new_offset->range_list.insert
                  (new_offset->range_list.begin(),
                   range_coeffs
                   (parametric_exp::copy(kt->lb),new default_expr()));
              }
              else{
                parametric_exp* curr_offset = parametric_exp::copy(jt->lb);
                if( !kt->lb->is_default() ){
                  curr_offset = curr_offset->add(parametric_exp::copy(kt->lb));
                }
                new_offset->range_list.insert
                  (new_offset->range_list.begin(),
                   range_coeffs(curr_offset,new default_expr()));
              }
            }
            for( ; kt != curr_output_symbol->offset_domain->range_list.rend() ;
                 kt++ ){
              new_offset->range_list.insert
                (new_offset->range_list.begin(),
                 range_coeffs(parametric_exp::copy(kt->lb),new default_expr()));
            }
            fn_bindings.AddSymbol
              (it->second,curr_output_symbol->var, NULL,
               new_offset, curr_output_symbol->field_name);
            print_vector_expr(it->second,fn_bindings);
            fn_bindings.RemoveSymbol(it->second);
            delete new_offset;
          }
          else{
            fn_bindings.AddSymbol
              (it->second,curr_output_symbol->var, NULL, curr_offset,
               curr_output_symbol->field_name);
            print_vector_expr(it->second,fn_bindings);
            fn_bindings.RemoveSymbol(it->second);
          }
        }
      }
    }
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_vec_id_expr
(const vec_id_expr_node* curr_expr, c_symbol_table& fn_bindings)
{
  ///When a vec_id_expr is reached, cant propagate the symbol more, need to copy
  ///the value to the output variable
  const c_symbol_info* curr_output_symbol =fn_bindings.GetSymbolInfo(curr_expr);
  const set<vector_expr_node*>& curr_defn_set = curr_expr->get_defn();
  assert(curr_defn_set.size() == 1 );
  const vector_expr_node* curr_defn = *(curr_defn_set.begin());
  c_symbol_info* curr_defn_symbol = fn_bindings.GetSymbolInfo(curr_defn);

  deque<c_symbol_info*>input_exprs;
  input_exprs.push_back(curr_defn_symbol);

  domain_node* loop_domain = new domain_node();
  domain_node* sub_domain = NULL;
  loop_domain->init_domain(curr_expr->get_expr_domain());

  if( curr_defn_symbol->offset_domain ){
    sub_domain = new domain_node(curr_defn_symbol->var->expr_domain);
    sub_domain->compute_intersection(curr_defn_symbol->offset_domain);
  }
  if( curr_expr->get_sub_domain() ){
    if( sub_domain == NULL ){
      sub_domain = new domain_node(curr_defn_symbol->var->expr_domain);
    }
    sub_domain->compute_intersection(curr_expr->get_sub_domain());
    loop_domain->compute_intersection(curr_expr->get_sub_domain());
    loop_domain->realign_domain();
  }

  deque<int> curr_unroll_factors(loop_domain->get_dim(), 1);
  print_domain_loops_header
    (loop_domain, input_exprs, curr_output_symbol->var,
     curr_output_symbol->scale_domain, curr_unroll_factors);

  output_buffer->indent();
  stringstream rhs_stream;

  if( sub_domain )
    print_domain_point(curr_defn_symbol->var,rhs_stream,sub_domain);
  else
    print_domain_point(curr_defn_symbol->var,rhs_stream);
  if( curr_expr->get_base_type().type == T_STRUCT ){
    int field_num = curr_expr->get_access_field();
    if( field_num != -1 ){
      assert(curr_expr->get_base_type().struct_info != NULL &&
             field_num <
             (int)(curr_expr->get_base_type().struct_info->fields.size()) );
      rhs_stream << "." <<
        curr_expr->get_base_type().struct_info->fields[field_num].field_name;
    }
  }
  string rhs_string = rhs_stream.str();
  print_assignment_stmt(curr_output_symbol,rhs_string,output_buffer->buffer);
  output_buffer->newline();

  print_domain_loops_footer(loop_domain, curr_unroll_factors);
  if( sub_domain )
    delete sub_domain;
  delete loop_domain;
}


///-----------------------------------------------------------------------------
void PrintC::print_vector_expr
(const vector_expr_node* curr_expr, c_symbol_table& fn_bindings)
{
  ///Redirecting based on the type of the vector expression
  assert(curr_expr != NULL );
  switch(curr_expr->get_type()){
  case VEC_ID: {
    print_vec_id_expr
      (static_cast<const vec_id_expr_node*>(curr_expr),fn_bindings);
    break;
  }
  case VEC_SCALE: {
    print_domainfn_expr
      (static_cast<const vec_domainfn_node*>(curr_expr),fn_bindings);
    break;
  }
  case VEC_FN: {
    print_fnid_expr(static_cast<const fnid_expr_node*>(curr_expr),fn_bindings);
    break;
  }
  case VEC_COMPOSE:{
    print_compose_expr
      (static_cast<const compose_expr_node*>(curr_expr),fn_bindings);
    break;
  }
  case VEC_MAKESTRUCT: {
    print_make_struct
      (static_cast<const make_struct_node*>(curr_expr),fn_bindings);
    break;
  }
  case VEC_SCALAR: {
    assert
      ((static_cast<const expr_node*>(curr_expr)->get_s_type() == S_VALUE) &&
       "[ME] : CodegenError : Currently rhs of compose expressions"
       "can either be vector expression or values\n");
    assert(0); ///TO BE COMPLETED
    break;
  }
  default:
    assert(0);
  }
}


///-----------------------------------------------------------------------------
void PrintC::print_vector_defn
(const vector_defn_node* curr_defn, c_symbol_table& fn_bindings)
{
  ///Add the vector defn to the symbol table
  c_var_info* new_var =
    new c_var_info
    (curr_defn->get_name(),curr_defn->get_expr_domain(),
     curr_defn->get_data_type());
  domain_node* sub_domain = NULL;
  if( curr_defn->get_sub_domain() )
    sub_domain = new domain_node(curr_defn->get_sub_domain());
  fn_bindings.AddSymbol(curr_defn,new_var,NULL,sub_domain,"");
  def_vars.push_back(new_var);
}


///-----------------------------------------------------------------------------
void PrintC::print_stmt
(const stmt_node* curr_stmt, c_symbol_table& fn_bindings, bool use_new_names)
{
  ///Create a new variable for holding the value of the stmt lhs
  init_rhs
    (curr_stmt->get_rhs(),curr_stmt->get_expr_domain(),output_buffer->buffer,
     fn_bindings,curr_stmt->get_scale_domain(),curr_stmt->get_sub_domain());
  domain_node* sub_domain = NULL;
  if( !fn_bindings.IsPresent(curr_stmt->get_rhs()) ){
    ///The rhs was a vec_id expressions, so just propogate the symbol used for
    ///the rhs
    const vec_id_expr_node* curr_vec_id =
      dynamic_cast<const vec_id_expr_node*>(curr_stmt->get_rhs());
    assert(curr_vec_id);
    const set<vector_expr_node*> curr_defn_set = curr_vec_id->get_defn();
    assert(curr_defn_set.size() == 1);
    const vector_expr_node* curr_defn = *(curr_defn_set.begin());
    const c_symbol_info* defn_symbol = fn_bindings.GetSymbolInfo(curr_defn);
    if( curr_defn->get_dim() != 0){
      if( defn_symbol->offset_domain ||curr_stmt->get_rhs()->get_sub_domain() ){
        sub_domain = new domain_node(curr_defn->get_expr_domain());
        if( defn_symbol->offset_domain ){
          sub_domain->compute_intersection(defn_symbol->offset_domain);
        }
        if( curr_stmt->get_rhs()->get_sub_domain() ){
          sub_domain->compute_intersection
            (curr_stmt->get_rhs()->get_sub_domain());
        }
      }
    }
    fn_bindings.AddSymbol(curr_stmt, defn_symbol->var, NULL,sub_domain,"");
  }
  else{
    const c_symbol_info* curr_rhs_symbol =
      fn_bindings.GetSymbolInfo(curr_stmt->get_rhs());
    if( curr_stmt->get_rhs()->get_sub_domain() ){
      sub_domain = new domain_node(curr_rhs_symbol->var->expr_domain);
      sub_domain->compute_intersection(curr_stmt->get_rhs()->get_sub_domain());
    }
    fn_bindings.AddSymbol(curr_stmt, curr_rhs_symbol->var,NULL,sub_domain,"");
    fn_bindings.RemoveSymbol(curr_stmt->get_rhs());
  }
}


void PrintC::print_vectorfn_body_helper
(const vectorfn_defn_node* curr_fn, c_symbol_table& fn_bindings,bool is_inlined)
{
  ///Precompute the expression for statements and the return expression
  const deque<stmt_node*> fn_body = curr_fn->get_body();
  for( deque<stmt_node*>::const_iterator it = fn_body.begin() ;
       it != fn_body.end() ; it++ ){
    print_stmt(*it,fn_bindings,is_inlined);
  }

  const vector_expr_node* return_expr = curr_fn->get_return_expr();
  print_vector_expr(return_expr,fn_bindings);
}


///-----------------------------------------------------------------------------
void PrintC::print_program_body
(const program_node* curr_program, c_symbol_table& fn_bindings,
 string& output_var)
{
  output_buffer->buffer << "#ifdef _LIKWID_";
  output_buffer->newline();
  output_buffer->indent();
  output_buffer->buffer << "likwid_markerInit();";
  output_buffer->newline();
  output_buffer->buffer << "#endif";
  output_buffer->newline();
  output_buffer->indent();

  if( generate_omp_pragmas ){
    output_buffer->buffer << "#pragma omp parallel ";
    output_buffer->newline();
    output_buffer->buffer << "{";

    output_buffer->newline();
    output_buffer->buffer << "#ifdef _LIKWID_";
    output_buffer->newline();
    output_buffer->indent();
    output_buffer->buffer << "likwid_markerThreadInit();";
    output_buffer->newline();
    output_buffer->indent();
    output_buffer->buffer << "likwid_markerStartRegion(\"Compute\");";
    output_buffer->newline();
    output_buffer->buffer << "#endif";
    output_buffer->newline();
    output_buffer->indent();


    output_buffer->newline();
  }
  if( generate_affine ){
    output_buffer->buffer << "#pragma scop";
    output_buffer->newline();
  }

  const vectorfn_defn_node* program_fn = curr_program->get_body();
  const deque<vector_defn_node*> curr_args = program_fn->get_args();
  ///Add the input variables to the symbol table
  for( deque<vector_defn_node*>::const_iterator it = curr_args.begin() ;
       it != curr_args.end() ; it++ ){
    print_vector_defn(*it,fn_bindings);
  }
  const vector_expr_node* return_expr = program_fn->get_return_expr();
  domain_node* return_expr_domain =
    new domain_node(return_expr->get_expr_domain());
  if( return_expr->get_sub_domain() )
    return_expr_domain->compute_intersection(return_expr->get_sub_domain());
  c_var_info* return_var =
    new c_var_info(output_var,return_expr_domain,return_expr->get_data_type());
  def_vars.push_back(return_var);
  fn_bindings.AddSymbol(program_fn->get_return_expr(),return_var,NULL,NULL,"");
  print_vectorfn_body_helper(curr_program->get_body(),fn_bindings,false);

  fn_bindings.RemoveSymbol(return_expr);
  delete return_expr_domain;

  if( generate_omp_pragmas ){
    output_buffer->buffer << "#ifdef _LIKWID_";
    output_buffer->newline();
    output_buffer->indent();
    output_buffer->buffer << "likwid_markerStopRegion(\"Compute\");";
    output_buffer->newline();
    output_buffer->buffer << "#endif";
    output_buffer->newline();
    output_buffer->indent();

    output_buffer->indent();
    output_buffer->buffer <<"}";
    output_buffer->newline();
  }
  if( generate_affine ){
    output_buffer->buffer << "#pragma endscop";
    output_buffer->newline();
  }
  output_buffer->buffer << "#ifdef _LIKWID_";
  output_buffer->newline();
  output_buffer->indent();
  output_buffer->buffer << "likwid_markerClose();";
  output_buffer->newline();
  output_buffer->buffer << "#endif";
  output_buffer->newline();
  output_buffer->indent();

}


///-----------------------------------------------------------------------------
void PrintC::GenerateCode
(const program_node* curr_program, const program_options& command_opts)
{
  if( command_opts.generate_affine ){
    generate_affine = true;
    generate_omp_pragmas = false;
  }
  if( !command_opts.use_openmp )
    generate_omp_pragmas = false;
  if( command_opts.use_icc_pragmas )
    generate_icc_pragmas = true;
  init_zero = command_opts.init_zero;
  if (command_opts.generate_unroll_code) {
    generate_unroll_code = true;
    unroll_factors.insert
      (unroll_factors.begin(), command_opts.unroll_factors.begin(),
       command_opts.unroll_factors.end());
  }

  output_buffer->increaseIndent();
  deallocate_buffer->increaseIndent();
  host_allocate->increaseIndent();

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
  c_symbol_table fn_bindings;

  string output_var = get_new_var();

  output_buffer->buffer << "#ifdef _TIMER_" ;
  output_buffer->newline();
  output_buffer->indent();
  output_buffer->buffer << "double _forma_timer_start_,_forma_timer_stop_;";
  output_buffer->newline();
  output_buffer->indent();
  output_buffer->buffer << "_forma_timer_start_ = rtclock();";
  output_buffer->newline();
  output_buffer->buffer << "#endif";
  output_buffer->newline();

  print_program_body(curr_program,fn_bindings,output_var);

  output_buffer->buffer << "#ifdef _TIMER_" ;
  output_buffer->newline();
  output_buffer->indent();
  output_buffer->buffer << "_forma_timer_stop_ = rtclock();";
  output_buffer->newline();
  output_buffer->indent();
  output_buffer->buffer <<
    "printf(\"[FORMA] Computation Time(ms) : %lf\\n\""
    ",(_forma_timer_stop_ -_forma_timer_start_ )*1000.0);";
  output_buffer->newline();
  output_buffer->buffer << "#endif";
  output_buffer->newline();

  output_buffer->decreaseIndent();

  if( use_single_malloc ){
    host_allocate_size->indent();
    host_allocate_size->buffer << "char* " << globalMemVar << ";";
    host_allocate_size->newline();

    host_allocate_size->indent();
    host_allocate_size->buffer << "if( " << globalMemOffset << " > 0 ){";
    host_allocate_size->newline();
    host_allocate_size->increaseIndent();

    host_allocate_size->indent();
    host_allocate_size->buffer << globalMemVar << " = (char*)malloc( " <<
      globalMemOffset << ");";
    host_allocate_size->newline();

    if( init_zero ){
      host_allocate_size->indent();
      host_allocate_size->buffer << "memset(" << globalMemVar << ",0," <<
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
    deallocate_buffer->buffer << "free(" << globalMemVar << ");";
    deallocate_buffer->newline();
    deallocate_buffer->decreaseIndent();
  }

  output_buffer->buffer << deallocate_buffer->buffer.str();
  deallocate_buffer->decreaseIndent();
  if( use_single_malloc )
    host_allocate_size->decreaseIndent();
  host_allocate->decreaseIndent();
  output_buffer->decreaseIndent();

  print_string(curr_program,command_opts,output_var);
}


///-----------------------------------------------------------------------------
void PrintC::print_string
(const program_node* curr_program, const program_options& command_opts,
 string& output_var)
{
  FILE* CodeGenFile = fopen(command_opts.c_output_file.c_str(),"w");

  print_header_info(CodeGenFile);
  const vectorfn_defn_node* program_fn = curr_program->get_body();
  const deque<vector_defn_node*> curr_args = program_fn->get_args();
  stringBuffer temp_buffer;
  PrintCParametricDefines(temp_buffer);
  PrintCStructDefinition(temp_buffer);
  fprintf(CodeGenFile,"%s",temp_buffer.buffer.str().c_str());
  fprintf(CodeGenFile,"void %s(",command_opts.kernel_name.c_str());

  for( deque<vector_defn_node*>::const_iterator it = curr_args.begin() ;
       it != curr_args.end() ; it++ ){
    fprintf(CodeGenFile,"%s ",get_string((*it)->get_data_type()).c_str());
    if( generate_affine && (*it)->get_dim() > 1 &&
        (*it)->get_dim() <= supported_affine_dim ){
      stringstream temp_stream;
      print_pointer((*it)->get_dim(),temp_stream);
      fprintf(CodeGenFile,"%s",temp_stream.str().c_str());
    }
    else{
#ifndef _WINDOWS_
      fprintf(CodeGenFile,"%s",((*it)->get_dim() == 0 ? "" : "* restrict "));
#else
      fprintf(CodeGenFile,"%s",((*it)->get_dim() == 0 ? "" : "* "));
#endif
    }
    fprintf(CodeGenFile,"%s, ",(*it)->get_name().c_str());
  }
  ///Add parameters as arguments
  for( deque<pair<string,parameter_defn*> >::const_iterator it =
         global_params->begin() ; it != global_params->end() ; it++ ){
    fprintf(CodeGenFile,"int %s, ",it->first.c_str());
  }

  fprintf(CodeGenFile,"%s",
          get_string(program_fn->get_return_expr()->get_data_type()).c_str());
  if( generate_affine && program_fn->get_return_expr()->get_dim() > 1 &&
      program_fn->get_return_expr()->get_dim() <= supported_affine_dim){
    stringstream temp_stream;
    print_pointer(program_fn->get_return_expr()->get_dim(),temp_stream);
    fprintf(CodeGenFile,"%s",temp_stream.str().c_str());
  }
  else{
#ifndef _WINDOWS_
    fprintf(CodeGenFile,"* restrict ");
#else
    fprintf(CodeGenFile,"* ");
#endif
  }
  fprintf(CodeGenFile," %s){\n",output_var.c_str());
  if( use_single_malloc ){
    fprintf(CodeGenFile,"%s",host_allocate_size->buffer.str().c_str());
  }
  fprintf(CodeGenFile,"%s",host_allocate->buffer.str().c_str());
  fprintf(CodeGenFile,"%s",output_buffer->buffer.str().c_str());
  fprintf(CodeGenFile,"}\n");
  fflush(CodeGenFile);
  fclose(CodeGenFile);
}


///-----------------------------------------------------------------------------
void PrintC::print_header_info(FILE * outfile)
{
  fprintf
    (outfile,
     "#include \"stdio.h\"\n"
     "#include \"stdlib.h\"\n"
     "#include \"math.h\"\n"
     "#include \"omp.h\"\n"
     "#include \"string.h\"\n\n"
     "#ifdef _LIKWID_\n"
     "#include \"likwid.h\"\n"
     "#endif // _LIKWID_\n\n"
     "#ifdef _TIMER_\ndouble rtclock();\n#endif\n"
     "void initialize_double_array(double * array, int size, double val);\n"
     "void initialize_int_array(int * array, int size, int val);\n"
     "void initialize_int8_array"
     "(unsigned char * array, int size, unsigned char val);\n"
     "void initialize_short_array"
     "(short * array, int size, short val);\n"
     "void initialize_float_array(float * array, int size, float val);\n\n");
}
