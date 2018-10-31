cmake_minimum_required(VERSION 3.1)
project(wasp)

set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")

add_subdirectory(third_party/abseil)

add_executable(wasp src/main.cc)
target_link_libraries(wasp absl::base)
