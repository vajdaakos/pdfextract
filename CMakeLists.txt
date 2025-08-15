cmake_minimum_required(VERSION 3.15)

project(pdf_extract LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build shared library of core functionality
add_library(pdf_extract_lib
  src/extractor.cpp
)

target_include_directories(pdf_extract_lib PUBLIC include)

if (MSVC)
  target_compile_options(pdf_extract_lib PRIVATE /W4)
else()
  target_compile_options(pdf_extract_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Executable links to the library
add_executable(pdf_extract
  main.cpp
)

target_link_libraries(pdf_extract PRIVATE pdf_extract_lib)

# Tests
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

enable_testing()

add_executable(pdf_extract_tests
  tests/test_parse.cpp
)

target_include_directories(pdf_extract_tests PRIVATE include)

target_link_libraries(pdf_extract_tests PRIVATE pdf_extract_lib Catch2::Catch2WithMain)

add_test(NAME pdf_extract_tests COMMAND pdf_extract_tests)