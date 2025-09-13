cmake_minimum_required(VERSION 3.20)


project(Injector)

option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory("zydis")


add_library(inject SHARED
    injector/dllmain.cpp
    injector/patchers.cpp
)
target_link_libraries(inject
    dbghelp.lib Winmm.lib Shlwapi.lib Zydis
)

project(Loader)

add_library(dsound SHARED
    loader/main.cpp
)
target_link_libraries(dsound
    Shlwapi.lib
)