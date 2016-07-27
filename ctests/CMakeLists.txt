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
enable_testing()

#find forma binaries and libraries
find_program(FORMA_EXECUTABLE forma ${FORMA_DIR}/bin)
message("-- Found Forma: " ${FORMA_EXECUTABLE})
find_library(FORMA_C_LIBRARY forma_C ${FORMA_DIR}/lib)
message("-- Found Forma C Library: " ${FORMA_C_LIBRARY})
find_library(FORMA_CUDA_LIBRARY forma_CUDA ${FORMA_DIR}/lib)
message("-- Found Forma CUDA Library: " ${FORMA_CUDA_LIBRARY})
find_library(FORMA_LLVM_LIBRARY forma_LLVM ${FORMA_DIR}/lib)
message("-- Found Forma LLVM Library: " ${FORMA_LLVM_LIBRARY})

#dev null
if( MSVC )
  set(DEVNULL "NUL")
else()
  set(DEVNULL "/dev/null")
endif()

#Add openmp flag
if(CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp")
endif()

#find cuda
find_package(CUDA)
message("-- Found CUDA_TOOLKIT :" ${CUDA_TOOLKIT_ROOT_DIR})
if( WIN32 )
  if( CMAKE_SIZEOF_VOID_P EQUAL 8)
    find_library(CUDART_LIBRARY cudart PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
    find_library(CUDA_LIBRARY cuda PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
  else()
    find_library(CUDART_LIBRARY cudart PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32)
    find_library(CUDART_LIBRARY cuda PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32)
  endif()
else()
  find_library(CUDART_LIBRARY cudart PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib64)
  find_library(CUDA_LIBRARY cuda PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib64/stubs)
endif()
message("-- Found CUDART_LIBRARY : " ${CUDART_LIBRARY})
message("-- Found CUDA_LIBRARY : " ${CUDA_LIBRARY})

#find llc
if(LLVM_INSTALL_DIR)
  set(BUILD_WITH_LLVM ON)
  find_program(LLC llc PATHS ${LLVM_INSTALL_DIR}/bin NO_DEFAULT_PATH)
  message("-- Found llc : " ${LLC})
endif()

#Add LINUX specific C flags
if( NOT WIN32 )
  set(FORMA_C_FLAGS "${FORMA_C_FLAGS} -std=gnu99")
endif()

macro (add_c_test name)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.c
    COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

    COMMAND ${FORMA_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-c
    --kernel-name ${name} --c-output ${name}.idsl.c > ${DEVNULL} 2> ${DEVNULL}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
    )
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.c
    PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} ${FORMA_C_FLAGS}")
  add_executable(${name}_C.x
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.c )
  if (CMAKE_COMPILER_IS_GNUCC)
    set_target_properties(${name}_C.x PROPERTIES
      LINK_FLAGS "${LINK_FLAGS} -fopenmp")
  endif()
  target_link_libraries(${name}_C.x ${FORMA_C_LIBRARY})
  add_test(${name}_C ${name}_C.x )
endmacro(add_c_test)

macro (add_gpu_test name)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.cu
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

    COMMAND ${FORMA_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-cuda
    --kernel-name ${name} --cuda-output ${name}.idsl.cu >${DEVNULL} 2>${DEVNULL}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
    )
  cuda_add_executable(${name}_CUDA.x
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.cu )
  target_link_libraries(${name}_CUDA.x ${FORMA_CUDA_LIBRARY} ${CUDART_LIBRARY} )
  add_test(${name}_CUDA ${name}_CUDA.x )
endmacro(add_gpu_test name)

macro(add_gpu_texture_test name)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.texture.cu
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

    COMMAND ${FORMA_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-cuda
    --use-texture --kernel-name ${name} --cuda-output ${name}.idsl.texture.cu
    > ${DEVNULL} 2> ${DEVNULL}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
    )
  cuda_add_executable(${name}_tex_CUDA.x
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.texture.cu )
  target_link_libraries(${name}_tex_CUDA.x
    ${FORMA_CUDA_LIBRARY} ${CUDART_LIBRARY})
  add_test(${name}_tex_CUDA ${name}_tex_CUDA.x )
endmacro(add_gpu_texture_test)

macro(add_llvm_test name)
  if(BUILD_WITH_LLVM)
    if( MSVC )
      add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.s
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
        ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

        COMMAND ${FORMA_EXECUTABLE}
        ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-llvm
        --kernel-name ${name} --llvm-output ${name}.idsl.ll
        >${DEVNULL} 2>${DEVNULL}

        COMMAND ${LLC}
        ARGS -filetype=obj -o ${name}.idsl.s ${name}.idsl.ll

        MAIN_DEPENDENCY
        ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
        )
    else()
      add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl.s
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
        ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

        COMMAND ${FORMA_EXECUTABLE}
        ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-llvm
        --kernel-name ${name} --llvm-output ${name}.idsl.ll
        > ${DEVNULL} 2> ${DEVNULL}

        COMMAND ${LLC} ARGS -o ${name}.idsl.s ${name}.idsl.ll

        MAIN_DEPENDENCY
        ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
        )
    endif()
    set_source_files_properties(${name}.idsl.s PROPERTIES EXTERNAL_OBJECT TRUE)
    add_executable(${name}_LLVM.x
      ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp ${name}.idsl.s )
    target_link_libraries(${name}_LLVM.x ${FORMA_LLVM_LIBRARY} )
    if(MSVC)
      set_target_properties(${name}_LLVM.x
        PROPERTIES LINK_FLAGS "-defaultlib:libcmt /FORCE:MULTIPLE")
    endif()
    add_test(${name}_LLVM ${name}_LLVM.x )
  endif()
endmacro(add_llvm_test)

macro (add_c_unroll_test name)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.unroll.idsl.c
    COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

    COMMAND ${FORMA_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-c
    --kernel-name ${name} --unroll-factors 6,4 --c-output ${name}.unroll.idsl.c > ${DEVNULL} 2> ${DEVNULL}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
    )
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${name}.unroll.idsl.c
    PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} ${FORMA_C_FLAGS}")
  add_executable(${name}_C_unroll.x
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.unroll.idsl.c )
  if (CMAKE_COMPILER_IS_GNUCC)
    set_target_properties(${name}_C_unroll.x PROPERTIES
      LINK_FLAGS "${LINK_FLAGS} -fopenmp")
  endif()
  target_link_libraries(${name}_C_unroll.x ${FORMA_C_LIBRARY})
  add_test(${name}_C_unroll ${name}_C_unroll.x )

endmacro(add_c_unroll_test)

macro (add_gpu_unroll_test name)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.unroll.idsl.cu
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.idsl
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl

    COMMAND ${FORMA_EXECUTABLE}
    ARGS ${CMAKE_CURRENT_BINARY_DIR}/${name}.idsl --init-zero --print-cuda
    --kernel-name ${name} --unroll-factors 6,4 --cuda-output ${name}.unroll.idsl.cu >${DEVNULL} 2>${DEVNULL}
    MAIN_DEPENDENCY ${CMAKE_CURRENT_LIST_DIR}/${name}.idsl ${FORMA_EXECUTABLE}
    )
  cuda_add_executable(${name}_CUDA_unroll.x
    ${CMAKE_CURRENT_SOURCE_DIR}/${name}.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/${name}.unroll.idsl.cu )
  target_link_libraries(${name}_CUDA_unroll.x ${FORMA_CUDA_LIBRARY} ${CUDART_LIBRARY} )
  add_test(${name}_CUDA_unroll ${name}_CUDA_unroll.x )

endmacro(add_gpu_unroll_test name)

set(TEST_FILES
  hello_world
  simple_stencil
  bdy_wrap
  bdy_clamped
  bdy_mirror
  bdy_constant
  mixed_bdy_NW
  mixed_bdy_CM
  stencil_sampling
  sample
  ternary
  extrapolate
  offset
  compose
  downsample
  upsample
  try_struct
  subdomain_function
  struct_subdomain
  vectorfn_subdomain
  blur_double
  blur_float
  blur_int
  blur_int8
  blur_int16
  blur_mirror
  canny_mirror
  downsample_mirror
  upsample_mirror
  getmax
  canny
  hdr_direct
  hello_world3D
  hello_world4D
  )

foreach(test ${TEST_FILES})
  add_c_test(${test})
  add_gpu_test(${test})
  add_llvm_test(${test})
endforeach(test)

set(TEST_FILES
  hello_world
  simple_stencil
  bdy_clamped
  bdy_constant
  stencil_sampling
  sample
  ternary
  extrapolate
  offset
  compose
  downsample
  upsample
  try_struct
  subdomain_function
  struct_subdomain
  vectorfn_subdomain
  blur_double
  blur_float
  blur_int
  blur_int8
  blur_int16
  upsample_mirror
  getmax
  canny
  hdr_direct
  hello_world3D
  hello_world4D
  )

foreach(test ${TEXTURE_TESTS_FILES})
  add_gpu_texture_test(${test})
endforeach(test)

set(UNROLL_TESTS
  blur_float
  canny
  downsample
  upsample
  blur_mirror
  downsample_mirror
  upsample_mirror
)
foreach(test ${UNROLL_TESTS})
  add_c_unroll_test(${test})
  add_gpu_unroll_test(${test})
endforeach(test)