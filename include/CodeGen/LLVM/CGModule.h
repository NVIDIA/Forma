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
#ifndef CGMODULE_H
#define CGMODULE_H

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"
#include "AST/parser.hpp"
#include <memory>
#include "program_opts.hpp"
#include "CodeGen/CodeGen.hpp"

namespace llvm {
class Module;
}


class CGVectorFn;
class CGStencilFn;

class CGModule : public CodeGen {
public:
  CGModule();

  void GenerateCode(const program_node* program, const program_options& opts );

  llvm::Module *getLLVMModule() {
    return mod.get();
  }
  const llvm::Module *getLLVMModule() const {
    return mod.get();
  }

  llvm::LLVMContext &getLLVMContext() {
    return ctx;
  }
  const llvm::LLVMContext &getLLVMContext() const {
    return ctx;
  }

  // Utility functions
  llvm::Type *getTypeForFormaType(const data_types &type);
  llvm::Type *getTypeForFormaBasicType(basic_data_types type);
  int getSizeOfType(const data_types& type);
  llvm::Type *getVectorStructType(const data_types &type, unsigned numDims);
  llvm::Type *getInfoStructType(unsigned numDims);


  //Add a stencil function with given boundary conditions
  void addStencilFunction(llvm::StringRef& fnName, const stencilfn_defn_node* fn,
			  std::deque<bdy_info*> curr_bdy_info);
  //Add a stencil function with default boundary conditions
  void addStencilFunction(llvm::StringRef& fnName, const stencilfn_defn_node* fn);

  //Add a vector function to the mix
  CGVectorFn* addVectorFunction(llvm::StringRef& fnName, const vectorfn_defn_node* fn);

  std::string getBdyInfoFnName(const stencilfn_defn_node* fm,
			       const std::deque<bdy_info*> curr_bdy_info);

private:
  
  bool init_zero;

  void generateWrapper(const std::string& kernel_name, const vectorfn_defn_node* defn);

  llvm::LLVMContext ctx;
  std::unique_ptr<llvm::Module> mod;

  llvm::StringMap<CGStencilFn*> stencil_fns;
  
  llvm::StringMap<CGVectorFn*> vector_fns;

  llvm::Value* getArgumentByIndex(llvm::Function* thisFn, unsigned index);
  const llvm::Value* getArgumentByIndex(llvm::Function* thisFn, unsigned index) const;

  llvm::Value* generateParametricExpr(const parametric_exp* pexp, llvm::IRBuilder<>& builder,
				      llvm::StringMap<llvm::Value*>& paramMap);
};


#endif
#endif

