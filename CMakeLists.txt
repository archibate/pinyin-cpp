cmake_minimum_required(VERSION 3.18)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_STANDARD 20)

project(main LANGUAGES CXX)

file(GLOB srcs "src/*.cpp")
file(GLOB tests "tests/*.cpp")

foreach(file ${tests})
    get_filename_component(name ${file} NAME_WE)
    add_executable(${name} ${file} ${srcs})
    target_include_directories(${name} PRIVATE include)
endforeach(file)
