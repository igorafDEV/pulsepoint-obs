cmake_minimum_required(VERSION 3.22)

# =============================================================================
# PulsePoint OBS — leva cada plugin PulsePoint para o OBS Studio como filtro de áudio.
#
# Um módulo do OBS por plugin (plugins.cmake). Cada módulo carrega o VST3 daquele plugin, que
# é instalado e licenciado à parte. Compila sozinho (cmake -B build) ou dentro do repositório
# dos plugins, que o inclui com add_subdirectory e já traz o JUCE.
# =============================================================================

set(PP_OBS_HOST_VERSION 1.0.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo (o do OBS 30)")
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "binário universal")
    project(PulsePointOBS VERSION ${PP_OBS_HOST_VERSION} LANGUAGES C CXX)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        add_compile_options(/utf-8)
    endif()

    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.13)
    FetchContent_MakeAvailable(JUCE)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/PulsePointObsHost.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/plugins.cmake)
