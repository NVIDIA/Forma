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

if [ ! -d ${FORMA_ROOT}/build ]
then
    mkdir ${FORMA_ROOT}/build
fi


if [ -z "${FORMA_BUILD_TYPE}" ]
then
    read -p "Build Type (Debug/Release)? " FORMA_BUILD_TYPE
fi

if [ ! -d ${FORMA_ROOT}/build/${FORMA_BUILD_TYPE} ]
then 
    mkdir ${FORMA_ROOT}/build/${FORMA_BUILD_TYPE}
fi


if [ -z "${FORMA_INSTALL_DIR}" ]
then
    read -p "Forma Installation Directory (${FORMA_ROOT}/${FORMA_BUILD_TYPE})) ? " -e FORMA_INSTALL_DIR
    if [ -z "${FORMA_INSTALL_DIR}" ]
    then
	FORMA_INSTALL_DIR=${FORMA_ROOT}/${FORMA_BUILD_TYPE}
    fi
fi

PREV_LD_LIBRARY_PATH="${LD_LIBRARY_PATH}"
if [ -z "${CUDA_TOOLKIT_ROOT_DIR}" ]
then
    read -p "CUDA Toolkit Root Directory (/usr/local/cuda) ? " -e CUDA_TOOLKIT_ROOT_DIR
    if [ -z "${CUDA_TOOLKIT_ROOT_DIR}" ]
    then
	CUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
    fi
fi
export LD_LIBRARY_PATH="${CUDA_TOOLKIT_ROOT_DIR}/lib64:${LD_LIBRARY_PATH}"
    
if [ -z "${USE_LLVM}" ]
then
    read -p "Enable LLVM backend [y/n] ? " USE_LLVM
    if [ ${USE_LLVM} == "y" ]
    then
	read -p "LLVM installation dir (>=3.8) ? " -e LLVM_DIR
	if [ -z "${LLVM_DIR}" ]
	then
	    echo "Need LLVM installation while using LLVM"
	    exit 1
	fi
    fi
fi

TEST_DIR=${FORMA_ROOT}/build/${FORMA_BUILD_TYPE}/tests
if [ ! -d ${TEST_DIR} ]
then 
    mkdir ${TEST_DIR}
    cd ${TEST_DIR}

    if [ ${USE_LLVM} == "y" ]
    then
	cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_BUILD_TYPE=${FORMA_BUILD_TYPE} -DFORMA_DIR=${FORMA_INSTALL_DIR} -DLLVM_INSTALL_DIR=${LLVM_DIR} -DCUDA_TOOLKIT_ROOT_DIR=${CUDA_TOOLKIT_ROOT_DIR} ${FORMA_ROOT}/ctests
    else
	cmake -DCMAKE_C_COMPILER=${CC} -DCMAKE_CXX_COMPILER=${CXX} -DCMAKE_BUILD_TYPE=${FORMA_BUILD_TYPE} -DFORMA_DIR=${FORMA_INSTALL_DIR} -DCUDA_TOOLKIT_ROOT_DIR=${CUDA_TOOLKIT_ROOT_DIR} ${FORMA_ROOT}/ctests
    fi
    if [ $? -ne 0 ]
    then
	echo "CMake tests failed"
	exit $?
    fi
else
    cd ${TEST_DIR}
fi

make
if [ $? -ne 0 ]
then
    echo "build tests failed"
    exit $?
fi

make test || true
export LD_LIBRARY_PATH="${PREV_LD_LIBRARY_PATH}"

cd ${FORMA_ROOT}



