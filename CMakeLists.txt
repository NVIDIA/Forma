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
cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

set( project_name "forma" )
message("-- Building " ${project_name})

project(${project_name})

# ENABLE_TESTING()
include_directories("${CMAKE_SOURCE_DIR}")

#FIXME : might not need this anymore - but leaving it there for now
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -DHEADER_STRING_DIR=${CMAKE_SOURCE_DIR}")

#get the CUDA package to get the CUDA-CMAKE macros
find_package(CUDA)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include/)


# Build with LLVM (optional)
option(BUILD_WITH_LLVM "Build with LLVM support" NO)

if (LLVM_INSTALL_DIR)
  set(BUILD_WITH_LLVM ON)
  set(LLVM_DIR "${LLVM_INSTALL_DIR}/share/llvm/cmake/")
  find_package(LLVM REQUIRED)
  include_directories(SYSTEM ${LLVM_INSTALL_DIR}/include)
  set(LLVM_LIB_DIR ${LLVM_INSTALL_DIR}/lib)
  message("-- Found LLVM_LIBS in: " ${LLVM_LIB_DIR})

  #FIXME : Get rid of this as well
  add_definitions(-DFORMA_USE_LLVM)

endif()

#FIXME : change test infrastructure - need to seperate it out
add_custom_target(build_test COMMAND ${CMAKE_CTEST_COMMAND})

add_subdirectory(include)
add_subdirectory(src)

#Forma srcs and headers
set(FORMA_SRCS ${AST_SRCS} ${AST_VISITOR_SRCS} ${CODEGEN_SRCS} ${AUX_SRCS})
set(FORMA_HEADERS
  ${AST_HEADERS} ${AST_VISITOR_HEADERS} ${CODEGEN_HEADERS} ${AUX_HEADERS})

#set lex and bison generated files as c++ files
SET_SOURCE_FILES_PROPERTIES(${CMAKE_SOURCE_DIR}/src/AST/parser.tab.c
  ${CMAKE_SOURCE_DIR}/src/AST/lex.yy.c PROPERTIES LANGUAGE CXX)

#The forma executable
add_executable(${project_name} ${FORMA_SRCS} ${FORMA_HEADERS})

if (BUILD_WITH_LLVM)
  llvm_map_components_to_libnames(LLVM_LIB_NAMES ipo analysis BitWriter)
  target_link_libraries(${project_name} ${LLVM_LIB_NAMES} ${NVLLVMAPI_LIB})
endif()

# FIXME : Change _WINDOWS_ to be _WINDOWS
if (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WINDOWS_")
endif()


#Only while using LLVM do we need c++11
if(NOT MSVC )
  set_target_properties(${project_name} PROPERTIES
    COMPILE_FLAGS "-std=c++11")
endif()

# Libraries that contain some helper information for C/Cuda and LLVM
add_library(${project_name}_C c_header.c)
cuda_add_library(${project_name}_CUDA cuda_header.cu)
if( BUILD_WITH_LLVM )
  add_library(${project_name}_LLVM llvm_helper.cpp)
endif()

#install buildt stuff
INSTALL(TARGETS ${project_name} DESTINATION bin)
INSTALL(TARGETS ${project_name}_C ${project_name}_CUDA  DESTINATION lib)
if( BUILD_WITH_LLVM )
  INSTALL(TARGETS ${project_name}_LLVM DESTINATION lib)
endif()
