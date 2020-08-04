macro(prost_set_compiler_flags)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -W -Wno-sign-compare")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -ansi -fmax-errors=2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Werror")

    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag( "-std=c++17" CXX11_FOUND )
    if(CXX11_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    else()
        message(FATAL_ERROR "${CMAKE_CXX_COMPILER} does not support C++17, please use a different compiler")
    endif()

    # Configuration-specific flags
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DDOCTEST_CONFIG_DISABLE -DNDEBUG -fomit-frame-pointer")
endmacro()

macro(prost_set_linker_flags)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -g")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} -O3")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} -O3")
endmacro()
