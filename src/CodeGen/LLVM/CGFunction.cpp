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

#include "CodeGen/LLVM/CGFunction.h"
#include "CodeGen/LLVM/CGModule.h"
#include "AST/parser.hpp"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

IRBuilder<>* CGVectorFn::builder = NULL;

CGVectorFn::CGVectorFn(CGModule &CGM, const vectorfn_defn_node *fn, bool topLevel, bool set_zero)
  : CGFunction(CGM),  init_zero(set_zero),  defn(fn), isTopLevel(topLevel) { }

// void CGVectorFn::generate(ArgDesc* outputVar, const domain_desc_node* outputIdxInfo) {
//   assert(thisFn && "Function not declared?");

//   // Create the entry block
//   BasicBlock *bb = BasicBlock::Create(cgm.getLLVMContext(), "entry", thisFn);
//   builder->SetInsertPoint(bb);

//   generateVectorFunction(defn,outputVar,outputIdxInfo);
// }


ArgDesc* CGVectorFn::allocateDomain(const domain_node* domain, const data_types& type, const std::string& name)
{
  //const domain_node* domain = expr->get_expr_domain();
  unsigned numDims = domain->get_dim();
  Value* outSize = builder->getInt32(1);
  SmallVector<Value*,4> dimSizes;
  for( unsigned i = 0 ; i != numDims ; i++ ){
    parametric_exp* start = domain->get_domain()[i].lb;
    Value* startVal = generateParametricExpr(start);
    parametric_exp* stop = domain->get_domain()[i].ub;
    Value* stopVal = generateParametricExpr(stop);
    Value* sub = builder->CreateSub(stopVal,startVal);
    Value* add = builder->CreateAdd(sub,builder->getInt32(1));
    outSize = builder->CreateMul(outSize,add);
    dimSizes.push_back(add);
  }  

  int bytesPerElem = cgm.getSizeOfType(type);
  //printf("Type Size in Alloc : %d\n",bytesPerElem);
  outSize = builder->CreateMul(outSize, builder->getInt32(bytesPerElem));

  SmallVector<Value*,2> formartAllocArgs;
  formartAllocArgs.push_back(outSize);
  formartAllocArgs.push_back(builder->getInt32(init_zero ? 1 : 0 ));
  
  SmallVector<Type*,2> allocFnArgTy ;
  allocFnArgTy.push_back(Type::getInt32Ty(cgm.getLLVMContext()));
  allocFnArgTy.push_back(Type::getInt32Ty(cgm.getLLVMContext()));
  FunctionType *allocFnTy =
    FunctionType::get(builder->getInt8PtrTy(), allocFnArgTy, false);
  Constant *allocFn =
    cgm.getLLVMModule()->getOrInsertFunction("__formart_alloc", allocFnTy);
  Value *outBaseAddr = builder->CreateCall(allocFn, formartAllocArgs);

  Type *valType = cgm.getTypeForFormaType(type);
  valType = PointerType::get(valType, /* AS */ 0);
  outBaseAddr = builder->CreateBitCast(outBaseAddr, valType);

  Type* outType = cgm.getVectorStructType(type,numDims);
  Value* outVal = UndefValue::get(outType);
  outVal = builder->CreateInsertValue(outVal,outBaseAddr,0);
  for( unsigned i = 0 ; i != numDims ; i++ ){
    outVal =  builder->CreateInsertValue(outVal,dimSizes[i],{i+1});
  }
  ArgDesc* outputArg = new ArgDesc(outType,numDims,name);
  outputArg->val = outVal;

  locals[name] = outputArg;
  return outputArg;
}				



void CGVectorFn::generateVectorFunction(const vectorfn_defn_node *fnDef, ArgDesc* outputVar,
					const domain_desc_node* outputIdxInfo) {

  // Generate each statement node
  for (auto it = fnDef->get_body().begin();
       it != fnDef->get_body().end(); ++it) {
    stmt_node *stmt = *it;
    assert(stmt->get_expr_domain());
    ArgDesc* resultDomain = allocateDomain(stmt->get_expr_domain(),stmt->get_data_type(),stmt->get_name());
    const domain_desc_node* outputIdxInfo = ( stmt->get_scale_domain() ? 
					      static_cast<const domain_desc_node*>(stmt->get_scale_domain()) :
					      static_cast<const domain_desc_node*>(stmt->get_sub_domain()));
    generateVectorExpr(stmt->get_rhs(),resultDomain,outputIdxInfo);
  }

  //Now the return stmt
  const vector_expr_node *retExpr = fnDef->get_return_expr();
  if (retExpr) {
    generateVectorExpr(retExpr,outputVar,outputIdxInfo);
  }
  
  ///Delete the buffers created for the local variables
  FunctionType* deallocFnType = FunctionType::get(builder->getVoidTy(),{builder->getInt8PtrTy()},false);
  Constant* deallocFn = cgm.getLLVMModule()->getOrInsertFunction("__formart_dealloc",deallocFnType);
  for( auto it = locals.begin() ; it != locals.end() ; it++ ){
    Value* baseAddr = builder->CreateExtractValue(it->second->val,{0});
    baseAddr = builder->CreateBitCast(baseAddr,builder->getInt8PtrTy());
    builder->CreateCall(deallocFn,baseAddr);
    delete it->second;
  }
  locals.clear();
}


ArgDesc *CGFunction::getArgumentByName(StringRef name) {
  for (ArgDesc *desc : arguments) {
    if (name.compare(desc->name.c_str()) == 0)
      return desc;
  }
  return NULL;
}

const ArgDesc *CGFunction::getArgumentByName(StringRef name) const {
  for (const ArgDesc *desc : arguments) {
    if (name.compare(desc->name.c_str()) == 0)
      return desc;
  }
  return NULL;
}

void CGVectorFn::InitParams(const StringMap<Value*>& paramList) {
  if( paramMap.size() == 0 ){
    for( auto currParam = paramList.begin() ; currParam != paramList.end() ; currParam++ )
      paramMap[currParam->first()] = currParam->second;
  }
  assert(paramList.size() == paramMap.size());
}




Function* CGVectorFn::generateDeclaration() {
  StringMap<unsigned> paramIndices;

  assert(arguments.size() == 0 && "Declaration already generated?");

  // Determine what the function name should be
  std::string name;
  assert(isTopLevel && "Currently all vector functions are inline, this function called only by the maind code-generation driver");

  if (isTopLevel)
    name = "__entry__";
  else
    name = defn->get_name();

  Type *i32Ty = Type::getInt32Ty(cgm.getLLVMContext());
  // unsigned numDims = defn->get_return_dim();

  // Generate the function argument types
  for (auto it = defn->get_args().begin(); it != defn->get_args().end(); ++it) {
    vector_defn_node *arg = *it;
    const data_types &type = arg->get_data_type();
    if (arg->get_dim() != 0) {
      // This is a vector with at least one dimension, emit a vector struct
      Type *structTy = cgm.getVectorStructType(type, arg->get_dim());
      arguments.push_back(new ArgDesc(structTy, arg->get_dim(), arg->get_name()));
    } else {
      // This is a scalar, emit a value type
      Type *baseType = cgm.getTypeForFormaType(type);
      arguments.push_back(new ArgDesc(baseType, 0, arg->get_name()));
    }
  }

  // Also pass the parameters
  for (auto it = global_params->begin(); it != global_params->end(); ++it) {
    const std::string &name = it->first;
    // parameter_defn *paramDef = it->second;
    arguments.push_back(new ArgDesc(i32Ty,0, name));
    paramIndices[name] = arguments.size()-1;
  }

  // Determine the return type
  Type *retType;
  const vector_expr_node* return_expr = static_cast<const vectorfn_defn_node*>(defn)->get_return_expr();
  Type* returnArgType = cgm.getVectorStructType(return_expr->get_data_type(),return_expr->get_dim());
  arguments.push_back(new ArgDesc(returnArgType,0,"return"));
 // The function return type will be void
  retType = Type::getVoidTy(cgm.getLLVMContext());
 
  SmallVector<Type *, 8> argTypes;

  // Generate the declaration
  for (ArgDesc *arg : arguments) {
    argTypes.push_back(arg->type);
  }

  FunctionType *funcTy = FunctionType::get(retType, argTypes, false);
  Constant *fnPtr = cgm.getLLVMModule()->getOrInsertFunction(name, funcTy);
  thisFn = cast<Function>(fnPtr);

  thisFn->setLinkage(GlobalValue::InternalLinkage);

  // Fill in the argument names/values
  auto argIt = arguments.begin();
  for (Argument &arg : thisFn->getArgumentList()) {
    ArgDesc *desc = *(argIt++);
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

  // generate(arguments.back());

  // Create the entry block
  BasicBlock *bb = BasicBlock::Create(cgm.getLLVMContext(), "entry", thisFn);
  builder->SetInsertPoint(bb);

  generateVectorFunction(defn,arguments.back());

  for( auto args: arguments )
    delete args;
  
  return thisFn;
}

#endif
