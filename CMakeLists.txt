cmake_minimum_required(VERSION 3.15)

project(pdf_extract LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(pdf_extract
  main.cpp
  src/extractor.cpp
)

target_include_directories(pdf_extract PRIVATE include)

if (MSVC)
  target_compile_options(pdf_extract PRIVATE /W4)
else()
  target_compile_options(pdf_extract PRIVATE -Wall -Wextra -Wpedantic)
endif()