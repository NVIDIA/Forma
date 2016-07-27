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
cmake_minimum_required(VERSION 2.8.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_SOURCE_DIR}/../cmake/Modules/")

#find forma binaries and executable
find_program(FORMA_EXECUTABLE forma ${FORMA_DIR} ${FORMA_DIR}/bin)
message(STATUS "Found Forma : " ${FORMA_EXECUTABLE})
find_library(FORMA_C_LIBRARY forma_C ${FORMA_DIR} ${FORMA_DIR}/lib)
message(STATUS "Found Forma C LIbrary : " ${FORMA_C_LIBRARY})
find_library(FORMA_CUDA_LIBRARY forma_CUDA ${FORMA_DIR} ${FORMA_DIR}/lib)
message(STATUS "Found Forma C LIbrary : " ${FORMA_CUDA_LIBRARY})

#dev null
if( MSVC )
  set(DEVNULL "NUL")
else()
  set(DEVNULL "/dev/null")
endif()

# Disgusting workaround for pgcc :( - for now, no cuda builds when using pgcc or
# icpc
if(${CMAKE_CXX_COMPILER} MATCHES "pgCC" OR ${CMAKE_CXX_COMPILER} MATCHES "icpc")
  add_definitions(-D__SWBLK_T_TYPE=int -D__syscall_slong_t=long )
else()
  #CUDA
  set(CUDA_PROPAGATE_HOST_FLAGS OFF)
  find_package(CUDA)
  message(STATUS "Found CUDA_TOOLKIT :" ${CUDA_TOOLKIT_ROOT_DIR})
  if( WIN32 )
    if( CMAKE_SIZEOF_VOID_P EQUAL 8)
      find_library(CUDART_LIBRARY cudart PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
      find_library(CUDA_LIBRARY cuda PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64)
    else()
      find_library(CUDART_LIBRARY cudart
        PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32)
      find_library(CUDART_LIBRARY cuda PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib/Win32)
    endif()
  else()
    find_library(CUDART_LIBRARY cudart PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib64)
    find_library(CUDA_LIBRARY cuda PATHS ${CUDA_TOOLKIT_ROOT_DIR}/lib64/stubs)
  endif()
  message(STATUS "Found CUDART_LIBRARY : " ${CUDART_LIBRARY})
  message(STATUS "Found CUDA_LIBRARY : " ${CUDA_LIBRARY})
  get_filename_component(CUDART_PATH ${CUDART_LIBRARY} DIRECTORY)
  message(STATUS "CUDART Path : " ${CUDART_PATH})

  if (CUDA_FOUND)
    add_definitions(-DHAVE_CUDA)
  endif()

endif()

#OpenMP flags
find_package(OpenMP)
if( OPENMP_FOUND)
  message(STATUS "OpenMP_C_FLAGS : " ${OpenMP_C_FLAGS})
  message(STATUS "OpenMP_CXX_FLAGS : " ${OpenMP_CXX_FLAGS})
  set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif( OPENMP_FOUND )

# Override OpenMP flag for pgCC (CMake does not know how to handle OpenMP with
# pgcc)
if (${CMAKE_CXX_COMPILER} MATCHES "pgCC")
  add_definitions(-mp)
endif()

if( CUDA_FOUND )
  if( NOT CUDA_NVCC_FLAGS)
    set(CUDA_NVCC_FLAGS "-arch=sm_20")
  endif( NOT CUDA_NVCC_FLAGS)
  include_directories(${CUDA_INCLUDE_DIRS})
endif()

#set TIMER ON
set(FORMA_USE_TIMER OFF CACHE BOOL "Use built-in timer")
if (FORMA_USE_TIMER)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_TIMER_")
  set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};-D__CUDA_PROFILE__")
endif()


#fined Likwid
if ( LIKWID_DIR )
  include_directories(${LIKWID_DIR}/include)
  find_library(LIKWID_LIBRARIES likwid ${LIKWID_DIR}/lib)
  find_package(Threads)
  set(FORMA_C_LIBRARY ${FORMA_C_LIBRARY} ${LIKWID_LIBRARIES})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_NTRIALS_=1 -D_LIKWID_")
endif()

if( ${CMAKE_C_COMPILER} MATCHES "pgcc")
  set(FORMA_C_FLAGS
    "${FORMA_C_FLAGS} -Minfo -fastsse -Mvect=fuse -Mvect=nosizelimit")
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Minfo -fastsse -Mvect=fuse -Mvect=nosizelimit")
elseif( ${CMAKE_C_COMPILER} MATCHES "icc")
  set(FORMA_C_FLAGS "${FORMA_C_FLAGS} -vec-report3 -mavx -std=c99")
else()
  set(FORMA_C_FLAGS "${FORMA_C_FLAGS} -std=gnu99")
endif()
set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS};${FORMA_CUDA_FLAGS}")

message(STATUS "CMAKE_C_FLAGS :" ${CMAKE_C_FLAGS})
message(STATUS "CMAKE_CXX_FLAGS :" ${CMAKE_CXX_FLAGS})

#Macro to add c experiments
macro(add_c_experiment BENCHMARK_NAME C_DRIVER_FILES)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.c
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}.idsl
    ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl
    COMMAND ${FORMA_EXECUTABLE} ARGS
    ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl  --print-c
    --enable-icc-pragmas --kernel-name ${BENCHMARK_NAME} --use-single-malloc
    --c-output ${BENCHMARK_NAME}.c > ${DEVNULL} 2> ${DEVNULL}
    MAIN_DEPENDENCY
    ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}.idsl
    ${FORMA_EXECUTABLE}
    )
  set(C_KERNEL_FILES ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.c)
  set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/driver.cpp
    PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set_source_files_properties(${C_KERNEL_FILES}
    PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} ${FORMA_C_FLAGS}")
  add_executable(${BENCHMARK_NAME}_C.x ${C_DRIVER_FILES} ${C_KERNEL_FILES})
  target_link_libraries(${BENCHMARK_NAME}_C.x ${FORMA_C_LIBRARY})
  install(TARGETS ${BENCHMARK_NAME}_C.x DESTINATION bin/${BENCHMARK_NAME})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.c
    ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl
    DESTINATION bin/${BENCHMARK_NAME})
endmacro(add_c_experiment)

#Macro to add cuda experiments
macro(add_cuda_experiment BENCHMARK_NAME GPU_DRIVER_FILES)
  if( CUDA_FOUND )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.cu
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}.idsl
      ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl
      COMMAND ${FORMA_EXECUTABLE} ARGS
      ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl  --print-cuda
      --kernel-name ${BENCHMARK_NAME} --cuda-output ${BENCHMARK_NAME}.cu
      --use-single-malloc > ${DEVNULL} 2> ${DEVNULL}
      MAIN_DEPENDENCY
      ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}.idsl ${FORMA_EXECUTABLE}
      )
    set(CUDA_KERNEL_FILES ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.cu)
    cuda_add_library(${BENCHMARK_NAME}_gpu ${CUDA_KERNEL_FILES})
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/driver_gpu.cpp
      PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    add_executable(${BENCHMARK_NAME}_CUDA.x ${GPU_DRIVER_FILES})
    target_link_libraries(${BENCHMARK_NAME}_CUDA.x ${BENCHMARK_NAME}_gpu
      ${FORMA_CUDA_LIBRARY})
    set_target_properties(${BENCHMARK_NAME}_CUDA.x PROPERTIES
      INSTALL_RPATH ${CUDART_PATH})
    install(TARGETS ${BENCHMARK_NAME}_CUDA.x DESTINATION bin/${BENCHMARK_NAME})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.cu
      DESTINATION bin/${BENCHMARK_NAME})
  endif()
endmacro(add_cuda_experiment)

#Macro to add cuda-texture experiments
macro(add_texture_experiment BENCHMARK_NAME GPU_DRIVER_FILES)
  if( CUDA_FOUND )
    add_custom_command(OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.texture.cu
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}.idsl
      ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl
      COMMAND ${FORMA_EXECUTABLE} ARGS
      ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.idsl  --print-cuda
      --use-texture --kernel-name ${BENCHMARK_NAME}
      --cuda-output ${BENCHMARK_NAME}.texture.cu > ${DEVNULL} 2> ${DEVNULL}
      MAIN_DEPENDENCY
      ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}.idsl ${FORMA_EXECUTABLE}
      )
    set(TEXTURE_KERNEL_FILES
      ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.texture.cu)
    cuda_add_library(${BENCHMARK_NAME}_texture ${TEXTURE_KERNEL_FILES})
    set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/driver_gpu.cpp
      PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    add_executable(${BENCHMARK_NAME}_TEX.x ${GPU_DRIVER_FILES})
    target_link_libraries(${BENCHMARK_NAME}_TEX.x
      ${BENCHMARK_NAME}_texture ${FORMA_CUDA_LIBRARY})
    install(TARGETS ${BENCHMARK_NAME}_TEX.x DESTINATION bin/${BENCHMARK_NAME})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${BENCHMARK_NAME}.texture.cu
      DESTINATION bin/${BENCHMARK_NAME})
  endif()
endmacro(add_texture_experiment)

#Macro to add all tests
macro(add_all_tests BENCHMARK_NAME)
  #Add C test
  if("${ARGV1}" STREQUAL "NO_GOLD")
    set(C_DRIVER_FILES
      ${CMAKE_CURRENT_LIST_DIR}/driver.cpp)
  else()
    set(C_DRIVER_FILES
      ${CMAKE_CURRENT_LIST_DIR}/driver.cpp
      ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}_gold.cpp )
  endif()
  add_c_experiment(${BENCHMARK_NAME} "${C_DRIVER_FILES}")

  if( CUDA_FOUND )
    #Add CUDA test
    if("${ARGV1}" STREQUAL "NO_GOLD")
      set(GPU_DRIVER_FILES
        ${CMAKE_CURRENT_LIST_DIR}/driver_gpu.cpp)
    else()
      set(GPU_DRIVER_FILES
        ${CMAKE_CURRENT_LIST_DIR}/driver_gpu.cpp
        ${CMAKE_CURRENT_LIST_DIR}/${BENCHMARK_NAME}_gold.cpp )
    endif()
    add_cuda_experiment(${BENCHMARK_NAME} "${GPU_DRIVER_FILES}")

    #Add CUDA-Texture test
    add_texture_experiment(${BENCHMARK_NAME} "${GPU_DRIVER_FILES}")

  endif()
endmacro(add_all_tests)

add_subdirectory(emboss)
add_subdirectory(blur)
add_subdirectory(canny)
add_subdirectory(hdr_direct)
add_subdirectory(pyramid)
add_subdirectory(pyramid_mirror)
add_subdirectory(pyramid_rgb)
add_subdirectory(pyramid_rgb_mirror)
add_subdirectory(camera_pipe)
add_subdirectory(hdr_pyramid)

install(FILES try.sh
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
  DESTINATION bin)
