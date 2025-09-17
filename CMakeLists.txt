cmake_minimum_required(VERSION 3.20)


project(Injector)

option(LANG_CN "Enable Chinese language compile" ON)
option(LANG_EN "Enable English language compile" OFF)
option(LANG_KR "Enable Korean language compile" OFF)

########## injector ############

option(ZYDIS_BUILD_TOOLS "" OFF)
option(ZYDIS_BUILD_EXAMPLES "" OFF)
add_subdirectory("zydis")

if(LANG_CN)
    add_definitions(-DLANG_KR)
else()
    add_definitions(-DLANG_NOT_CN)
endif()

if(LANG_KR)
    add_definitions(-DLANG_KR)
endif()
if(LANG_EN)
    add_definitions(-DLANG_EN)
endif()

add_library(inject SHARED
    injector/dllmain.cpp
    injector/patchers.cpp
)
target_link_libraries(inject
    dbghelp.lib Winmm.lib Shlwapi.lib Zydis
)

########## bootstrap #########

add_library(bootstp SHARED
    bootstrap/main.cpp
)
target_link_libraries(bootstp
    Shlwapi.lib
)
target_link_options(
    bootstp PRIVATE /DEF:${PROJECT_SOURCE_DIR}/bootstrap/Source.def
)

########## exe patcher source maker ######
set(GENERATED_RES ${CMAKE_CURRENT_BINARY_DIR}/res.inl)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(
    OUTPUT  ${GENERATED_RES}
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/exe_patcher/make_resource.py $<TARGET_FILE:bootstp> ${GENERATED_RES}
    DEPENDS bootstp ${PROJECT_SOURCE_DIR}/exe_patcher/make_resource.py
    COMMENT "Generating Headers"
    VERBATIM
)
set_source_files_properties(${GENERATED_RES} PROPERTIES GENERATED TRUE)

######### exe patcher #########

add_executable(patcher WIN32 exe_patcher/main.cpp ${GENERATED_RES})

target_link_libraries(patcher Comdlg32.lib Shlwapi.lib)
target_include_directories(patcher PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
