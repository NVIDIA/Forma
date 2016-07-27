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

#include "program_opts.hpp"
#include "AST/parser.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"


/// Base class for LLVM-based targets
class PrintLLVM {
public:
  PrintLLVM();
  virtual ~PrintLLVM();

  virtual void print_program(const program_node *, const program_options &) = 0;


protected:

  /// Context for the LLVM code generator
  struct CGContext {
    CGContext(llvm::LLVMContext &ctx)
      : builder(ctx) {}

    llvm::IRBuilder<> builder;
  };


  llvm::Type *getTypeForFormaType(const data_types &type);
  llvm::LLVMContext &getLLVMContext() { return Ctx; }
  const llvm::LLVMContext &getLLVMContext() const { return Ctx; }

  llvm::Value *generateExpr(const expr_node *expr,
                            CGContext &context);
  llvm::Value *generateConstant(const expr_node *expr,
                                CGContext &context);
  llvm::Value *generateBinaryOp(const expr_op_node *expr,
                                CGContext &context);
  llvm::Value *generateStencilOp(const stencil_op_node *expr,
                                 CGContext &context);

private:

  llvm::LLVMContext Ctx;

};

#endif
