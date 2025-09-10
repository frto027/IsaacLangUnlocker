cmake_minimum_required(VERSION 3.20)
project(Injector)

option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory("zydis")


add_library(userenv SHARED
    dllmain.cpp
    Injector.cpp
    stdafx.cpp
)
target_link_libraries(userenv
    dbghelp.lib Winmm.lib Zydis
)
target_link_options(
    userenv PRIVATE /DEF:${PROJECT_SOURCE_DIR}/Source.def
)