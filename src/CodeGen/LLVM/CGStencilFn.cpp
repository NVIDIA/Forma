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
#include "AST/parser.hpp"
#include "llvm/Support/raw_ostream.h"
#include "CodeGen/LLVM/CGStencilFn.h"
#include <string>

using namespace llvm;

Value* CGStencilFn::castToType(Value* currVal, const data_types& currType)
{
  switch( currType.type ){
  case T_DOUBLE:{
    switch(currVal->getType()->getTypeID()){
      ///Cast from double to double
    case Type::DoubleTyID :
      return currVal;
      ///Cast from float to double
    case Type::FloatTyID :
      return builder.CreateFPExt(currVal,Type::getDoubleTy(cgm.getLLVMContext()));
    case Type::IntegerTyID :{
      IntegerType* currValType = cast<IntegerType>(currVal->getType());
      ///Cast from int8 to double
      if( currValType->getBitWidth() == 8 )
	return builder.CreateUIToFP(currVal, Type::getDoubleTy(cgm.getLLVMContext()));
      ///Cast from int* to Double
      else
	return builder.CreateSIToFP(currVal, Type::getDoubleTy(cgm.getLLVMContext()));
    }
    default:
      assert(0 && "Incompatible Cast operation");
      return NULL;
    }
  }
  case T_FLOAT: {
    switch(currVal->getType()->getTypeID()){
      ///Cast from double to float
    case Type::DoubleTyID :
      return builder.CreateFPTrunc(currVal,Type::getFloatTy(cgm.getLLVMContext()));
      ///Cast from float to float
    case Type::FloatTyID:
      return currVal;
    case Type::IntegerTyID : {
      IntegerType* currValType = cast<IntegerType>(currVal->getType());
      ///Cast from int8 to float
      if( currValType->getBitWidth() == 8 )
	return builder.CreateUIToFP(currVal, Type::getFloatTy(cgm.getLLVMContext()));
      else
	///Cast from int* to float
	return builder.CreateSIToFP(currVal, Type::getFloatTy(cgm.getLLVMContext()));
    }
    default:
      assert(0 && "Incompatible Cast Operation");
      return NULL;
    }
  }
  case T_INT : {
    switch(currVal->getType()->getTypeID()){
      ///cast from double to int
    case Type::DoubleTyID:
      return builder.CreateFPToSI(currVal,Type::getInt32Ty(cgm.getLLVMContext()));
      ///Cast from float to int
    case Type::FloatTyID:
      return builder.CreateFPToSI(currVal,Type::getInt32Ty(cgm.getLLVMContext()));
    case Type::IntegerTyID: {
      IntegerType* currValType = cast<IntegerType>(currVal->getType());
      switch( currValType->getBitWidth() ){
      case 32:
      	return currVal;
      case 16:
      	///Cast from int16 to int
      	return builder.CreateSExt(currVal,Type::getInt32Ty(cgm.getLLVMContext()));
      case 8:
	///Case from int8 to int
	return builder.CreateZExt(currVal,Type::getInt32Ty(cgm.getLLVMContext()));
      default:
	assert(0 && "Unsupported Integer Type");
	return NULL;
      }
    }
    default:
      assert(0 && "Incompatible Cast Operation");
      return NULL;
    }
  }
  case T_INT16: {
    switch(currVal->getType()->getTypeID()){
      ///cast from double to int16
    case Type::DoubleTyID:
      return builder.CreateFPToSI(currVal,Type::getInt16Ty(cgm.getLLVMContext()));
      ///Cast from float to int16
    case Type::FloatTyID:
      return builder.CreateFPToSI(currVal,Type::getInt16Ty(cgm.getLLVMContext()));
    case Type::IntegerTyID: {
      IntegerType* currValType = cast<IntegerType>(currVal->getType());
      switch ( currValType->getBitWidth()) {
      case 32:
	///Cast from int to int16
	return builder.CreateTrunc(currVal,Type::getInt16Ty(cgm.getLLVMContext()));
      case 16:
	return currVal;
      case 8:
	///Cast from int16 to int8
	return builder.CreateZExt(currVal,Type::getInt16Ty(cgm.getLLVMContext()));
      default:
	assert(0 && "Unsupported integer type found");
	return NULL;
      }
    }
    default:
      assert(0 && "Incompatible Cast Operation");
      return NULL;
    }
  }
  case T_INT8: {
    switch(currVal->getType()->getTypeID()){
    /// Cast from double to int8
    case Type::DoubleTyID:
      return builder.CreateFPToUI(currVal,Type::getInt8Ty(cgm.getLLVMContext()));
      ///Cast from float to int8
    case Type::FloatTyID:
      return builder.CreateFPToUI(currVal,Type::getInt8Ty(cgm.getLLVMContext()));
    case Type::IntegerTyID: {
      IntegerType* currValType = cast<IntegerType>(currVal->getType());
      if( currValType->getBitWidth() == 8 )
	return currVal;
      ///Cast from int* to int8
      else
	return builder.CreateTrunc(currVal,Type::getInt8Ty(cgm.getLLVMContext()));
    }
    default:
      assert(0 && "Unsupported integer type found");
      return NULL;
    }
  }
  default:
    assert(0 && "Unsupported Cast Operation");
    return NULL;
  }
}


void CGStencilFn::checkTypes(Value* &lhsValue, Value* &rhsValue){
  Type::TypeID lhsTypeID = lhsValue->getType()->getTypeID();
  Type::TypeID rhsTypeID = rhsValue->getType()->getTypeID();
  if( lhsTypeID == rhsTypeID ) 
    return;

  switch(lhsTypeID ){
  case Type::DoubleTyID: {
    switch (rhsTypeID ){
    case Type::FloatTyID :
      rhsValue = builder.CreateFPExt(rhsValue,Type::getDoubleTy(cgm.getLLVMContext()));
      break;
    case Type::IntegerTyID :{
      if( (cast<IntegerType>(rhsValue->getType()))->getBitWidth() == 8 )
	rhsValue = builder.CreateUIToFP(rhsValue,Type::getDoubleTy(cgm.getLLVMContext()));
      else
	rhsValue = builder.CreateSIToFP(rhsValue,Type::getDoubleTy(cgm.getLLVMContext()));
    }
      break;
    
    default:
      assert(0 && "Incompatible types in binary operations, double and Unknown");
    }
  }
    break;
  case Type::FloatTyID: {
    switch (rhsTypeID) {
    case Type::DoubleTyID :
      lhsValue = builder.CreateFPExt(lhsValue,Type::getDoubleTy(cgm.getLLVMContext()));
      break;
    case Type::IntegerTyID : {
      if( (cast<IntegerType>(rhsValue->getType()))->getBitWidth() == 8 )
	rhsValue = builder.CreateUIToFP(rhsValue,Type::getFloatTy(cgm.getLLVMContext()));
      else
	rhsValue = builder.CreateSIToFP(rhsValue,Type::getFloatTy(cgm.getLLVMContext()));
    }
      break;
    default:
      assert(0 && "Incompatible types in binary operations, float and Unknown");
    }
  }
    break;
  case Type::IntegerTyID : {
    IntegerType* lhsType = cast<IntegerType>(lhsValue->getType());
    if( lhsType->getBitWidth() == 8 ){
      switch(rhsTypeID) {
      case Type::DoubleTyID :
	lhsValue = builder.CreateUIToFP(lhsValue,Type::getDoubleTy(cgm.getLLVMContext()));
	break;
      case Type::FloatTyID :
	lhsValue = builder.CreateUIToFP(lhsValue,Type::getFloatTy(cgm.getLLVMContext()));
	break;
      default:
	assert(0 && "Incompatible types in binary operations, integer and Unknown");
      }
    }
    else{
      switch(rhsTypeID) {
      case Type::DoubleTyID :
	lhsValue = builder.CreateSIToFP(lhsValue,Type::getDoubleTy(cgm.getLLVMContext()));
	break;
      case Type::FloatTyID :
	lhsValue = builder.CreateSIToFP(lhsValue,Type::getFloatTy(cgm.getLLVMContext()));
	break;
      default:
	assert(0 && "Incompatible types in binary operations, integer and Unknown");
      }
    }
  }
    break;
  default:
    assert(0 && "Incompatible types in binary operations , unknown and unknown");
  }
}


Value* CGStencilFn::generateMinCode(Value* a, Value* b)
{
  checkTypes(a,b);
  Value* compare;
  if( a->getType()->isIntegerTy() ){
    compare = builder.CreateICmpSLE(a,b);
  }
  else{
    compare = builder.CreateFCmpULE(a,b);
  }
  return builder.CreateSelect(compare,a,b);

}

Value* CGStencilFn::generateMaxCode(Value* a, Value* b)
{
  Value* compare;
  checkTypes(a,b);
  if( a->getType()->isIntegerTy()) {
    compare = builder.CreateICmpSGT(a,b);
  }
  else{
    compare = builder.CreateFCmpOGT(a,b);
  }
  return builder.CreateSelect(compare,a,b);
}


Value *CGStencilFn::generateBinaryOp(const expr_op_node *expr) {

  // Get the two sub-expressions
  const expr_node *lhs = expr->get_lhs_expr();
  const expr_node *rhs = expr->get_rhs_expr();
  operator_type op = expr->get_op();

  Value *lhsValue = generateExpr(lhs);
  Value *rhsValue = generateExpr(rhs);

  //Ensure that both operands are of the same type
  checkTypes(lhsValue,rhsValue);
  
  bool useFloatOp = lhsValue->getType()->isFloatingPointTy() || rhsValue->getType()->isFloatingPointTy();
  ///Integer types are promoted to int32 for ops to maintain precision
  if( !useFloatOp ){
    IntegerType* rhsIntType = cast<IntegerType>(rhsValue->getType());
    switch( rhsIntType->getBitWidth() ){
    case 8:
      rhsValue = builder.CreateZExt(rhsValue,Type::getInt32Ty(cgm.getLLVMContext()));
      break;
    case 16:
      rhsValue = builder.CreateSExt(rhsValue,Type::getInt32Ty(cgm.getLLVMContext()));
      break;
    case 32: 
      break;
    default:
      assert(0 && "Unhandled integer type");
    }

    IntegerType* lhsIntType = cast<IntegerType>(lhsValue->getType());
    switch( lhsIntType->getBitWidth() ){
    case 8:
      lhsValue = builder.CreateZExt(lhsValue,Type::getInt32Ty(cgm.getLLVMContext()));
      break;
    case 16:
      lhsValue = builder.CreateSExt(lhsValue,Type::getInt32Ty(cgm.getLLVMContext()));
      break;
    case 32: 
      break;
    default:
      assert(0 && "Unhandled integer type");
    }
  }

  BinaryOperator::BinaryOps llvmOp;

  Value* retValue; 

  switch (op) {
  case O_PLUS:
    llvmOp = (useFloatOp ? BinaryOperator::FAdd : BinaryOperator::Add );
    retValue = builder.CreateBinOp(llvmOp, lhsValue, rhsValue, "");
    break;
  case O_MINUS:
    llvmOp = (useFloatOp ? BinaryOperator::FSub : BinaryOperator::Sub );
    retValue = builder.CreateBinOp(llvmOp, lhsValue, rhsValue, "");
    break;
  case O_MULT:
    llvmOp = (useFloatOp ? BinaryOperator::FMul : BinaryOperator::Mul );
    retValue = builder.CreateBinOp(llvmOp, lhsValue, rhsValue, "");;
    break;
  case O_DIV:
    if( useFloatOp )
      llvmOp = BinaryOperator::FDiv;
    else{
      if( expr->get_data_type().type == T_INT8 )
	llvmOp = BinaryOperator::UDiv;
      else
	llvmOp = BinaryOperator::SDiv;
    }
    retValue = builder.CreateBinOp(llvmOp, lhsValue, rhsValue, "");
    break;
  case O_LT:
    retValue = ( useFloatOp ? builder.CreateFCmp(CmpInst::FCMP_OLT,lhsValue,rhsValue)
		 : builder.CreateICmp(CmpInst::ICMP_SLT,lhsValue,rhsValue) );
    break;
  case O_GT:
    retValue = ( useFloatOp ? builder.CreateFCmp(CmpInst::FCMP_OGT,lhsValue,rhsValue)
		 : builder.CreateICmp(CmpInst::ICMP_SGT,lhsValue,rhsValue) );
    break;
  case O_LE:
    retValue = ( useFloatOp ? builder.CreateFCmp(CmpInst::FCMP_OLE,lhsValue,rhsValue)
		 : builder.CreateICmp(CmpInst::ICMP_SLE,lhsValue,rhsValue) );
    break;
  case O_GE:
    retValue = ( useFloatOp ? builder.CreateFCmp(CmpInst::FCMP_OGE,lhsValue,rhsValue)
		 : builder.CreateICmp(CmpInst::ICMP_SGE,lhsValue,rhsValue) );
    break;
  case O_EQ:
    retValue = ( useFloatOp ? builder.CreateFCmp(CmpInst::FCMP_OEQ,lhsValue,rhsValue)
		 : builder.CreateICmp(CmpInst::ICMP_EQ,lhsValue,rhsValue) );
    break;
  case O_NE:
    retValue = ( useFloatOp ? builder.CreateFCmp(CmpInst::FCMP_ONE,lhsValue,rhsValue)
		 : builder.CreateICmp(CmpInst::ICMP_NE,lhsValue,rhsValue) );
    break;
  default:
    llvm_unreachable("Unhandled binary operator");
  }  
  
  return retValue;
}

Value *CGStencilFn::checkBdyIndices(Value* addr, Value* size , Value* isBdy, const bdy_info* bdyCondn )
{
  if( bdyCondn->type == B_NONE )
    return addr;

  Value* checkBeforeBdy = builder.CreateICmpSGE(addr,builder.getInt32(0));
  BasicBlock* trueLBBranch = BasicBlock::Create(cgm.getLLVMContext(),"lbCheckTrue",thisFn);
  BasicBlock* falseLBBranch = BasicBlock::Create(cgm.getLLVMContext(),"lbCheckFalse",thisFn);
  BasicBlock* falseUBBranch = BasicBlock::Create(cgm.getLLVMContext(),"ubCheckFalse",thisFn);
  BasicBlock* exitIndex = BasicBlock::Create(cgm.getLLVMContext(),"exitIndexCheck",thisFn);
  builder.CreateCondBr(checkBeforeBdy,trueLBBranch,falseLBBranch);
 
  builder.SetInsertPoint(trueLBBranch);
  Value* checkAfterBdy = builder.CreateICmpSLT(addr,size);
  builder.CreateCondBr(checkAfterBdy,exitIndex,falseUBBranch);

  Value* retValLB;
  Value* retValUB;

  builder.SetInsertPoint(falseLBBranch);
  switch( bdyCondn->type) {
  case B_CONSTANT:{
    Value* currIsBdy = builder.CreateLoad(isBdy);
    Value* storeVal = builder.CreateOr(currIsBdy,builder.getInt1(true));
    builder.CreateStore(storeVal,isBdy);
    retValLB = builder.CreateAdd(addr,builder.getInt32(0));
  }
    break;
  case B_CLAMPED:
  case B_EXTEND:
    retValLB = builder.getInt32(0);
    break;
  case B_WRAP:
    retValLB = builder.CreateAdd(addr,size);
    break;
  case B_MIRROR:
    retValLB = builder.CreateSub(builder.getInt32(0),addr);
    break;
  default:
    {
      assert(0 && "Invalid Bdy Condn");
      retValLB = NULL;
    }    
  }
  builder.CreateBr(exitIndex);

  builder.SetInsertPoint(falseUBBranch);
  switch(bdyCondn->type) {
  case B_CONSTANT:{
    Value* currIsBdy = builder.CreateLoad(isBdy);
    Value* storeVal = builder.CreateOr(currIsBdy,builder.getInt1(true));
    builder.CreateStore(storeVal,isBdy);
    retValUB = builder.CreateAdd(addr,builder.getInt32(0));
  }
    break;
  case B_CLAMPED:
  case B_EXTEND:
    retValUB = builder.CreateSub(size,builder.getInt32(1));
    break;
  case B_WRAP:
    retValUB = builder.CreateSub(addr,size);
    break;
  case B_MIRROR:{
    Value* maxRight = builder.CreateSub(size,builder.getInt32(1));
    Value* twiceMaxRight = builder.CreateMul(builder.getInt32(2),maxRight);
    retValUB = builder.CreateSub(twiceMaxRight,addr);
  }
    break;
  default:
    {
      assert(0 && "Invalid Bdy Condn");
      retValUB = NULL;
    }
  }
  builder.CreateBr(exitIndex);
  
  builder.SetInsertPoint(exitIndex);
  
  PHINode* retVal = builder.CreatePHI(Type::getInt32Ty(cgm.getLLVMContext()),3,"fixIndex");
  
  retVal->addIncoming(addr,trueLBBranch);
  retVal->addIncoming(retValLB,falseLBBranch);
  retVal->addIncoming(retValUB,falseUBBranch);

  return retVal;
}

Value* CGStencilFn::generateArrayAccess(const array_access_node* expr){
  Value* retVal = NULL;
  
  const std::string& paramName = expr->get_name();
  const vector_defn_node* defn = expr->get_var();
  unsigned numDims = defn->get_dim();
  
  ArgDesc *argValue = getArgumentByName(paramName);
  ArgDesc* argInfoValue = getArgOffsetInfo(paramName);

  assert(argValue && "Argument not found");

  if( numDims == 0 ){
    retVal = argValue->val;
  }
  else{
    SmallVector<Value*, 4> dimSizes;
    SmallVector<Value*, 4> dimIndex;
    auto indexExprs = expr->get_index_exprs();
    assert(indexExprs.size() == numDims && "Dimension mismatch in array access");
    auto currExpr = indexExprs.begin();
    for( unsigned i = 0 , e = numDims ; i != e ; ++i , currExpr++ ){
      dimSizes.push_back(builder.CreateExtractValue(argValue->val, {i+1}));
      Value* currIndex = generateExpr(*currExpr);
      dimIndex.push_back(currIndex);
    }
  
    Value* idx = dimIndex[numDims-1];
    Value* base_offset = builder.CreateExtractValue(argInfoValue->val,numDims-1);
    Value* addr = builder.CreateAdd(idx,base_offset);
    Value* size = dimSizes[numDims-1];
    for( int i = numDims-2 ; i >= 0 ; i-- ){
      idx = dimIndex[i];
      base_offset = builder.CreateExtractValue(argInfoValue->val,i);
      idx = builder.CreateAdd(idx,base_offset);
      Value* mul = builder.CreateMul(size,idx);
      addr = builder.CreateAdd(addr,mul);
      size = builder.CreateMul(size,dimSizes[i]);
    }
    addr = builder.CreateInBoundsGEP(builder.CreateExtractValue(argValue->val, {0}), addr);
    retVal = builder.CreateLoad(addr);
  }
  return retVal;
}



Value *CGStencilFn::generateStencilOp(const stencil_op_node *expr) {
  Value* retVal = NULL;

  const std::string &paramName = expr->get_name();
  const vector_defn_node *defn = expr->get_var();
  unsigned numDims = defn->get_dim();
  const domainfn_node *domain = expr->get_scale_fn();

  ArgDesc *argValue = getArgumentByName(paramName);
  ArgDesc* argInfoValue = getArgOffsetInfo(paramName);
  assert(argValue && "Argument not found");
  const bdy_info* currBdy = getBdyCondn(paramName);
  // llvm::errs() << "[MD] CurrBdy : " << currBdy << "\n";
  assert(currBdy && "Error getting Bdy Condn");


  if (numDims == 0) {
    // If this is an access of a non-vector, just return the scalar.
    // XXX: Is this really right?  It feels weird to access scalars as
    // stencil ops in the AST.  

    //Mahesh : Yeah, all arguments to a stencil fn are of type
    // vector_defn, it is essentially seen as a vector of "0"
    // dimensions. Might not have been the ideal way of doing this.
    retVal = argValue->val;
  }
  else{
    SmallVector<Value *, 4> dimSizes;
    SmallVector<Value *, 4> base_offsets;
    for (unsigned i = 0, e = numDims; i != e; ++i) {
      dimSizes.push_back(builder.CreateExtractValue(argValue->val, {i+1}));
      // dimSizes.push_back(builder.CreateExtractValue(argValue, {2*i+1}));
      // base_offsets.push_back(builder.CreateExtractValue(argValue, {2*i+2}));
    }

    Type *i32Ty = Type::getInt32Ty(cgm.getLLVMContext());

    Value* isBdy = NULL;
    if( currBdy->type == B_CONSTANT ){
      isBdy = builder.CreateAlloca(Type::getInt1Ty(cgm.getLLVMContext()));
      builder.CreateStore(builder.getInt1(false),isBdy);
    }
  
    // Generate linearized address
    // Get last index
    Value *addr = getStencilIndex(numDims-1);
    Value *scale = ConstantInt::get(i32Ty, domain->scale_fn[numDims-1].scale);
    addr = builder.CreateMul(addr, scale);
    addr = builder.CreateAdd(addr,builder.getInt32(domain->scale_fn[numDims-1].offset));
    addr = checkBdyIndices(addr,dimSizes[numDims-1],isBdy,currBdy);
    Value* base_offset = builder.CreateExtractValue(argInfoValue->val,numDims-1);
    addr = builder.CreateAdd(addr,base_offset);
  
  
    Value *size = dimSizes[numDims-1];

    for (int i = numDims-2; i >= 0; --i) {
      Value *idx = getStencilIndex(i);
      Value *scale = ConstantInt::get(i32Ty, domain->scale_fn[i].scale);
      idx = builder.CreateMul(idx, scale );
      idx = builder.CreateAdd(idx,builder.getInt32(domain->scale_fn[i].offset));
      base_offset = builder.CreateExtractValue(argInfoValue->val,i);
      idx = builder.CreateAdd(idx,base_offset);
      idx = checkBdyIndices(idx,dimSizes[i],isBdy,currBdy);
      Value *mul = builder.CreateMul(idx, size);
      size = builder.CreateMul(size, dimSizes[i]);
      addr = builder.CreateAdd(addr, mul);
    }

    if( currBdy->type == B_CONSTANT ){
      Value* bdyVal = builder.CreateLoad(isBdy);
      BasicBlock* setConstant = BasicBlock::Create(cgm.getLLVMContext(),"setConstantBdy",thisFn);
      BasicBlock* doNormal = BasicBlock::Create(cgm.getLLVMContext(),"doNormal",thisFn);
      BasicBlock* joinBlock = BasicBlock::Create(cgm.getLLVMContext(),"joinBdy",thisFn);
      builder.CreateCondBr(bdyVal,setConstant,doNormal);

      builder.SetInsertPoint(setConstant);
      Value* retValBdy; 
      switch(defn->get_data_type().type){
      case T_DOUBLE:
	retValBdy = ConstantFP::get(Type::getDoubleTy(cgm.getLLVMContext()),currBdy->value);
	break;
      case T_FLOAT:
	retValBdy = ConstantFP::get(Type::getFloatTy(cgm.getLLVMContext()),currBdy->value);
	break;
      case T_INT:
	retValBdy = builder.getInt32(currBdy->value);
	break;
      case T_INT16:
	retValBdy = ConstantInt::get(Type::getInt16Ty(cgm.getLLVMContext()),static_cast<short>(currBdy->value));
	break;
      case T_INT8:
	retValBdy = ConstantInt::get(Type::getInt8Ty(cgm.getLLVMContext()),static_cast<uint8_t>(currBdy->value));
	break;
      case T_STRUCT:
      default:
	assert(0 && "Unhandled type for constant bdy conditions");
	retValBdy = NULL;
      }
      builder.CreateBr(joinBlock);

      builder.SetInsertPoint(doNormal);
      addr =  builder.CreateInBoundsGEP(builder.CreateExtractValue(argValue->val, {0}), addr);
      Value* retValInner = builder.CreateLoad(addr);
      builder.CreateBr(joinBlock);

      builder.SetInsertPoint(joinBlock);
      PHINode* retPHI = builder.CreatePHI(cgm.getTypeForFormaType(defn->get_data_type()),2);
      retPHI->addIncoming(retValBdy,setConstant);
      retPHI->addIncoming(retValInner,doNormal);
      retVal = retPHI;
    }
    else{
      addr =
	builder.CreateInBoundsGEP(builder.CreateExtractValue(argValue->val, {0}), addr);
      retVal = builder.CreateLoad(addr);
    }
  } 
  assert( retVal && "Unassigned return value in stencil operation");
  if( expr->get_access_field() != -1 ){
    return builder.CreateExtractValue(retVal,expr->get_access_field());
  }
  else
    return retVal;
}
 

Value *CGStencilFn::getStencilIndex(unsigned dim) {
  assert(dim < indexArgs.size() && "Out of range");
  return indexArgs[dim];
}

const Value *CGStencilFn::getStencilIndex(unsigned dim) const {
  assert(dim < indexArgs.size() && "Out of range");
  return indexArgs[dim];
}

Value *CGStencilFn::generateConstant(const expr_node *expr) {
  switch (expr->get_data_type().type) {
  case T_FLOAT: {
    float val = static_cast<const value_node<float>*>(expr)->get_value();
    return ConstantFP::get(Type::getFloatTy(cgm.getLLVMContext()), val);
  }
  case T_DOUBLE: {
    double val = static_cast<const value_node<double>*>(expr)->get_value();
    return ConstantFP::get(Type::getDoubleTy(cgm.getLLVMContext()), val);
  }
  case T_INT: {
    int val = static_cast<const value_node<int>*>(expr)->get_value();
    return ConstantInt::get(Type::getInt32Ty(cgm.getLLVMContext()),val);
  }
  case T_INT16: {
    int val = static_cast<const value_node<int>*>(expr)->get_value();
    return ConstantInt::get(Type::getInt16Ty(cgm.getLLVMContext()),(short)val);
  }
  case T_INT8: {
    int val = static_cast<const value_node<int>*>(expr)->get_value();
    return ConstantInt::get(Type::getInt8Ty(cgm.getLLVMContext()),(unsigned char)val);
  }
  default:
    assert(0 && "Unhandled constant type");
    return NULL;
  }
}

Value* CGStencilFn::generateMathFnExpr(const math_fn_expr_node* expr){
  SmallVector<Value*,4> callArgs;
  SmallVector<Type*,4> argTypes;
  std::string name = expr->get_name();
  auto fnArgs = expr->get_args();
  for( auto fnArgIter : fnArgs ){
    Value* argVal = generateExpr(fnArgIter);
    callArgs.push_back(argVal);
    argTypes.push_back(cgm.getTypeForFormaType(fnArgIter->get_data_type()));
  }
  if( name.compare("min") == 0 ){
    assert(callArgs.size() == 2 && "Min function with more than 2 arguments unsupported on code-generator");
    return generateMinCode(callArgs[0],callArgs[1]);
  }
  else if( name.compare("max") == 0 ){
    assert(callArgs.size() == 2 && "Max function with more than 2 arguments unsupported on code-generator");
    return generateMaxCode(callArgs[1],callArgs[0]);
  }
  else if( (name.compare("abs") == 0 ) ||  (name.compare("fabs") == 0 ) ){
    assert(callArgs.size() == 1 && "FloatingPt abs function with more than 2 arguments unsupported on code-generator");
    Value* zero = castToType(builder.getInt32(0),fnArgs.front()->get_data_type());
    Value* retVal;
    Value* compare;
    if( callArgs[0]->getType()->isFloatingPointTy() ){
      compare = builder.CreateFCmpULT(callArgs[0],zero);
      retVal = builder.CreateSelect(compare,builder.CreateFNeg(callArgs[0]),callArgs[0]);
    }
    else{
      compare = builder.CreateICmpSLT(callArgs[0],zero);
      retVal = builder.CreateSelect(compare,builder.CreateNeg(callArgs[0]),callArgs[0]);
    }
    return retVal;
  }
  else{
    FunctionType* fnType = FunctionType::get(cgm.getTypeForFormaType(expr->get_data_type()),argTypes,false);
    Constant* fn = cgm.getLLVMModule()->getOrInsertFunction(expr->get_name().c_str(),fnType);
    Function* fnPtr = cast<Function>(fn);
    return builder.CreateCall(fnPtr,callArgs);
  }
}



Value* CGStencilFn::generateStructExpr(const pt_struct_node* expr){
  StructType* retType = cgm.getLLVMModule()->getTypeByName(expr->get_struct_name());
  assert(retType && "Undefined LLVM type for a user defined struct");
  Value* retVal = UndefValue::get(retType);
  unsigned field_num = 0;
  for( auto fieldExpr : expr->get_field_exprs() ){
    Value* fieldVals = generateExpr(fieldExpr);
    retVal = builder.CreateInsertValue(retVal,fieldVals,field_num);
    field_num++;
  }
  return retVal;
}


Value *CGStencilFn::generateIdent(const id_expr_node *expr) {
  const std::string &name = expr->get_name();
  // Is this a local variable?
  auto it = locals.find(name);
  if (it != locals.end())
    return it->second;
  assert(0 && "Unknown S_ID found");
  return NULL;
}

Value *CGStencilFn::generateExpr(const expr_node *expr, bool checkCast) {
  Value* retVal = NULL;
  switch (expr->get_s_type()) {
  case S_VALUE:
    retVal = generateConstant(expr);
    break;
  case S_ID:
    retVal = generateIdent(static_cast<const id_expr_node*>(expr));
    break;
  case S_UNARYNEG:
    retVal = generateUnaryNeg(static_cast<const unary_neg_expr_node*>(expr));
    break;
  case S_BINARYOP:
    retVal = generateBinaryOp(static_cast<const expr_op_node *>(expr));
    break;
  case S_STENCILOP:
    retVal = generateStencilOp(static_cast<const stencil_op_node*>(expr));
    break;
  case S_TERNARY:
    retVal = generateTernaryExpr(static_cast<const ternary_expr_node*>(expr));
    break;
  case S_STRUCT:
    retVal = generateStructExpr(static_cast<const pt_struct_node*>(expr));
    break;
  case S_MATHFN:
    retVal = generateMathFnExpr(static_cast<const math_fn_expr_node*>(expr));
    break;
  case S_ARRAYACCESS:
    retVal = generateArrayAccess(static_cast<const array_access_node*>(expr));
    break;
  default:
    assert(0 && "Unhandled expression type");
    retVal = NULL;
  }
  assert(retVal && "Error in expression code gen");
  if( checkCast && expr->get_s_type() != S_STRUCT )
    return castToType(retVal,expr->get_data_type());
  else
    return retVal;
}


Value *CGStencilFn::generateUnaryNeg(const unary_neg_expr_node* expr){
  const expr_node* baseExp = expr->get_base_expr();
  Value* baseValue = generateExpr(baseExp);
  if( baseValue->getType()->isIntegerTy() )
    return builder.CreateNeg(baseValue);
  else
    return builder.CreateFNeg(baseValue);
}


Value* CGStencilFn::generateTernaryExpr(const ternary_expr_node* expr){
  BasicBlock* trueBranch = BasicBlock::Create(cgm.getLLVMContext(), "truebranch", thisFn);
  BasicBlock* falseBranch = BasicBlock::Create(cgm.getLLVMContext(), "falsebranch", thisFn);
  BasicBlock* joinBranch = BasicBlock::Create(cgm.getLLVMContext(), "joinbranch", thisFn);

  //Generate the cnditional check
  Value* condnValue = generateExpr(expr->get_bool_expr(),false);
  assert(condnValue);
  builder.CreateCondBr(condnValue,trueBranch,falseBranch);
  
  //Generate the true branch
  builder.SetInsertPoint(trueBranch);
  Value* trueValue = generateExpr(expr->get_true_expr());
  assert(trueValue);
  builder.CreateBr(joinBranch);
  BasicBlock* trueJoin = builder.GetInsertBlock();
  
  //Generate the false branch
  builder.SetInsertPoint(falseBranch);
  Value* falseValue = generateExpr(expr->get_false_expr());
  assert(falseValue);
  builder.CreateBr(joinBranch);
  BasicBlock* falseJoin = builder.GetInsertBlock();

  //Get the final value
  builder.SetInsertPoint(joinBranch);
  PHINode* joinValue = builder.CreatePHI(trueValue->getType(),2,"phival");
  joinValue->addIncoming(trueValue,trueJoin);
  joinValue->addIncoming(falseValue,falseJoin);
  
  return joinValue;
}


void CGStencilFn::generate() {
  if( is_defined )
    return;
  is_defined = true;
  // llvm::errs() << "[MD] : Generating function body for " << name << "\n";
  // Create the entry block
  BasicBlock *bb = BasicBlock::Create(cgm.getLLVMContext(), "entry", thisFn);
  builder.SetInsertPoint(bb);
  locals.clear();

  // Generate each statement node
  for (auto it = defn->get_body().begin();
       it != defn->get_body().end(); ++it) {
    pt_stmt_node *stmt = *it;
    const std::string &lhs = stmt->get_lhs();
    const expr_node *expr = stmt->get_rhs();

    Value *rhs = generateExpr(expr);
    assert(rhs && "Failed to generate expression but did not signal error");
    locals[lhs] = rhs;
  }

  // Generate the return expression
  const expr_node *retExpr = defn->get_return_expr();
  assert(retExpr && "Return is NULL?");
  Value *retVal = generateExpr(retExpr);
  builder.CreateRet(retVal);
}


Function* CGStencilFn::generateDeclaration() 
{
  StringMap<unsigned> paramIndices;

  assert(arguments.size() == 0 && "Declaration already generated?");

  assert(defn);
  
  Type *i32Ty = Type::getInt32Ty(cgm.getLLVMContext());
  unsigned numDims = defn->get_return_dim();

  // Generate index parameters if we are processing a stencil function
  for (unsigned i = 0; i != numDims; ++i) {
    arguments.push_back(new ArgDesc(i32Ty, 0, "idx"));
  }

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
    ///Add argument description
    Type* argInfoType = cgm.getInfoStructType(arg->get_dim());
    std::stringstream argInfoName;
    argInfoName << arg->get_name() << "__info__";
    arguments.push_back(new ArgDesc(argInfoType,arg->get_dim() + 1, argInfoName.str()));
  }

  // Also pass the parameters
  for (auto it = global_params->begin(); it != global_params->end(); ++it) {
    const std::string &arg_name = it->first;
    //parameter_defn *paramDef = it->second;
    arguments.push_back(new ArgDesc(i32Ty, 0, arg_name));
    paramIndices[arg_name] = arguments.size()-1;
  }

  // Determine the return type
  Type *retType;
  // If this is a stencil function, the return value is a scalar
  const data_types &type = defn->get_data_type();
  retType = cgm.getTypeForFormaType(type);
  

  SmallVector<Type *, 8> argTypes;

  // Generate the declaration
  for (ArgDesc *arg : arguments) {
    argTypes.push_back(arg->type);
  }

  FunctionType *funcTy = FunctionType::get(retType, argTypes, false);
  // llvm::errs () << "[MD] Adding new fndefn : "<< name << "\n";
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

  for (unsigned i = 0; i != numDims; ++i) {
    indexArgs.push_back(arguments[i]->val);
  }
  return thisFn;
}


const bdy_info* CGStencilFn::getBdyCondn(StringRef argName) const {
  auto fnParams = defn->get_args();
  auto argInfo = fnParams.begin();
  auto bdyInfo = bdy_condns.begin();
  assert(defn->get_args().size() == bdy_condns.size());
  for( ; argInfo != fnParams.end() ; argInfo++,bdyInfo++ ){
    if( argName.compare((*argInfo)->get_name()) == 0 ){
      // llvm::errs() << "[MD] Fn : " << name << " : Arg :" << argName.str() << ", BdyCondn: " << *bdyInfo << "\n";
      return *bdyInfo;
    }
  }
  return NULL;
}


ArgDesc* CGStencilFn::getArgOffsetInfo(StringRef name) {
  std::stringstream argInfoStream;
  argInfoStream << name.str() << "__info__";
  std::string argInfoName = argInfoStream.str();
  for( ArgDesc* desc : arguments ) {
    if( argInfoName.compare(desc->name) == 0 )
      return desc;
  }
  return NULL;
}



#endif



