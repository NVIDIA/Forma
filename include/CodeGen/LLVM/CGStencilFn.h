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
#ifndef CGSTENCILFN_H
#define CGSTENCILFN_H

#include "CGFunction.h"
#include "llvm/Support/raw_ostream.h"

class CGStencilFn : public CGFunction {

 public: 

 CGStencilFn(CGModule &GCM, const stencilfn_defn_node* fn , llvm::StringRef& fnName, const std::deque<bdy_info*>& curr_bdy_info):
  CGFunction(GCM),
    builder(cgm.getLLVMContext()),
    is_defined(false),
    defn(fn),
    name(fnName.str()){
    /* llvm::errs() << "[MD] : Fn : " << name << "(" << this << ")" << " Bdys :" ; */
    for( auto iterator : curr_bdy_info ){
      /* llvm::errs() << " " << iterator ; */
      bdy_condns.push_back(iterator);
    }
    /* llvm::errs() << "\n"; */
  }
  
  ~CGStencilFn() {
    for( auto iterator : bdy_condns)
      delete iterator;
    bdy_condns.clear();
  }

  void generate();

  llvm::Function* generateDeclaration();

 private:
  
  /// IR builder pointing to the current codegen cursor
  llvm::IRBuilder<> builder;

  bool is_defined;

  const stencilfn_defn_node* defn;  

  const std::string name;

  /// Map from local variable name to its Value
  llvm::StringMap<llvm::Value *> locals;

  /// The boundary conditions to be used for each function
  std::deque<bdy_info*> bdy_condns;

  // Expression helpers, implemented in CGExpr.cpp
  llvm::Value *generateExpr(const expr_node *expr, bool checkCast = true );
  llvm::Value *generateConstant(const expr_node *expr);
  llvm::Value *generateIdent(const id_expr_node *expr);
  llvm::Value *generateBinaryOp(const expr_op_node *expr);
  llvm::Value *generateStencilOp(const stencil_op_node *expr );
  llvm::Value* checkBdyIndices(llvm::Value* addr, llvm::Value* size, llvm::Value* isBdy, const bdy_info* bdyCondn);
  llvm::Value *generateUnaryNeg(const unary_neg_expr_node *expr);
  llvm::Value *generateTernaryExpr(const ternary_expr_node * expr );
  llvm::Value* generateStructExpr(const pt_struct_node* expr);
  llvm::Value* generateMathFnExpr(const math_fn_expr_node* expr);
  llvm::Value* generateArrayAccess(const array_access_node* expr);

  ///Function to perform appropriate casting
  void checkTypes(llvm::Value*&,llvm::Value*&);
  llvm::Value* castToType(llvm::Value* currVal, const data_types& currType);

  ///Function to generate min/max checks
  llvm::Value* generateMinCode(llvm::Value*,llvm::Value*);
  llvm::Value* generateMaxCode(llvm::Value*,llvm::Value*);

  llvm::Value *getStencilIndex(unsigned dim);
  const llvm::Value *getStencilIndex(unsigned dim) const;

  /// List of indices for stencil functions
  llvm::SmallVector<llvm::Value *, 4> indexArgs;

  /// Get the bdy condition for an argument
  const bdy_info* getBdyCondn(llvm::StringRef) const;
  
  /// Get the arg offset information
  ArgDesc* getArgOffsetInfo(llvm::StringRef) ;

  
};

#endif 


#endif
