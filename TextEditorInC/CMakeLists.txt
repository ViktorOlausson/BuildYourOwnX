cmake_minimum_required(VERSION 3.31)
project(TextEditorInC C)

set(CMAKE_C_STANDARD 11)

add_executable(TextEditorInC main.c
        editor.h)
