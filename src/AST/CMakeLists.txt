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
find_program(FLEX flex)
if (NOT FLEX)
  message(FATAL_ERROR "flex not found. Please add path to flex to PATH")
endif()
execute_process(
  COMMAND ${FLEX} -o lex.yy.c tokenize.lex
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

find_program(BISON bison)
if (NOT BISON)
  message(FATAL_ERROR "bison not found. Please add path to bison to PATH")
endif()
execute_process(
  COMMAND ${BISON} -d -b parser parser.y
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set(AST_SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/data_types.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/domain.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fn_defn.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lex.yy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/parametric.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/parser.tab.c
  ${CMAKE_CURRENT_SOURCE_DIR}/print_node.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pretty_print.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/stencil_expr.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/symbol_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/vector_expr.cpp
  PARENT_SCOPE)


