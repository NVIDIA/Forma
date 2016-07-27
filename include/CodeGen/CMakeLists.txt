#****************************************************************************#
#* Copyright (c) 2016, NVIDIA CORPORATION. All rights reserved.             *#
#*                                                                          *#
#* Redistribution and use in source and binary forms, with or without       *#
#* modification, are permitted provided that the following conditions       *#
#* are met:                                                                 *#
#*  * Redistributions of source code must retain the above copyright        *#
#*    notice, this list of conditions and the following disclaimer.         *#
#*  * Redistributions in binary form must reproduce the above copyright     *#
#*    notice, this list of conditions and the following disclaimer in the   *#
#*    documentation and/or other materials provided with the distribution.  *#
#*  * Neither the name of NVIDIA CORPORATION nor the names of its           *#
#*    contributors may be used to endorse or promote products derived       *#
#*    from this software without specific prior written permission.         *#
#*                                                                          *#
#* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY     *#
#* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE        *#
#* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR       *#
#* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR        *#
#* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,    *#
#* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,      *#
#* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR       *#
#* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY      *#
#* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT             *#
#* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE    *#
#* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.     *#
#****************************************************************************#
list (APPEND
  CODEGEN_HEADER_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/CodeGen.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/print_C.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/print_CUDA.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/print_dot.hpp
)

if(BUILD_WITH_LLVM)
  list(APPEND
    CODEGEN_HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LLVM/CGModule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LLVM/CGFunction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LLVM/CGStencilFn.h
    )
endif()

set(CODEGEN_HEADERS ${CODEGEN_HEADER_FILES} PARENT_SCOPE)