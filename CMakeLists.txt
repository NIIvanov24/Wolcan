cmake_minimum_required(VERSION 3.24)
project(Wolcan LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

option(WOLCAN_BUILD_GAME "Build the raylib presentation layer" ON)

add_library(DAL STATIC src/DAL/MazeLoader.cpp src/DAL/SaveManager.cpp src/DAL/ProfileStore.cpp)
target_include_directories(DAL PUBLIC include)

add_library(BLL STATIC
    src/BLL/Maze.cpp
    src/BLL/Player.cpp
    src/BLL/GameLogic.cpp
    src/BLL/Progression.cpp
)
target_include_directories(BLL PUBLIC include)
target_link_libraries(BLL PRIVATE DAL)

if(WOLCAN_BUILD_GAME)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
    if(EMSCRIPTEN)
        set(PLATFORM Web CACHE STRING "raylib platform" FORCE)
    endif()

    set(raylibArchive "https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/downloads/raylib-5.5.tar.gz")
        set(raylibArchive "${CMAKE_CURRENT_SOURCE_DIR}/build/downloads/raylib-5.5.tar.gz")
    endif()
    FetchContent_Declare(raylib
        URL "${raylibArchive}"
        URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    # raylib 5.5 predates CMake 4; use the documented third-party policy floor.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
    FetchContent_MakeAvailable(raylib)

    # Stable paths also let the original vcxproj consume this same dependency.
    set_target_properties(raylib PROPERTIES ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/raylib/lib")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/raylib/include")
    configure_file("${raylib_SOURCE_DIR}/src/raylib.h" "${CMAKE_BINARY_DIR}/raylib/include/raylib.h" COPYONLY)

    add_executable(Wolcan
        src/PL/Main.cpp src/PL/MainMenu.cpp src/PL/GameScreen.cpp
        src/PL/MenuLayout.cpp src/PL/MenuArt.cpp src/PL/WindowControls.cpp src/PL/GameArt.cpp
        src/PL/GameSettings.cpp src/PL/GameAudio.cpp)
    target_link_libraries(Wolcan PRIVATE BLL raylib)
    # Keep assets beside the executable for IDE launches and arbitrary working directories.
    if(EMSCRIPTEN)
        set_target_properties(Wolcan PROPERTIES SUFFIX ".html")
        target_link_options(Wolcan PRIVATE "-sASYNCIFY" "-sASYNCIFY_STACK_SIZE=32768" "-sALLOW_MEMORY_GROWTH=1" "-fexceptions"
            "-sEXPORTED_RUNTIME_METHODS=['HEAPF32']"
            "SHELL:--preload-file \"${CMAKE_CURRENT_SOURCE_DIR}/assets@/assets\""
            "SHELL:--shell-file \"${CMAKE_CURRENT_SOURCE_DIR}/Wolcan/PL/WebShell.html\"")
        foreach(target Wolcan BLL DAL)
            target_compile_options(${target} PRIVATE "-fexceptions")
        endforeach()
        file(GLOB_RECURSE webAssets CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
        set_property(TARGET Wolcan APPEND PROPERTY LINK_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/Wolcan/PL/WebShell.html" "${webAssets}")
    else()
        add_custom_target(WolcanAssets ALL
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:Wolcan>/assets"
            VERBATIM)
        add_dependencies(Wolcan WolcanAssets)
    endif()
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Wolcan)
    if(MSVC)
        target_compile_options(Wolcan PRIVATE /W4 /WX /permissive- /utf-8)
    endif()
endif()

if(MSVC)
    target_compile_options(BLL PRIVATE /W4 /WX /permissive-)
    target_compile_options(DAL PRIVATE /W4 /WX /permissive-)
endif()
