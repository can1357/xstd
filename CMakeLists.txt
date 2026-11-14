cmake_minimum_required(VERSION 3.17)
project(xstd LANGUAGES CXX)

# Header-only interface library: link as xstd::xstd.
add_library(xstd INTERFACE)
add_library(xstd::xstd ALIAS xstd)
target_include_directories(xstd INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_compile_features(xstd INTERFACE cxx_std_20)

# Portability on non-MSVC compilers.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(xstd INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-value>      # RNG discarding index.
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-function>   # Static helpers.
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-format-security>) # Custom logger.
endif()

# Opt-in NVCC host-header compatibility check; normal builds never probe CUDA.
# Configure with -DXSTD_BUILD_CUDA_CHECKS=ON, then build xstd_cuda_checks.
option(XSTD_BUILD_CUDA_CHECKS "Build the CUDA header compile check" OFF)
if(XSTD_BUILD_CUDA_CHECKS)
  enable_language(CUDA)
  add_library(xstd_cuda_checks OBJECT tests/cuda_check.cu)
  target_link_libraries(xstd_cuda_checks PRIVATE xstd::xstd)
  set_target_properties(xstd_cuda_checks PROPERTIES
    CUDA_STANDARD 20
    CUDA_STANDARD_REQUIRED YES
    CUDA_EXTENSIONS NO)

  # Match the host baseline used by the ordinary compile checks.
  if(CMAKE_CUDA_COMPILER_ID STREQUAL "NVIDIA" AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU"
     AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(xstd_cuda_checks PRIVATE -Xcompiler=-march=haswell)
  endif()
endif()

# CI / development: compile every public header in one TU at the advertised
# C++20 floor. Headers wrapping external libraries are excluded.
option(XSTD_BUILD_CHECKS "Build the include-all compile check" OFF)
if(XSTD_BUILD_CHECKS)
  file(GLOB xstd_headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/includes/xstd
       ${CMAKE_CURRENT_SOURCE_DIR}/includes/xstd/*.hpp)
  list(REMOVE_ITEM xstd_headers
       gzip.hpp zstd.hpp                  # need zlib / libzstd
       plf_colony.hpp robin_hood.hpp)     # vendored, pulled in transitively
  list(SORT xstd_headers)
  set(tu_content "")
  foreach(header IN LISTS xstd_headers)
    string(APPEND tu_content "#include <xstd/${header}>\n")
  endforeach()
  string(APPEND tu_content "int main() { return 0; }\n")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include_all.cpp "${tu_content}")

  add_executable(xstd_include_all ${CMAKE_CURRENT_BINARY_DIR}/include_all.cpp)
  target_link_libraries(xstd_include_all PRIVATE xstd::xstd)

  # xstd assumes BMI2 / SSE4.2 / CX16 on AMD64; match upstream's -march baseline.
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(xstd_include_all PRIVATE -march=haswell)
  endif()

  # The hash paths are the one place where a second implementation of the same
  # algorithm exists, so they get a differential test rather than just a vector.
  enable_testing()
  add_executable(xstd_hash_test tests/hash_test.cpp)
  target_link_libraries(xstd_hash_test PRIVATE xstd::xstd)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_compile_options(xstd_hash_test PRIVATE -march=haswell)
  endif()
  add_test(NAME hash COMMAND xstd_hash_test)
endif()
