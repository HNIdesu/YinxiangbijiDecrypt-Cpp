cmake_minimum_required(VERSION 3.10)
project(yinxiangbijidecrypt VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
include_directories(include)
link_directories(lib)
add_subdirectory(src)