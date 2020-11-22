cmake_minimum_required(VERSION 3.17)
project(xstd)

# Header only interface.
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE includes)

# C++20 requirement.
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# For portability on non-MSVC compilers, add some compile options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${PROJECT_NAME} INTERFACE -Wno-unused-value)       # RNG discarding index.
    target_compile_options(${PROJECT_NAME} INTERFACE -Wno-unused-function)    # Static helpers
    target_compile_options(${PROJECT_NAME} INTERFACE -Wno-format-security)    # Custom logger
endif()