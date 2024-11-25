cmake_minimum_required(VERSION 3.20)
project(Injector)

add_library(userenv SHARED
    dllmain.cpp
    Injector.cpp
    stdafx.cpp
)
target_link_libraries(userenv
    dbghelp.lib
)
target_link_options(
    userenv PRIVATE /DEF:${PROJECT_SOURCE_DIR}/Source.def
)