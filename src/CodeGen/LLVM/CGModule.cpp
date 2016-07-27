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

#ifdef FORMA_USE_LLVM

#include "CodeGen/LLVM/CGModule.h"
#include "CodeGen/LLVM/CGFunction.h"
#include "CodeGen/LLVM/CGStencilFn.h"
#include "AST/parser.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/Verifier.h"
#include "ASTVisitor/visitor.hpp"

#include <memory>

using namespace llvm;


// class LLVMVisitor : public ASTVisitor<int> {
//   virtual void visit_vectorfn(vectorfn_defn_node *node, int state) {
//     //outs() << "visit_vectorfn\n";
//   }
// };



CGModule::CGModule() :
  stencil_fns(8)
{}

void CGModule::GenerateCode(const program_node* program, const program_options& command_opts) {
  PrettyPrinter::instance(80);

  // LLVMVisitor V;
  // V.visit(program, 0);

  init_zero = command_opts.init_zero;
  
  // Create the module
  mod.reset(new llvm::Module("FORMA", ctx));

  //Generate user defined types
  if( defined_types ){
    for( auto defStruct = defined_types->begin() ; defStruct != defined_types->end() ; defStruct++ ){
      defined_data_type* currType = defStruct->second;
      SmallVector<Type*,4> fields;
      for( auto defFields : currType->fields ){
	fields.push_back(getTypeForFormaBasicType(defFields.field_type));
      }
      StringRef currTypeName(currType->name);
      StructType::create(fields,currTypeName);
    }
  }

  CGVectorFn::builder = new IRBuilder<>(ctx);

  // Generate the top-level function
  CGVectorFn topFn(*this, program->get_body(), true, init_zero);
  topFn.generateDeclaration();
  CGVectorFn::builder->CreateRetVoid();

  delete CGVectorFn::builder;

  // Create the user-callable wrapper function
  generateWrapper(command_opts.kernel_name,program->get_body());
  
  ///Now generate all the stencil function bodies.
  for( auto stencilFnIter = stencil_fns.begin() ; stencilFnIter != stencil_fns.end() ; stencilFnIter++ ){
    stencilFnIter->second->generate();
  }

  std::error_code errorCode;
  std::unique_ptr<tool_output_file> Out;
  Out.reset(new tool_output_file(command_opts.llvm_file_name.c_str(), errorCode,
                                 sys::fs::F_None));
  if (errorCode.value() != 0 ) {
    errs() << errorCode.message() << "\n";
    return;
  }


  // // The module has now been generated.  Run some simple optimizations
  // // to clean up our codegen gunk.
  legacy::PassManager PM;
  PM.add(createVerifierPass(true));
  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(createInstructionCombiningPass());
  PM.add(createFunctionInliningPass());
  PM.add(createLoopSimplifyPass());
  PM.add(createLICMPass());
  PM.add(createCFGSimplificationPass());
  PM.add(createSinkingPass());
  PM.add(createIndVarSimplifyPass());
  PM.add(createScalarReplAggregatesPass());
  PM.add(createSROAPass());
  PM.add(createEarlyCSEPass());
  PM.add(createGVNPass());
  // PM.add(createGlobalMergePass());
  PM.add(createLoopStrengthReducePass());
  PM.add(createInstructionCombiningPass());
  PM.add(createCFGSimplificationPass());
  PM.add(createSinkingPass());
  PM.add(createScalarReplAggregatesPass());
  PM.add(createSROAPass());
  PM.add(createEarlyCSEPass());
  PM.add(createGVNPass());
  PM.add(createDeadStoreEliminationPass());
  PM.add(createDeadInstEliminationPass());
  PM.add(createAggressiveDCEPass());
  PM.add(createPrintModulePass(Out->os()));
  PM.run(*mod);

  Out->keep();
}



void CGModule::generateWrapper(const std::string& kernel_name, const vectorfn_defn_node* defn) {

  // We are not using generateDeclaration for the wrapper function since the
  // function prototype corresponds to the Forma C ABI instead of the internal
  // vector function ABI.

  StringMap<unsigned> paramIndices;
  StringMap<Value*> paramMap;
  SmallVector<ArgDesc*,8> arguments;

  Type *i32Ty = Type::getInt32Ty(ctx);
  // unsigned numDims = defn->get_return_dim();

  // Generate the function argument types
  for (auto it = defn->get_args().begin(); it != defn->get_args().end(); ++it) {
    vector_defn_node *arg = *it;
    const data_types &type = arg->get_data_type();
    if (arg->get_dim() != 0) {
      // This is a vector with at least one dimension, emit a pointer
      Type *ptrTy = PointerType::get(getTypeForFormaType(type), 0);
      arguments.push_back(new ArgDesc(ptrTy, arg->get_dim(), arg->get_name()));
    } else {
      // This is a scalar, emit a value type
      Type *baseType = getTypeForFormaType(type);
      arguments.push_back(new ArgDesc(baseType, 0, arg->get_name()));
    }
  }

  // Also pass the parameters
  for (auto it = global_params->begin(); it != global_params->end(); ++it) {
    const std::string &name = it->first;
    // parameter_defn *paramDef = it->second;
    arguments.push_back(new ArgDesc(i32Ty, 0, name));
    paramIndices[name] = arguments.size()-1;
  }

  // Add return vector pointer
  const data_types &type = defn->get_data_type();
  Type *baseType = getTypeForFormaType(type);
  // This is a vector with at least one dimension, emit a pointer type
  arguments.push_back(new ArgDesc(PointerType::get(baseType, 0), 0, "return"));


  // Determine the return type
  Type *retType = Type::getVoidTy(getLLVMContext());

  SmallVector<Type *, 8> argTypes;

  // Generate the declaration
  for (ArgDesc * arg : arguments) {
    argTypes.push_back(arg->type);
  }

  IRBuilder<> builder(getLLVMContext());
  FunctionType *funcTy = FunctionType::get(retType, argTypes, false);
  Constant *fnPtr = getLLVMModule()->getOrInsertFunction(kernel_name.c_str(), funcTy);
  Function* thisFn = cast<Function>(fnPtr);

  // Fill in the argument names/values
  auto argIt = arguments.begin();
  for (Argument& arg : thisFn->getArgumentList()) {
    ArgDesc * desc = *(argIt++);
    desc->val = &arg;
    arg.setName(desc->name);
    // Reset the argument name cache, as the argument name could have changed
    // due to uniquing
    desc->name = arg.getName();
  }

  // Populate parameter map
  for (auto it = paramIndices.begin(); it != paramIndices.end(); ++it) {
    paramMap[it->first()] = arguments[it->second]->val;
  }

  BasicBlock *bb = BasicBlock::Create(getLLVMContext(), "entry", thisFn);
  Function *entry = getLLVMModule()->getFunction("__entry__");
  assert(entry && "No __entry__ function?");

  builder.SetInsertPoint(bb);

  SmallVector<Value *, 4> callArgs;
  
  // Collect inputs
  for (unsigned i = 0; i != defn->get_args().size(); ++i) {
    vector_defn_node *arg = defn->get_args()[i];
    const data_types &type = arg->get_data_type();
    if (arg->get_dim() != 0) {
      const domain_node *domain = arg->get_expr_domain();
      assert(domain && "No domain?");
      Type *structTy = getVectorStructType(type, arg->get_dim());
      Value *str = UndefValue::get(structTy);
      Value *ptr = getArgumentByIndex(thisFn,i);

      str = builder.CreateInsertValue(str, ptr, {0});
      for (unsigned j = 0; j != (unsigned)(arg->get_dim()); ++j) {
        parametric_exp *start = domain->get_domain()[j].lb;
        Value *startVal = generateParametricExpr(start,builder,paramMap);
        parametric_exp *stop = domain->get_domain()[j].ub;
        Value *stopVal = generateParametricExpr(stop,builder,paramMap);
        Value *sub = builder.CreateSub(stopVal, startVal);
        Value *add = builder.CreateAdd(sub, builder.getInt32(1));
        str = builder.CreateInsertValue(str, add, {j+1});
      }
      callArgs.push_back(str);
    } else {
      // Scalar
      Value *ptr = getArgumentByIndex(thisFn,i);
      callArgs.push_back(ptr);
    }
  }

  // Collect parameters
  for (auto it = global_params->begin(); it != global_params->end(); ++it) {
    const std::string &name = it->first;
    assert(paramMap.find(name) != paramMap.end() && "Parameter does not exist");
    callArgs.push_back(paramMap[name]);
  }
  
  // Add the return arg
  const vector_expr_node* return_expr = static_cast<const vectorfn_defn_node*>(defn)->get_return_expr();
  const domain_node* output_domain = static_cast<const vectorfn_defn_node*>(defn)->get_expr_domain();
  Type* returnArgType = getVectorStructType(return_expr->get_data_type(), return_expr->get_dim());
  Value* returnVar = UndefValue::get(returnArgType);
  returnVar = builder.CreateInsertValue(returnVar,arguments.back()->val,{0});
  for( unsigned j = 0 ; j < (unsigned)(return_expr->get_dim()) ; j++ ){
    parametric_exp* start = output_domain->get_domain()[j].lb;
    Value* startVal = generateParametricExpr(start,builder,paramMap);
    parametric_exp* stop = output_domain->get_domain()[j].ub;
    Value* stopVal = generateParametricExpr(stop,builder,paramMap);
    Value* sub = builder.CreateSub(stopVal,startVal);
    Value* add = builder.CreateAdd(sub,builder.getInt32(1));
    returnVar = builder.CreateInsertValue(returnVar,add,{j+1});
  }
  callArgs.push_back(returnVar);
  builder.CreateCall(entry, callArgs);
  builder.CreateRetVoid();
}


std::string CGModule::getBdyInfoFnName(const stencilfn_defn_node* fn, 
				const std::deque<bdy_info*> curr_bdy_info)
{
  std::stringstream fnName;
  fnName << fn->get_name() << "_stencil_" ;
  int argNum =0;
  for( auto bdyInfoIter : curr_bdy_info ){
    switch(bdyInfoIter->type){
    case B_CONSTANT:
      fnName << "_" << argNum << "CO" ;
      break;
    case B_CLAMPED:
    case B_EXTEND:
      fnName << "_" << argNum << "CL" ;
      break;
    case B_WRAP:
      fnName << "_" << argNum << "WR" ;
      break;
    case B_MIRROR :
      fnName << "_" << argNum << "MI" ;
      break;
    case B_NONE:
    default:
      ;
    }
    argNum++;
  }
  return fnName.str();
}


void CGModule::addStencilFunction(StringRef& fnName, const stencilfn_defn_node* fn,
			     const std::deque<bdy_info*> curr_bdy_info)
{
  auto fnFindIter = stencil_fns.find(fnName);
  if( fnFindIter == stencil_fns.end() ){
    CGStencilFn* newStencilFn = new CGStencilFn(*this,fn,fnName,curr_bdy_info);
    newStencilFn->generateDeclaration();
    stencil_fns[fnName] = newStencilFn;
  }
}


void CGModule::addStencilFunction(StringRef& fnName, const stencilfn_defn_node* fn)
{
  auto fnParams = fn->get_args();
  std::deque<bdy_info*> dummy_bdy_condns;
  for( int i = 0 ; i < (int)fnParams.size() ; i++ ){
    bdy_info* new_bdy_cond = new bdy_info(B_NONE);
    dummy_bdy_condns.push_back(new_bdy_cond);
  }
  auto fnFindIter = stencil_fns.find(fnName);
  if( fnFindIter == stencil_fns.end() ){
    CGStencilFn* newStencilFn = new CGStencilFn(*this,fn,fnName,dummy_bdy_condns);
    newStencilFn->generateDeclaration();
    stencil_fns[fnName] = newStencilFn;
  }
}


CGVectorFn* CGModule::addVectorFunction(StringRef& fnName, const vectorfn_defn_node* fn)
{
  auto fnFindIter = vector_fns.find(fnName);
  if( fnFindIter == vector_fns.end() ){
    CGVectorFn* newVectorFn = new CGVectorFn(*this,fn,false,init_zero);
    //newVectorFn->generateDeclaration();
    vector_fns[fnName] = newVectorFn;
    return newVectorFn;
  }
  else{
    return fnFindIter->second;
  }
}


Type *CGModule::getTypeForFormaType(const data_types &type) {
  if( type.type == T_STRUCT ){
    StructType* varType = mod->getTypeByName(type.struct_info->name);
    return varType;
  }
  else
    return getTypeForFormaBasicType(type.type);
}


Type *CGModule::getTypeForFormaBasicType(basic_data_types type) {
  switch (type) {
  default:
    assert(0 && "Unhandled type");
    return NULL;
  case T_FLOAT:
    return llvm::Type::getFloatTy(ctx);
  case T_DOUBLE:
    return llvm::Type::getDoubleTy(ctx);
  case T_INT:
    return llvm::Type::getInt32Ty(ctx);
  case T_INT8:
    return llvm::Type::getInt8Ty(ctx);
  case T_INT16:
    return llvm::Type::getInt16Ty(ctx);
  // XXX: Missing structs
  }
}

int CGModule::getSizeOfType(const data_types& type)
{
  switch(type.type ){
  case T_STRUCT: {
    int data_size = 0;
    for( auto fieldType : type.struct_info->fields ){
      Type* currType = getTypeForFormaBasicType(fieldType.field_type);
      data_size += currType->getPrimitiveSizeInBits()/8;
    }
    return data_size;
  }
  case T_FLOAT:
  case T_DOUBLE:
  case T_INT:
  case T_INT8:
  case T_INT16:{
    Type* currType = getTypeForFormaBasicType(type.type);
    return currType->getPrimitiveSizeInBits()/8;
  }
  default:
    assert(0 && "Unhandled Type");
    return 0;
  }
}

Type *CGModule::getVectorStructType(const data_types &type,
                                      unsigned numDims) {
  Type *baseType = getTypeForFormaType(type);

  SmallVector<Type *, 4> comps;
  comps.push_back(PointerType::get(baseType, /* AS */ 0));

  for (unsigned i = 0; i != numDims; ++i){
    comps.push_back(Type::getInt32Ty(getLLVMContext()));
  }

  return StructType::get(getLLVMContext(), comps, false);
}

Type *CGModule::getInfoStructType(unsigned ndims)
{
  SmallVector<Type*,4> types;
  SmallVector<Type *, 4> comps;
  for( unsigned i = 0 ; i != ndims ; i++ )
    comps.push_back(Type::getInt32Ty(getLLVMContext()));
  return StructType::get(getLLVMContext(),comps,false);
}

Value *CGModule::getArgumentByIndex(Function* thisFn, unsigned index) {
  assert(thisFn && "Function is NULL");
  assert(index < thisFn->arg_size() && "Index out of range");
  Function::arg_iterator it = thisFn->arg_begin();
  for (unsigned i = 0; i != index; ++i)
    ++it;
  return &(*it);
}

const Value *CGModule::getArgumentByIndex(Function* thisFn, unsigned index) const {
  assert(thisFn && "Function is NULL");
  assert(index < thisFn->arg_size() && "Index out of range");
  Function::const_arg_iterator it = thisFn->arg_begin();
  for (unsigned i = 0; i != index; ++i)
    ++it;
  return &(*it);
}


Value *CGModule::generateParametricExpr(const parametric_exp *pexp, IRBuilder<>& builder,
					StringMap<Value*>& paramMap) {
  if (const int_expr *expr = dynamic_cast<const int_expr *>(pexp)) {
    // Constant value
    return builder.getInt32(expr->value);
  } else if (const binary_expr *expr =
               dynamic_cast<const binary_expr *>(pexp)) {
    Value *lhs = generateParametricExpr(expr->lhs_expr,builder,paramMap);
    Value *rhs = generateParametricExpr(expr->rhs_expr,builder,paramMap);
    switch (expr->type) {
    case P_ADD:
      return builder.CreateAdd(lhs, rhs);
    case P_SUBTRACT:
      return builder.CreateSub(lhs, rhs);
    case P_MULTIPLY:
      return builder.CreateMul(lhs, rhs);
    case P_DIVIDE:
      return builder.CreateUDiv(lhs, rhs);
    case P_MIN: {
      Value* checkVal = builder.CreateICmpSLT(lhs,rhs);
      return builder.CreateSelect(checkVal,lhs,rhs);
    }
      break;
    default:
      assert(0 && "Unhandled binary op");
      return NULL;
    }
  } else if (const param_expr *expr =
               dynamic_cast<const param_expr *>(pexp)) {
    assert(paramMap.find(expr->param->param_id) != paramMap.end() &&
           "Accessing a non-existant parameter");
    return paramMap[expr->param->param_id];
  } else {
    assert(0 && "Unhandled bound");
    return NULL;
  }
}


#endif
