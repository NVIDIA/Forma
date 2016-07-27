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
#ifndef CGFUNCTION_H
#define CGFUNCTION_H

#include "AST/parser.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/StringMap.h"
#include <memory>
#include "program_opts.hpp"

namespace llvm {
class Module;
}

class CGModule;


/// ArgDesc - Descriptor for function arguments
struct ArgDesc {
  llvm::Type *type;
  unsigned dims;
  std::string name;
  llvm::Value *val;

ArgDesc(llvm::Type *t, unsigned d, const std::string &name)
: type(t), dims(d), name(name), val(NULL) {
  assert(type != NULL && "Argument must have a type!");
  assert(name.size() > 0 && "Argument must have a name!");
}
};

///Temporary variable info that including sub-domains of access and
/// fields for structs
struct TempVarInfo{
  ArgDesc* arg;
  const domain_node* sub_domain;
  int access_field_num;
TempVarInfo(ArgDesc* a, const domain_node* sd = NULL, int field_num= -1) :
  arg(a), sub_domain(sd),access_field_num(field_num) { }

};


class CGFunction{
 public:
 CGFunction(CGModule &CGM):
  cgm(CGM),  thisFn(NULL)  { }
  
  /* virtual void generate() = 0; */

  virtual ~CGFunction() { }

 protected:

  CGModule &cgm;

  /// The function we are generating
  llvm::Function *thisFn;

  /// Function argument list. This will be filled in by generateDeclaration
  llvm::SmallVector<ArgDesc*, 8> arguments;

  ArgDesc *getArgumentByName(llvm::StringRef name);
  const ArgDesc *getArgumentByName(llvm::StringRef name) const;
};



class CGVectorFn : public CGFunction {
public:
  CGVectorFn(CGModule &CGM, const vectorfn_defn_node *fn, bool topLevel, bool set_zero);

  /* void generate(ArgDesc* outputVar, const domain_desc_node* outputIdxInfo = NULL); */
  
  ///Main function to process the forma program, called only on the topLevel function
  llvm::Function* generateDeclaration();

  ~CGVectorFn() {   }

  /// IR builder pointing to the current codegen cursor
  static llvm::IRBuilder<>* builder;

 private:

  ///Function to initialize parameters
  void InitParams(const llvm::StringMap<llvm::Value*>& paramList);

  /// Map from local variable name to its Value
  llvm::StringMap<ArgDesc *> locals;

  //void generateStencilFunction(const stencilfn_defn_node *fnDef);
  void generateVectorFunction(const vectorfn_defn_node *fnDef, ArgDesc* outputVar,
			      const domain_desc_node* outputIdxInfo = NULL);

  // Function to allocate a variable
  /* ArgDesc* allocateDomain(const vector_expr_node* expr, const std::string& name); */
  const bool init_zero;
  ArgDesc* allocateDomain(const domain_node* domain, const data_types& type,
			  const std::string& name);

  // Process constant scalar expressiosn
  ArgDesc *generateConstant(const expr_node* expr);
  // Function to process vector expressions when output var is not
  // specified, allocates a new buffer
  ArgDesc* generateVectorExpr(const vector_expr_node *expr);
  // Function to process vector expression using preallocated output buffers
  void generateVectorExpr(const vector_expr_node* expr, ArgDesc* outputArg, 
			  const domain_desc_node* outputIdxInfo = NULL);
  

  //Function to handled simple assignment statements
  void generateSimpleAssignment(const vector_expr_node* domain, ArgDesc* rhsVar,
				ArgDesc* outputVar, const domain_desc_node* outputIdxInfo);
  
  ///Function to handled stencil functions
  void generateStencilFnExpr(const fnid_expr_node *expr, ArgDesc* outputVar, 
			     const domain_desc_node* outputIdxInfo);

  ///Function to handle compose expressions
  void generateComposeExpr(const compose_expr_node* expr, ArgDesc* outputArg);
  void generateMakeStruct(const make_struct_node* expr, ArgDesc* outputArg, 
			  const domain_desc_node* outputIdxInfo);

  ///Function to handle vector fn calls
  void generateVectorFnExpr(const fnid_expr_node* expr, ArgDesc* outputArg,
			    const domain_desc_node* outputIdxInfo);

  //Function to handle vector interpolations
  void generateVecScaleExpr(const vec_domainfn_node* expr, ArgDesc* outputArg,
			    const domain_desc_node* outputIdxInfo);
  
  /// Handle vecID expression
  ArgDesc *generateVectorIdExpr(const vec_id_expr_node *expr);

  llvm::Value *generateParametricExpr(const parametric_exp *pexp);

  llvm::Value *getArgumentByIndex(unsigned index);
  const llvm::Value *getArgumentByIndex(unsigned index) const;

  void computeBdyPatches(const fnid_expr_node*fn, 
			 std::deque<offset_hull> &offsets,
			 int currDim, int numDims,
			 std::deque<std::pair<llvm::Value*,bool> > * patches,
			 const domain_node* domain,
			 const domainfn_node* outputIdxInfo) ;
  
  void generateBdyCode( const fnid_expr_node* fn,
			std::deque<std::pair<llvm::Value*,bool> >* patches,
			std::deque<std::pair<llvm::Value*,llvm::Value*> >& loopBounds,
			int currDim, int numDims, bool isBdy,
			llvm::StringRef fnName,
			llvm::SmallVector<TempVarInfo*,4>& fnArgVals,
			ArgDesc* outputVar,
			const domain_desc_node* outputIdxInfo );

  /* void checkTypes(llvm::Value*&,llvm::Value*&); */

  // Function to generate Loop headers/footers
  llvm::BasicBlock* generateLoopHeaderFooters(std::deque<std::pair<llvm::Value*,llvm::Value*> >& loopBounds,
					      llvm::SmallVector<llvm::Value*,4>& iterators,
					      llvm::BasicBlock* exitBloc, llvm::BasicBlock* innerBB);


  /// The Forma AST function we are generating
  const vectorfn_defn_node *defn;

  /// Mapping from parameter name to parameter value for the current function
  llvm::StringMap<llvm::Value *> paramMap;

  /// Is this a top-level Forma function?
  bool isTopLevel;
  
  /// Name for temporary domains
  std::string getTemporaryName(){
    static int temp_num = 0;
    std::stringstream name_stream;
    name_stream << "__temp__" << temp_num++ ;
    return name_stream.str();
  }

  /// Helper function to generate loop bounds
  void generateLoopBounds(const domain_node* domain, 
			  std::deque<std::pair<llvm::Value*,llvm::Value*> >& loopBounds,
			  const domainfn_node* outputIdxInfo = NULL,
			  const domainfn_node* inputScaleInfo = NULL);
  
  /// Helper function to generate domain sizes
  void generateDomainSize(const domain_node* domain,
		     llvm::SmallVector<llvm::Value*,4>& dimSizes);

  ///Helper function to generate code for the RHS array access
  llvm::Value* generateRHSArrayAccess(const llvm::SmallVector<llvm::Value*,4>& iterators,
				      TempVarInfo& rhsVar);

  ///Helper function to generate code for the LHS array access
  llvm::Value* generateLHSArrayAccess(const llvm::SmallVector<llvm::Value*,4>& iterators,
				      ArgDesc* rhsVar, const domain_desc_node* outputIdxInfo);


};


#endif
#endif

