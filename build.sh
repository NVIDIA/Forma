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
#!/bin/bash
set -e

if [ ! -d build ]
then
    mkdir build
fi

read -p "Build Type (Debug/Release)? " FORMA_BUILD_TYPE

if [ -x $CC ]
then
    CC=gcc
fi

if [ -x $CXX ]
then
    CXX=g++
fi

FORMA_ROOT=$(dirname $0)
if [[ ${FORMA_ROOT} != /* ]]
then
    FORMA_ROOT=`pwd`/${FORMA_ROOT}
fi
FORMA_BUILD_DIR=${FORMA_ROOT}/build/${FORMA_BUILD_TYPE}

if [ ! -f "${FORMA_BUILD_DIR}/CMakeCache.txt" ]
then
    read -p "Forma Installation Directory (${FORMA_ROOT}/${FORMA_BUILD_TYPE}) ? " -e FORMA_INSTALL_DIR
    if [ -z "${FORMA_INSTALL_DIR}" ]
    then
	FORMA_INSTALL_DIR=${FORMA_ROOT}/${FORMA_BUILD_TYPE}
    fi

    echo "Build type : ${FORMA_BUILD_TYPE}"

    read -p "CUDA Toolkit Directory (/usr/local/cuda) ?" -e CUDA_TOOLKIT_ROOT_DIR
    if [ -z "${CUDA_TOOLKIT_ROOT_DIR}" ]
    then
	CUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
    fi

    if [ ! -d ${FORMA_BUILD_DIR} ]
    then
	mkdir -p ${FORMA_BUILD_DIR}
    fi
    cd ${FORMA_BUILD_DIR}
    
    read -p "Enable LLVM backend [y/n(default)] ? " USE_LLVM
    if [ -z "${USE_LLVM}" ]
    then
	USE_LLVM="n"
    fi
    if [ ${USE_LLVM} == "y" ]
    then
	read -p "LLVM installation (>=3.8) ? " -e LLVM_DIR
	if [ -z "${LLVM_DIR}" ]
	then
	    echo "Need LLVM installation while using LLVM"
	    exit 1
	fi

	cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_BUILD_TYPE=${FORMA_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${FORMA_INSTALL_DIR} -DLLVM_INSTALL_DIR=${LLVM_DIR} -DCUDA_TOOLKIT_ROOT_DIR=${CUDA_TOOLKIT_ROOT_DIR} ${FORMA_ROOT}
    else
	cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_BUILD_TYPE=${FORMA_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=${FORMA_INSTALL_DIR} -DCUDA_TOOLKIT_ROOT_DIR=${CUDA_TOOLKIT_ROOT_DIR} ${FORMA_ROOT}
    fi
    
    if [ $? -ne 0 ]
    then
    	echo "CMake failed while building Forma"
    	exit $?
    fi
else
    cd ${FORMA_BUILD_DIR}
    FORMA_INSTALL_DIR=`grep "CMAKE_INSTALL_PREFIX" CMakeCache.txt | head -n 1 | awk -F= '{print $2}'`
    CUDA_TOOLKIT_ROOT_DIR=`grep "CUDA_TOOLKIT_ROOT_DIR" CMakeCache.txt | head -n 1 | awk -F= '{print $2}'`
    LLVM_DIR=`grep "LLVM_INSTALL_DIR" CMakeCache.txt | headn -n 1 | awk -F= '{print $2}'`
    if [ ! -z "${LLVM_DIR}" ]
    then
	USE_LLVM="y"
    else
	USE_LLVM="n"
    fi
fi
    
make install
if [ $? -ne 0 ]
then
    echo "make forma failed"
    exit $?
fi

cd ${FORMA_ROOT}

read -p "Build Test [y/n] ? " BUILD_TEST
if [ ${BUILD_TEST} == "y" ]
then
    . ./build_test.sh
fi

echo "USE_LLVM " ${USE_LLVM}
read -p "Build Experiments [y/n] ? "  BUILD_EXPERIMENTS
if [ ${BUILD_EXPERIMENTS} == "y" ]
then
    . ./build_experiments.sh
fi
