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
#include "CodeGen/LLVM/CGFunction.h"
#include "CodeGen/LLVM/CGModule.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdio>

using namespace llvm;

ArgDesc *CGVectorFn::generateConstant(const expr_node *expr) {
  Type* argType = NULL;
  Value* constVal = NULL;
  switch (expr->get_data_type().type) {
  case T_FLOAT: {
    float val = static_cast<const value_node<float>*>(expr)->get_value();
    argType = Type::getFloatTy(cgm.getLLVMContext());
    constVal = ConstantFP::get(Type::getFloatTy(cgm.getLLVMContext()), val);
  }
    break;
  case T_DOUBLE: {
    double val = static_cast<const value_node<double>*>(expr)->get_value();
    argType = Type::getDoubleTy(cgm.getLLVMContext());
    constVal = ConstantFP::get(Type::getDoubleTy(cgm.getLLVMContext()), val);
  }
    break;
  case T_INT: {
    int val = static_cast<const value_node<int>*>(expr)->get_value();
    argType = Type::getInt32Ty(cgm.getLLVMContext());
    constVal = ConstantInt::get(Type::getInt32Ty(cgm.getLLVMContext()), val);
  }
  case T_INT16: {
    int val = static_cast<const value_node<short>*>(expr)->get_value();
    argType = Type::getInt32Ty(cgm.getLLVMContext());
    constVal = ConstantInt::get(Type::getInt32Ty(cgm.getLLVMContext()), val);
  }
  case T_INT8: {
    int val = static_cast<const value_node<unsigned char>*>(expr)->get_value();
    argType = Type::getInt32Ty(cgm.getLLVMContext());
    constVal = ConstantInt::get(Type::getInt32Ty(cgm.getLLVMContext()), val);
  }
    break;
  default:
    assert(0 && "Unhandled constant type");
  }
  ArgDesc* retConst = new ArgDesc(argType,0,getTemporaryName());
  retConst->val = constVal;
  return retConst;
}


ArgDesc* CGVectorFn::generateVectorExpr(const vector_expr_node* expr){
  ArgDesc* resultDomain = NULL;
  switch (expr->get_type()){
  case VEC_FN: {
    ///Allocate space to store result of this expr
    resultDomain = allocateDomain(expr->get_expr_domain(),expr->get_base_type(),getTemporaryName());
    const fnid_expr_node* fn_expr = static_cast<const fnid_expr_node*>(expr);
    const fn_defn_node* fn_defn = fn_expr->get_defn();
    if( dynamic_cast<const stencilfn_defn_node*>(fn_defn) == NULL ){
      generateVectorFnExpr(fn_expr,resultDomain,NULL);
    }
    else{
      generateStencilFnExpr(fn_expr,resultDomain,NULL);
    }
  }
    break;
  case VEC_COMPOSE: 
    ///Allocate space to store result of this expr
    resultDomain = allocateDomain(expr->get_expr_domain(),expr->get_base_type(),getTemporaryName());
    generateComposeExpr(static_cast<const compose_expr_node*>(expr),resultDomain);
    break;
  case VEC_MAKESTRUCT:
    ///Allocate space to store result of this expr
    resultDomain = allocateDomain(expr->get_expr_domain(),expr->get_base_type(),getTemporaryName());
    generateMakeStruct(static_cast<const make_struct_node*>(expr),resultDomain,NULL);
    break;
  case VEC_SCALE:
    ///Allocate space to store the result
    resultDomain = allocateDomain(expr->get_expr_domain(),expr->get_base_type(),getTemporaryName());
    generateVecScaleExpr(static_cast<const vec_domainfn_node*>(expr),resultDomain,NULL);
    break;
  case VEC_ID: 
    resultDomain = generateVectorIdExpr(static_cast<const vec_id_expr_node*>(expr));
    break;
  case VEC_SCALAR:
    resultDomain = generateConstant(static_cast<const expr_node*>(expr));
    break;
  default:
    assert(0 && "Unhandled expression type in LLVM Codegen");
  }
  return resultDomain;
}


void CGVectorFn::generateVectorExpr(const vector_expr_node *expr, ArgDesc* outputArg,
				    const domain_desc_node* outputIdxInfo) {
  switch (expr->get_type()) {
  case VEC_FN: {
    const fnid_expr_node* fn_expr = static_cast<const fnid_expr_node*>(expr);
    const fn_defn_node* fn_defn = fn_expr->get_defn();
    if( dynamic_cast<const stencilfn_defn_node*>(fn_defn) == NULL ){
      generateVectorFnExpr(fn_expr,outputArg,outputIdxInfo);
    }
    else{
      generateStencilFnExpr(fn_expr,outputArg,outputIdxInfo);
    }
  }
    break;
  case VEC_COMPOSE:
    if( outputIdxInfo == NULL ){
      generateComposeExpr(static_cast<const compose_expr_node*>(expr),outputArg);
    }
    else{
      ///Generate a new variable (since dont know how to compose this
      ///domain with domain of individual components of a compse expr
      ArgDesc* newVar = allocateDomain(expr->get_expr_domain(),expr->get_base_type(),getTemporaryName());
      generateComposeExpr(static_cast<const compose_expr_node*>(expr),newVar);
      ///Now generate an assignment statement
      generateSimpleAssignment(expr,newVar,outputArg,outputIdxInfo);
    }    
    break;
  case VEC_ID:{
    ArgDesc* rhsVar = generateVectorIdExpr(static_cast<const vec_id_expr_node*>(expr));
    generateSimpleAssignment(expr,rhsVar,outputArg,outputIdxInfo);
  }
    break;
  case VEC_SCALAR:
    assert(0 && "LLVM Codegen unable to handle simple assignments using a scalar" );
    break;
  case VEC_MAKESTRUCT:
    generateMakeStruct(static_cast<const make_struct_node*>(expr),outputArg,outputIdxInfo);
    break;
  case VEC_SCALE:
    generateVecScaleExpr(static_cast<const vec_domainfn_node*>(expr),outputArg,outputIdxInfo);
    break;
  default:
    assert(0 && "Unhandled vector expression type");
  }
}



void CGVectorFn::generateVecScaleExpr(const vec_domainfn_node* expr, ArgDesc* outputArg, 
				      const domain_desc_node* outputIdxInfo)
{
  ArgDesc* baseVar;
  
  const vector_expr_node* baseExpr = expr->get_base_expr();
  ///First take care of subdomain used for the base expression
  if( baseExpr->get_sub_domain() != NULL || baseExpr->get_access_field() != -1 ){
    domain_node* base_domain = new domain_node(baseExpr->get_expr_domain());
    if( baseExpr->get_sub_domain() != NULL ){
      base_domain->compute_intersection(baseExpr->get_sub_domain());
      base_domain->realign_domain();
    }
    baseVar = allocateDomain(base_domain,baseExpr->get_data_type(),getTemporaryName());
    generateVectorExpr(baseExpr,baseVar,NULL);
    delete base_domain;
  }
  else{
    baseVar = generateVectorExpr(baseExpr);
  }

  unsigned numDims = expr->get_dim();
  std::deque<std::pair<Value*,Value*> > loopBounds;
  generateLoopBounds(expr->get_expr_domain(),loopBounds,dynamic_cast<const domainfn_node*>(outputIdxInfo),expr->get_scale_fn());


  auto origIP = builder->saveIP();

  Type* i32Ty = Type::getInt32Ty(cgm.getLLVMContext());
  //Create Iterators
  SmallVector<Value*,4> iterators;
  builder->SetInsertPoint(&thisFn->getEntryBlock(),thisFn->getEntryBlock().getFirstInsertionPt());
  for( unsigned i = 0 ; i != numDims ; i++ )
    iterators.push_back(builder->CreateAlloca(i32Ty));

  ///Inner Block
  BasicBlock* innerBB = BasicBlock::Create(cgm.getLLVMContext(),"inner-pt",thisFn);
  builder->SetInsertPoint(innerBB);

  auto& scaleFn = expr->get_scale_fn()->scale_fn;

  ///Rhs Index accessed
  Value* addr = builder->CreateLoad(iterators[numDims-1]);
  addr = builder->CreateMul(addr,builder->getInt32(scaleFn[numDims-1].scale));
  addr = builder->CreateAdd(addr,builder->getInt32(scaleFn[numDims-1].offset));
  Value* size = builder->CreateExtractValue(baseVar->val,{numDims});
  for( int i = numDims-2 ; i>=0 ; i-- ){
    Value* idx = builder->CreateLoad(iterators[i]);
    idx = builder->CreateMul(idx,builder->getInt32(scaleFn[i].scale));
    idx = builder->CreateAdd(idx,builder->getInt32(scaleFn[i].offset));
    Value* mul = builder->CreateMul(idx,size);
    size = builder->CreateMul(size,builder->CreateExtractValue(baseVar->val,{(unsigned)(i+1)}));
    addr = builder->CreateAdd(addr,mul);
  }
  Value* baseAddr = builder->CreateExtractValue(baseVar->val,{0});
  addr = builder->CreateInBoundsGEP(baseAddr,{addr});
  Value* readVal = builder->CreateLoad(addr);

  ///LhsIndex 
  Value* lhsAddr = generateLHSArrayAccess(iterators,outputArg,outputIdxInfo);
  builder->CreateStore(readVal,lhsAddr);

  BasicBlock* loopExit = BasicBlock::Create(cgm.getLLVMContext(),"exitBlock",thisFn);
  BasicBlock* loopEntry = generateLoopHeaderFooters(loopBounds,iterators,loopExit,innerBB);
  
  builder->restoreIP(origIP);
  builder->CreateBr(loopEntry);

  builder->SetInsertPoint(loopExit);
}



void CGVectorFn::generateVectorFnExpr(const fnid_expr_node* expr, ArgDesc* outputArg, 
				      const domain_desc_node* outputIdxInfo )
{
  vectorfn_defn_node* defn= static_cast<vectorfn_defn_node*>(expr->get_defn());
  assert(defn && "Expecting a vector function call, but definition is not that");
  StringRef fnName(defn->get_name());
  CGVectorFn* calledFn = cgm.addVectorFunction(fnName,defn);
  
  SmallVector<ArgDesc*,4> callArgs;
  std::deque<const domain_node*> arg_domains;
  ///Evaluate the arguments, and recompute the domain size information
  for( auto it = expr->get_args().begin() ; it != expr->get_args().end() ; it++ ){
    const arg_info & ai = *it;
    vector_expr_node* argExpr = ai.arg_expr;
    domain_node* temp_domain = new domain_node(argExpr->get_expr_domain());
    if( argExpr->get_sub_domain() != NULL || argExpr->get_access_field() != -1 ){
      if( argExpr->get_sub_domain() != NULL ){
	temp_domain->compute_intersection(argExpr->get_sub_domain());
	temp_domain->realign_domain();
      }
      ArgDesc* tempVar = allocateDomain(temp_domain,argExpr->get_data_type(),getTemporaryName());
      generateVectorExpr(argExpr,tempVar,NULL);
      callArgs.push_back(tempVar);
    }
    else{
      ArgDesc *v = generateVectorExpr(argExpr);
      assert(v && "Bad Vector Expression generated");
      callArgs.push_back(v);
    }
    arg_domains.push_back(temp_domain);
  }
  ///Put in the return argument as well, but its not really used right now
  callArgs.push_back(outputArg);

  //Save the current calling function parameters
  SmallVector<ArgDesc*,4> prevArgs;
  for( auto arg : calledFn->arguments )
    prevArgs.push_back(arg);


  ///Compute the domain of the vector function in the current context
  defn->compute_domain(arg_domains);

  ///Populate it with the new arguments
  calledFn->arguments.clear();
  auto calledFnParam = defn->get_args().begin();
  auto arg = callArgs.begin();
  for( ; calledFnParam != defn->get_args().end() ; arg++ , calledFnParam++ ){
    ArgDesc* calledFnArg = new ArgDesc((*arg)->type,(*arg)->dims,(*calledFnParam)->get_name());
    calledFnArg->val = (*arg)->val;
    calledFn->arguments.push_back(calledFnArg);
  }
  ArgDesc* returnArg = new ArgDesc((*arg)->type,(*arg)->dims,getTemporaryName());
  calledFn->arguments.push_back(returnArg);

  ///Populate the parameters
  calledFn->InitParams(paramMap);  

  //Set the function for code generation
  calledFn->thisFn = thisFn;

  calledFn->generateVectorFunction(defn,outputArg,outputIdxInfo);
  
  calledFn->thisFn  = NULL;

  for( auto arg : calledFn->arguments )
    delete arg;
  calledFn->arguments.clear();

  for( auto argDomain : arg_domains)
    delete argDomain;

  for( auto arg : prevArgs ){
    calledFn->arguments.push_back(arg);
  }

}


void CGVectorFn::generateComposeExpr(const compose_expr_node* expr, ArgDesc* outputArg)
{
  for( auto composeStmt : expr->get_expr_list() ){
    const vector_expr_node* currRHS = composeStmt.second;
    if( currRHS->get_sub_domain() != NULL || currRHS->get_access_field() != -1 ){
      domain_node* temp_domain = new domain_node(currRHS->get_expr_domain());
      if( currRHS->get_sub_domain() != NULL ){
	temp_domain->compute_intersection(currRHS->get_sub_domain());
	temp_domain->realign_domain();
      }
      ArgDesc* tempVar = allocateDomain(temp_domain,currRHS->get_data_type(),getTemporaryName());
      delete temp_domain;
      generateVectorExpr(currRHS,tempVar,NULL);
      generateSimpleAssignment(currRHS,tempVar,outputArg,composeStmt.first);
    }
    else{
      generateVectorExpr(currRHS,outputArg,composeStmt.first);
    }
  }
}


void CGVectorFn::generateMakeStruct(const make_struct_node* expr, ArgDesc* outputArg,
				    const domain_desc_node* outputIdxInfo)
{
  SmallVector<ArgDesc*,4> rhsVals;
  for( auto rhsExprs : expr->get_field_inputs() ){
    ArgDesc* currRhs = generateVectorExpr(rhsExprs);
    rhsVals.push_back(currRhs);
  }

  //const domain_node* domain = expr->get_expr_domain();
  domain_node* loop_domain = new domain_node(expr->get_expr_domain());
  // if( expr->get_sub_domain() != NULL ){
  //   loop_domain->compute_intersection(expr->get_sub_domain());
  //   loop_domain->realign_domain();
  // }
  unsigned numDims = loop_domain->get_dim();
  std::deque<std::pair<Value*,Value*> > loopBounds;
  generateLoopBounds(loop_domain,loopBounds,dynamic_cast<const domainfn_node*>(outputIdxInfo));

  auto origIP = builder->saveIP();

  Type* i32Ty = Type::getInt32Ty(cgm.getLLVMContext());
  //Create Iterators
  SmallVector<Value*,4> iterators;
  builder->SetInsertPoint(&thisFn->getEntryBlock(),thisFn->getEntryBlock().getFirstInsertionPt());
  for( unsigned i = 0 ; i != numDims ; i++ )
    iterators.push_back(builder->CreateAlloca(i32Ty));

  ///Inner Block
  BasicBlock* innerBB = BasicBlock::Create(cgm.getLLVMContext(),"inner-pt",thisFn);
  builder->SetInsertPoint(innerBB);
  
  ///Lhs Variable info
  Value* lhsAddr = generateLHSArrayAccess(iterators,outputArg,outputIdxInfo);
  StructType* lhsType = cgm.getLLVMModule()->getTypeByName(expr->get_data_type().struct_info->name);
  //llvm::errs() << "LHS Type : " << lhsType->getName().str() << "\n";
  assert(lhsType && "Unknown structure type formed during LLVM codegen");
  Value* currLhsVal = UndefValue::get(lhsType);

  int field_num = 0;
  auto currRhsVal = rhsVals.begin();
  auto currRhsExpr = expr->get_field_inputs().begin();
  for( ; currRhsVal != rhsVals.end() ; currRhsVal++, currRhsExpr++, field_num++){
    Value* currReadVal = NULL;
    if( (*currRhsExpr)->get_dim() != 0 ){
      TempVarInfo currRhsInfo(*currRhsVal,(*currRhsExpr)->get_sub_domain(),(*currRhsExpr)->get_access_field());
      currReadVal = generateRHSArrayAccess(iterators,currRhsInfo);
    }
    else{
      currReadVal = (*currRhsVal)->val;
    }
    currLhsVal = builder->CreateInsertValue(currLhsVal,currReadVal,field_num);
  }
  
  builder->CreateStore(currLhsVal,lhsAddr);

  BasicBlock* loopExit = BasicBlock::Create(cgm.getLLVMContext(),"exitBlock",thisFn);
  BasicBlock* loopEntry = generateLoopHeaderFooters(loopBounds,iterators,loopExit,innerBB);

  builder->restoreIP(origIP);
  builder->CreateBr(loopEntry);

  builder->SetInsertPoint(loopExit);
  delete loop_domain;
}


 void CGVectorFn::generateSimpleAssignment(const vector_expr_node* expr, ArgDesc* rhsVar,
					   ArgDesc* outputVar, const domain_desc_node* outputIdxInfo)
{
  //const domain_node* domain = expr->get_expr_domain();
  domain_node* loop_domain = new domain_node(expr->get_expr_domain());
  if( expr->get_sub_domain() != NULL ){
    loop_domain->compute_intersection(expr->get_sub_domain());
    loop_domain->realign_domain();
  }

  unsigned numDims = loop_domain->get_dim();
  std::deque<std::pair<Value*,Value*> > loopBounds;
  generateLoopBounds(loop_domain,loopBounds, dynamic_cast<const domainfn_node*>(outputIdxInfo));
  
  auto origIP = builder->saveIP();
  
  Type* i32Ty = Type::getInt32Ty(cgm.getLLVMContext());
  ///Create Iterators
  SmallVector<Value*,4> iterators;
  builder->SetInsertPoint(&thisFn->getEntryBlock(),thisFn->getEntryBlock().getFirstInsertionPt());
  for( unsigned i = 0 ;i != numDims ; i++ )
    iterators.push_back(builder->CreateAlloca(i32Ty));

  ///Inner Block
  BasicBlock* innerBB = BasicBlock::Create(cgm.getLLVMContext(),"inner-pt",thisFn);
  builder->SetInsertPoint(innerBB);
  
  Value* readValue;
  if( expr->get_dim() != 0 ){
    // ///Generate the RHS expr
    TempVarInfo rhsInfo(rhsVar,expr->get_sub_domain(),expr->get_access_field());
    readValue = generateRHSArrayAccess(iterators,rhsInfo);
  }
  else{
    readValue = rhsVar->val;
  }

  ///Generate the LHS expr and store the value read before
  Value* lhsAddr = generateLHSArrayAccess(iterators,outputVar,outputIdxInfo);
  builder->CreateStore(readValue,lhsAddr);

  BasicBlock* loopExit = BasicBlock::Create(cgm.getLLVMContext(),"exitBlock",thisFn);
  BasicBlock* loopEntry = generateLoopHeaderFooters(loopBounds,iterators,loopExit,innerBB);

  builder->restoreIP(origIP);
  builder->CreateBr(loopEntry);

  builder->SetInsertPoint(loopExit);
  
  delete loop_domain;
}



void CGVectorFn::generateStencilFnExpr(const fnid_expr_node *expr, ArgDesc* outputVar,
				       const domain_desc_node* outputIdxInfo) {
  // llvm::errs() << "- Visiting function: " << expr->get_name() << "\n";
  
  fn_defn_node *defn = expr->get_defn();
  
  //First process the arguments
  SmallVector<TempVarInfo*,4> fnArgVals;
  std::deque<bdy_info*> curr_bdy_info;
  std::deque<const domain_node*> argDomains;
  auto argInfo = expr->get_args();
  for( auto argInfoIter : argInfo ){
    domain_node* argDomain = new domain_node(argInfoIter.arg_expr->get_expr_domain());
    if( argInfoIter.arg_expr->get_sub_domain() != NULL )
      argDomain->compute_intersection(argInfoIter.arg_expr->get_sub_domain());
    argDomain->realign_domain();
    argDomains.push_back(argDomain);

    ArgDesc* argVal = NULL;
    if( argInfoIter.arg_expr->get_access_field() != -1 ){
      argVal = allocateDomain(argDomain,argInfoIter.arg_expr->get_data_type(),getTemporaryName());
      ArgDesc* rhsVar = generateVectorExpr(argInfoIter.arg_expr);
      generateSimpleAssignment(argInfoIter.arg_expr,rhsVar,argVal,NULL);
    }
    else
      argVal = generateVectorExpr(argInfoIter.arg_expr);
    assert(argVal && "Error in processing function arguments");
    // if (argInfoIter.arg_expr->get_sub_domain() != NULL )
    //   printf("Argument subdomain is not NULL\n");
    // else
    //   printf("Argumenr subdomain in NULL\n");
    TempVarInfo* argInfo = new TempVarInfo(argVal,argInfoIter.arg_expr->get_sub_domain(),
					   argInfoIter.arg_expr->get_access_field());
    fnArgVals.push_back(argInfo);
    bdy_info* newBdyInfo = new bdy_info(argInfoIter.bdy_condn->type,argInfoIter.bdy_condn->value);
    curr_bdy_info.push_back(newBdyInfo);
  }

  ///Recmopute the stencil fn domain
  stencilfn_defn_node* stencil_defn = static_cast<stencilfn_defn_node*>(defn);
  const domain_node* domain = stencil_defn->compute_domain(argDomains);

  //get the sizes of the domain
  unsigned numDims = domain->get_dim();
  SmallVector<Value*,4> dimSizes;
  generateDomainSize(domain,dimSizes);

  ///Add the base function to list of functions
  std::string baseFnNameStr = defn->get_name();
  baseFnNameStr.append("_stencil_");
  StringRef baseFnName(baseFnNameStr);
  cgm.addStencilFunction(baseFnName,static_cast<const stencilfn_defn_node*>(defn));
  
  //Add the function for the boundaries
  std::string bdyInfoName = cgm.getBdyInfoFnName(static_cast<const stencilfn_defn_node*>(defn),curr_bdy_info);

  StringRef fnName(bdyInfoName);
  assert(dynamic_cast<const stencilfn_defn_node*>(defn) != NULL );
  cgm.addStencilFunction(fnName,static_cast<const stencilfn_defn_node*>(defn),curr_bdy_info);

  const domainfn_node* outputScaleInfo = dynamic_cast<const domainfn_node*>(outputIdxInfo);
  
  ///Compute offsets for the inner domain regions
  std::deque<offset_hull> offsets;
  cgm.computeOffsets(expr, offsets , outputScaleInfo);

  ///Compute offsets for the boundary
  std::deque<std::pair<int,int> > currPatchInfo;
  std::deque<std::pair<Value*,bool> > * patches = new std::deque<std::pair<Value*,bool> >[numDims];
  computeBdyPatches(expr,offsets,0,numDims,patches,domain, outputScaleInfo);

  /// Generate code for all rectangular patches
  std::deque<std::pair<Value*,Value*> > loopBounds;
  generateBdyCode(expr, patches, loopBounds, 0, numDims, false, fnName, fnArgVals, outputVar, outputIdxInfo);
  delete[] patches;

  for( auto fnArgIt : fnArgVals )
    delete fnArgIt;

  for( auto argDomain : argDomains )
    delete argDomain;
}


void CGVectorFn::generateBdyCode( const fnid_expr_node* fn,
				  std::deque<std::pair<Value*,bool> > * patches,
				  std::deque<std::pair<Value*,Value*> >& loopBounds,
				  int currDim, int numDims, bool isBdy,
				  StringRef fnName,
				  SmallVector<TempVarInfo*,4>& fnArgVals,
				  ArgDesc* outputArg,
				  const domain_desc_node* outputIdxInfo) 
{
  if( currDim == numDims ){
    // llvm::errs() << "[MD] LoopBounds :"; 
    // for( auto boundsIter : loopBounds ){
    //   llvm::errs() << " (" << boundsIter.first << "," << boundsIter.second << ")" ;
    // }
    // llvm::errs() << " : " << isBdy <<  "\n";

    auto origIP = builder->saveIP();

    Type *i32Ty = Type::getInt32Ty(cgm.getLLVMContext());
    
    // Create some iterators
    SmallVector<Value *, 4> iterators;
    builder->SetInsertPoint(&thisFn->getEntryBlock(),
			   thisFn->getEntryBlock().getFirstInsertionPt());

    for (int i = 0; i != numDims; ++i)
      iterators.push_back(builder->CreateAlloca(i32Ty));


    // Generate the inner compute block
    BasicBlock *innerBB =
      BasicBlock::Create(cgm.getLLVMContext(), "inner-pt", thisFn);
    builder->SetInsertPoint(innerBB);

    SmallVector<Value *, 4> callArgs;

    // Send indices
    for (int i = 0; i != numDims; ++i) {
      Value *ld = builder->CreateLoad(iterators[i]);
      callArgs.push_back(ld);
    }

    // Send vectors
    for( auto fnArgIt = fnArgVals.begin() ; fnArgIt != fnArgVals.end() ; fnArgIt++ ){
      callArgs.push_back((*fnArgIt)->arg->val);
      unsigned argDim = (*fnArgIt)->arg->dims;
      Type* argInfoType = cgm.getInfoStructType(argDim);
      Value* offset = UndefValue::get(argInfoType);
      if( (*fnArgIt)->sub_domain != NULL ){
	unsigned dim_num = 0;
	for( auto offsetInfo : (*fnArgIt)->sub_domain->get_domain() ){
	  //printf(" Dim %d offset, lb : ",dim_num);
	  //offsetInfo.lb->print_node(stdout);
	  //printf("\n");
	  fflush(stdout);
	  Value* lbOffset = generateParametricExpr(offsetInfo.lb);
	  offset = builder->CreateInsertValue(offset,lbOffset,dim_num);
	  dim_num++;
	}
      }
      else{
	for(unsigned i = 0 ; i < argDim ; i++ )
	  offset = builder->CreateInsertValue(offset,builder->getInt32(0),i);
      }
      callArgs.push_back(offset);			 
    }

    // Send parameters
    for (auto it = global_params->begin(); it != global_params->end(); ++it) {
      const std::string &name = it->first;
      assert(paramMap.find(name) != paramMap.end() &&
	     "Accessing an unknown parameter");
      callArgs.push_back(paramMap[name]);
    }


    Function *fnPtr;
    if( isBdy ){
      fnPtr = cgm.getLLVMModule()->getFunction(fnName.str());
    }
    else{
      std::string baseFnNameStr = fn->get_name();
      baseFnNameStr.append("_stencil_");
      StringRef actualFnName(baseFnNameStr);
      fnPtr = cgm.getLLVMModule()->getFunction(actualFnName);
    }
    assert(fnPtr && "Function not found");
    Value *ret = builder->CreateCall(fnPtr, callArgs);

    // Generate linearized address for output
    Value* addr = generateLHSArrayAccess(iterators,outputArg,outputIdxInfo);
    builder->CreateStore(ret, addr);

    BasicBlock* loopExit = BasicBlock::Create(cgm.getLLVMContext(),"exitBlock",thisFn);
    BasicBlock* loopEntry = generateLoopHeaderFooters(loopBounds,iterators,loopExit,innerBB);
    // Set the insertion point to the exit block for later codegen
    
    builder->restoreIP(origIP);
    builder->CreateBr(loopEntry);
    
    builder->SetInsertPoint(loopExit);
  }
  else{
    auto dimLB = patches[currDim].begin();
    auto dimUB = patches[currDim].begin();
    dimUB++;
    unsigned patch_num = 0;
    for( ; dimUB != patches[currDim].end() ; dimLB++, dimUB++ , patch_num++ ){
      Value* ub = dimUB->first;
      //Due to the way patches are computed, (the start of patch is
      //stored for all except that last val), subtract 1 for all but the last value
      if( patch_num != patches[currDim].size() - 2 ){
	ub = builder->CreateSub(ub,builder->getInt32(1));
      }
      loopBounds.push_back(std::pair<Value*,Value*>(dimLB->first,ub));
      generateBdyCode(fn,patches,loopBounds,currDim+1,numDims,isBdy || dimLB->second, fnName, fnArgVals, outputArg, outputIdxInfo );
      loopBounds.pop_back();
    }
  }
}



BasicBlock* CGVectorFn::generateLoopHeaderFooters(std::deque<std::pair<Value*,Value*> >& loopBounds,
						  SmallVector<Value*,4>& iterators,
						  BasicBlock* exitBlock, BasicBlock* innerBB)
{
  // Create the header/footer basic blocks
  SmallVector<BasicBlock *, 4> preheaders;
  SmallVector<BasicBlock *, 4> headers;
  SmallVector<BasicBlock *, 4> footers;
  
  int numDims = loopBounds.size();

  for (int i = 0; i != (int)loopBounds.size(); ++i) {
    BasicBlock *prehdr =
      BasicBlock::Create(cgm.getLLVMContext(), "preheader", thisFn);
    preheaders.push_back(prehdr);
    BasicBlock *hdr =
      BasicBlock::Create(cgm.getLLVMContext(), "header", thisFn);
    headers.push_back(hdr);
    BasicBlock *ftr =
      BasicBlock::Create(cgm.getLLVMContext(), "footer", thisFn);
    footers.push_back(ftr);
  }
  
  /// The IP is assumed to be at the inner basic block
  // Create branch from loop body to inner-most footer
  builder->CreateBr(footers[footers.size()-1]);

  // Generate the loop headers/footers
  for (int i = 0; i != (int)loopBounds.size(); ++i) {
    // Preheader
    builder->SetInsertPoint(preheaders[i]);

    // What is our starting index?
    builder->CreateStore(loopBounds[i].first, iterators[i]);
    builder->CreateBr(headers[i]);

    // Header
    builder->SetInsertPoint(headers[i]);

    Value *iterVal = builder->CreateLoad(iterators[i]);
    Value *cmp = builder->CreateICmpUGT(iterVal, loopBounds[i].second);

    BasicBlock *loopBody = (i == numDims-1) ? innerBB : preheaders[i+1];
    BasicBlock *loopExit = (i == 0) ? exitBlock : footers[i-1];
    builder->CreateCondBr(cmp, loopExit, loopBody);


    // Footer
    builder->SetInsertPoint(footers[i]);
    Value *newIter = builder->CreateAdd(iterVal, builder->getInt32(1));
    builder->CreateStore(newIter, iterators[i]);
    builder->CreateBr(headers[i]);
  }

  // Set the insertion point to the exit block for later codegen
  builder->SetInsertPoint(exitBlock);

  return preheaders[0];
}


ArgDesc *CGVectorFn::generateVectorIdExpr(const vec_id_expr_node *expr) {
  const std::string &name = expr->get_name();
  auto defn = expr->get_defn();
  assert(defn.size() == 1 && "Expected 1 definition");
  // const vector_expr_node *defnExpr = *(defn.begin());
  // unsigned numDim = defnExpr->get_dim();

  // XXX: For testing, we are assuming the expression is an argument
  ArgDesc *v = getArgumentByName(name);
  if (v == NULL) {
    // Not an argument, could be a previous vector expr
    auto it = locals.find(name);
    if (it != locals.end()) {
      v = it->second;
    }
    else{
      printf("[ERROR] : Couldnt find variable : %s\n",name.c_str());
      assert(0);
    }
  }
  return v;
}

Value *CGVectorFn::generateParametricExpr(const parametric_exp *pexp) {
  if (const int_expr *expr = dynamic_cast<const int_expr *>(pexp)) {
    // Constant value
    return builder->getInt32(expr->value);
  } else if (const binary_expr *expr =
               dynamic_cast<const binary_expr *>(pexp)) {
    Value *lhs = generateParametricExpr(expr->lhs_expr);
    Value *rhs = generateParametricExpr(expr->rhs_expr);
    switch (expr->type) {
    case P_ADD:
      return builder->CreateAdd(lhs, rhs);
    case P_SUBTRACT:
      return builder->CreateSub(lhs, rhs);
    case P_MULTIPLY:
      return builder->CreateMul(lhs, rhs);
    case P_DIVIDE:
      return builder->CreateUDiv(lhs, rhs);
    case P_MIN: {
      SmallVector<Type*,2> argTypes;
      argTypes.push_back(builder->getInt32Ty());
      argTypes.push_back(builder->getInt32Ty());
      FunctionType* minFnTy = FunctionType::get(builder->getInt32Ty(), argTypes, false);
      Constant *minFn = cgm.getLLVMModule()->getOrInsertFunction("min",minFnTy);
      SmallVector<Value*,2> args;
      args.push_back(lhs);
      args.push_back(rhs);
      return builder->CreateCall(minFn,args);
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

void CGVectorFn::computeBdyPatches(const fnid_expr_node* fn, 
				   std::deque<offset_hull> &offsets,
				   int currDim , int numDims,
				   std::deque<std::pair<Value*,bool> > * patches,
				   const domain_node* domain,
				   const domainfn_node* outputIdxInfo) 
{
  const fn_defn_node* fnDefn = fn->get_defn();
  auto fnParams = fnDefn->get_args();
  auto currArgs = fn->get_args();

  ///Look at arguments, al those without any arguments decide the
  ///start (end) of bdy before (after)
  auto fnArgIt = fnParams.begin();
  auto argInfoIt = currArgs.begin();
  auto fnArgEnd = fnParams.end() ;
  int bdyStart = 0, bdyEnd = 0;
  if( outputIdxInfo ){
    bdyStart = std::min(bdyStart,FLOOR(outputIdxInfo->scale_fn[currDim].offset,outputIdxInfo->scale_fn[currDim].scale));
    bdyEnd = std::max(bdyEnd,FLOOR(outputIdxInfo->scale_fn[currDim].offset,outputIdxInfo->scale_fn[currDim].scale));
  }
  for( ; fnArgIt != fnArgEnd ; ++fnArgIt, ++argInfoIt ){
    if( (*fnArgIt)->get_dim() != 0 &&  argInfoIt->bdy_condn->type == B_NONE ){
      auto accessInfo = (*fnArgIt)->get_access_info();
      bdyStart = std::min(bdyStart,FLOOR(accessInfo[currDim].max_negetive,accessInfo[currDim].scale));
      bdyEnd = std::max(bdyEnd,CEIL(accessInfo[currDim].max_positive,accessInfo[currDim].scale));
    }      
  }
  assert( bdyStart >= offsets[currDim].max_negetive );
  assert( bdyEnd <= offsets[currDim].max_positive );

  ///Compute the patches each dimension is split into
  // llvm::errs() << "[MD] Dim : " << currDim << ",";
  if( bdyStart > offsets[currDim].max_negetive ){
    // llvm::errs() << " " << bdyStart ;
    parametric_exp *start = domain->get_domain()[currDim].lb;
    Value* bdyStartVal = generateParametricExpr(start);
    bdyStartVal = builder->CreateAdd(bdyStartVal,builder->getInt32((-bdyStart)));
    patches[currDim].push_back(std::pair<Value*,bool>(bdyStartVal,true));
  }
  // llvm::errs() << " " << offsets[currDim].max_negetive;
  parametric_exp* main_start = domain->get_domain()[currDim].lb;
  Value* mainStartVal = generateParametricExpr(main_start);
  mainStartVal = builder->CreateAdd(mainStartVal,builder->getInt32(-(int)(FLOOR(offsets[currDim].max_negetive,offsets[currDim].scale))));
  patches[currDim].push_back(std::pair<Value*,bool>(mainStartVal,false));
			     
  // llvm::errs() << " " << offsets[currDim].max_positive;
  parametric_exp* main_end = domain->get_domain()[currDim].ub;
  Value* mainEndVal = generateParametricExpr(main_end);
  mainEndVal = builder->CreateSub(mainEndVal,builder->getInt32((int)CEIL(offsets[currDim].max_positive,offsets[currDim].scale)));
  patches[currDim].push_back(std::pair<Value*,bool>(mainEndVal,true));

  if( bdyEnd < offsets[currDim].max_positive ){
    //The last patch position has to be made a lb of next patch (add 1)
    auto prevUb = patches[currDim].rbegin();
    prevUb->first = builder->CreateAdd(prevUb->first,builder->getInt32(1));

    // llvm::errs() << " " << bdyEnd;
    parametric_exp* bdy_end = domain->get_domain()[currDim].ub;
    Value* bdyEndVal = generateParametricExpr(bdy_end);
    bdyEndVal = builder->CreateSub(bdyEndVal,builder->getInt32(bdyEnd));
    patches[currDim].push_back(std::pair<Value*,bool>(bdyEndVal,true));
  }   
  // llvm::errs() << "\n";

  if( currDim < numDims - 1 )
    computeBdyPatches(fn, offsets, currDim+1, numDims, patches, domain , outputIdxInfo );
}


void CGVectorFn::generateLoopBounds(const domain_node* domain, 
				    std::deque<std::pair<Value*,Value*> >& loopBounds,
				    const domainfn_node* outputIdxInfo,
				    const domainfn_node* inputScaleInfo)
{
  unsigned numDims = domain->get_dim();
  SmallVector<int,4> lb_offset,ub_offset;
  if( outputIdxInfo ){
    for( unsigned i = 0 ; i < numDims ; i++ ){
      int lbStart = std::min(0,FLOOR(outputIdxInfo->scale_fn[i].offset,outputIdxInfo->scale_fn[i].scale));
      int ubStart = std::max(0,FLOOR(outputIdxInfo->scale_fn[i].offset,outputIdxInfo->scale_fn[i].scale));
      lb_offset.push_back(lbStart);
      ub_offset.push_back(ubStart);
    }
  }
  else{
    for( unsigned i = 0 ; i != numDims ; i++ ){
      lb_offset.push_back(0);
      ub_offset.push_back(0);
    }
  }
  if( inputScaleInfo ){
    for( unsigned i = 0 ; i < numDims ; i++ ){
      lb_offset[i] = std::min(lb_offset[i],FLOOR(inputScaleInfo->scale_fn[i].offset,
						 inputScaleInfo->scale_fn[i].scale));
      ub_offset[i] = std::max(ub_offset[i],FLOOR(inputScaleInfo->scale_fn[i].offset,
						inputScaleInfo->scale_fn[i].scale));
    }
  }
  for( unsigned i = 0 ; i != numDims ; i++ ){
    parametric_exp* start = domain->get_domain()[i].lb;
    Value* startVal = generateParametricExpr(start);
    startVal = builder->CreateAdd(startVal,builder->getInt32(-lb_offset[i]));
    parametric_exp* stop = domain->get_domain()[i].ub;
    Value* stopVal = generateParametricExpr(stop);
    stopVal = builder->CreateSub(stopVal,builder->getInt32(ub_offset[i]));
    loopBounds.push_back(std::pair<Value*,Value*>(startVal,stopVal));
  }  
}


void CGVectorFn::generateDomainSize(const domain_node* domain, SmallVector<Value*,4>& dimSizes)
{
  unsigned numDims = domain->get_dim();
  for( unsigned i = 0 ; i != numDims ; i++ ){
    parametric_exp* start = domain->get_domain()[i].lb;
    Value* startVal = generateParametricExpr(start);
    parametric_exp* stop = domain->get_domain()[i].ub;
    Value* stopVal = generateParametricExpr(stop);
    Value* diff = builder->CreateSub(stopVal,startVal);
    Value* size = builder->CreateAdd(diff,builder->getInt32(1));
    dimSizes.push_back(size);
  }  
}



Value* CGVectorFn::generateRHSArrayAccess( const SmallVector<Value*,4>& iterators, 
					   TempVarInfo& rhsVarInfo)
{
  unsigned numDims = iterators.size();
  Value* rhsVar = rhsVarInfo.arg->val;
  SmallVector<Value*,4> subDomainOffset;
  if( rhsVarInfo.sub_domain ) {
    auto offsetInfo = rhsVarInfo.sub_domain->get_domain();
    for( auto offsetIter : offsetInfo ){
      if( !offsetIter.lb->is_default() ){
	Value* lb_offset = generateParametricExpr(offsetIter.lb);
	subDomainOffset.push_back(lb_offset);
      }
      else
	subDomainOffset.push_back(builder->getInt32(0));
    }
  }
  else{
    for( unsigned i = 0 ; i < numDims ; i++ )
      subDomainOffset.push_back(builder->getInt32(0));
  }
  Value* addr = builder->CreateLoad(iterators[numDims-1]);
  addr = builder->CreateAdd(addr,subDomainOffset[numDims-1]);
  Value* size = builder->CreateExtractValue(rhsVar,{numDims});
  for( int i = numDims-2 ; i >= 0 ; i-- ){
    Value* idx = builder->CreateLoad(iterators[i]);
    idx = builder->CreateAdd(idx,subDomainOffset[i]);
    Value* mul = builder->CreateMul(idx,size);
    size = builder->CreateMul(size,builder->CreateExtractValue(rhsVar,{(unsigned)(i+1)}));
    addr = builder->CreateAdd(addr,mul);
  }
  Value* baseAddr = builder->CreateExtractValue(rhsVar,{0});
  addr = builder->CreateInBoundsGEP(baseAddr,{addr});
  Value* retValue = builder->CreateLoad(addr);
  if( rhsVarInfo.access_field_num != -1 ){
    retValue = builder->CreateExtractValue(retValue,rhsVarInfo.access_field_num);
  }
  return retValue;
}
				  

Value* CGVectorFn::generateLHSArrayAccess(const SmallVector<Value*,4>& iterators, ArgDesc* lhsVar,
					  const domain_desc_node* outputIdxInfo)
{
  int numDims = iterators.size();
  SmallVector<Value*,4> idxScale,idxOffset;
  if( outputIdxInfo ){
    int outputDim = outputIdxInfo->get_dim();
    if( dynamic_cast<const domainfn_node*>(outputIdxInfo) ){
      assert( (outputDim == numDims) 
	      && "LLVM Codegen error : Scale Domain and iteration domain not of the same size" );
      const domainfn_node* outputScaleInfo = static_cast<const domainfn_node*>(outputIdxInfo);
      assert( ( iterators.size() == outputScaleInfo->scale_fn.size() ) && 
	      "Mismatch in loop depth and extrapolate function dimensions");
      for( int i = 0 ; i < outputDim ; i++ ){
	idxScale.push_back(builder->getInt32(outputScaleInfo->scale_fn[i].scale));
	idxOffset.push_back(builder->getInt32(outputScaleInfo->scale_fn[i].offset));
      }
    }
    else{
      assert( outputDim >= numDims 
	      && "LLVM Codegen error : LHS offset domain is smaller than iteration domain, invalid usage");
      const domain_node* outputOffsetInfo = static_cast<const domain_node*>(outputIdxInfo);
      for( int i = 0 ; i < outputDim ; i++ ){
	idxScale.push_back(builder->getInt32(1));
	Value* offsetVal = generateParametricExpr(outputOffsetInfo->range_list[i].lb);
	idxOffset.push_back(offsetVal);
      }
    }
  }
  else{
    for( int i = 0 ; i < numDims ; i++ ){
      idxScale.push_back(builder->getInt32(1));
      idxOffset.push_back(builder->getInt32(0));
    }
  }

  Value* addr = builder->CreateLoad(iterators[numDims-1]);
  int outputDim = idxOffset.size() - 1;
  addr = builder->CreateMul(addr,idxScale[outputDim]);
  addr = builder->CreateAdd(addr,idxOffset[outputDim]);
  Value* size = builder->CreateExtractValue(lhsVar->val,{(unsigned)(outputDim+1)});
  outputDim--;
  for( int i = numDims-2 ; i >= 0 ; i-- , outputDim-- ){
    Value* idx = builder->CreateLoad(iterators[i]);
    idx = builder->CreateMul(idx,idxScale[outputDim]);
    idx = builder->CreateAdd(idx,idxOffset[outputDim]);
    Value* mul = builder->CreateMul(idx,size);
    size = builder->CreateMul(size,builder->CreateExtractValue(lhsVar->val,{(unsigned)(outputDim+1)}));
    addr = builder->CreateAdd(addr,mul);
  }
  for( ; outputDim >= 0 ; outputDim-- ){
    Value* mul = builder->CreateMul(idxOffset[outputDim],size);
    size = builder->CreateMul(size,builder->CreateExtractValue(lhsVar->val,{(unsigned)(outputDim+1)}));
    addr = builder->CreateAdd(addr,mul);
  }
  
  Value* baseAddr = builder->CreateExtractValue(lhsVar->val,{0});
  addr = builder->CreateInBoundsGEP(baseAddr,{addr});
  return addr;
}

    
