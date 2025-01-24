cmake_minimum_required(VERSION 3.29)
project(promptly)

set(CMAKE_CXX_STANDARD 23)

add_compile_options(-Wall -Wextra)

add_executable(promptly main.cpp
        Segment/Segment.cpp
        Segment/Segment.h
        term.h
        Element/Element.cpp
        Element/Element.h
        icons.h
        List/List.h
)
