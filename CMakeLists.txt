cmake_minimum_required(VERSION 3.15)
project(crayon_emulator VERSION 0.1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    if(NOT CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        add_compile_options(/W4)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_LIBRETRO "Build libretro core" ON)
option(BUILD_STANDALONE "Build standalone emulator" ON)
option(ENABLE_SDL "Enable SDL2 frontend (requires SDL2)" ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files for core emulator library
# MO5-specific components (stubs initially, filled in per milestone)
set(CORE_SOURCES
    src/cpu6809.cpp
    src/gate_array.cpp
    src/memory_system.cpp
    src/pia.cpp
    src/audio_system.cpp
    src/input_handler.cpp
    src/light_pen.cpp
    src/cassette_interface.cpp
    src/master_clock.cpp
    src/emulator_core.cpp
    src/disassembler.cpp
    src/savestate.cpp
    src/debugger.cpp
    src/debugger_ui.cpp
    src/frontend_headless.cpp
)

# UI source files
set(UI_SOURCES
    src/ui/config_manager.cpp
    src/ui/zip_handler.cpp
    src/ui/text_renderer.cpp
    src/ui/dialogs.cpp
    src/ui/file_browser.cpp
    src/ui/menu_system.cpp
    src/ui/osd_renderer.cpp
    src/ui/save_state_manager.cpp
    src/ui/input_mapper.cpp
    src/ui/recent_files_list.cpp
    src/ui/imgui_debugger_ui.cpp
)

# Core emulator library
add_library(crayon_core STATIC ${CORE_SOURCES})
target_include_directories(crayon_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
set_target_properties(crayon_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Find miniz for ZIP support
find_package(miniz CONFIG QUIET)
if(miniz_FOUND)
    target_link_libraries(crayon_core PUBLIC miniz::miniz)
    target_compile_definitions(crayon_core PUBLIC HAVE_MINIZ)
    message(STATUS "Building with miniz support for ZIP files")
else()
    message(WARNING "miniz not found. ZIP file support will be disabled.")
endif()

# Dear ImGui (for debugger UI)
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${IMGUI_SOURCES} PROPERTIES
        COMPILE_FLAGS "-Wno-error=strict-aliasing"
    )
    set_source_files_properties(src/ui/save_state_manager.cpp PROPERTIES
        COMPILE_FLAGS "-Wno-error=deprecated-declarations"
    )
endif()

# Standalone executable
if(BUILD_STANDALONE)
    if(ENABLE_SDL)
        if(WIN32)
            find_package(SDL2 CONFIG QUIET)
            if(NOT SDL2_FOUND)
                find_package(SDL2 MODULE)
            endif()
            find_package(SDL2_ttf CONFIG QUIET)
        else()
            find_package(SDL2)
            find_package(SDL2_ttf QUIET)
        endif()
        
        if(SDL2_FOUND)
            add_executable(crayon
                src/main.cpp
                src/frontend_sdl.cpp
                ${UI_SOURCES}
            )
            
            target_sources(crayon PRIVATE ${IMGUI_SOURCES})
            target_include_directories(crayon PRIVATE 
                ${imgui_SOURCE_DIR} 
                ${imgui_SOURCE_DIR}/backends
            )
            
            if(TARGET SDL2::SDL2)
                if(WIN32 AND TARGET SDL2::SDL2main)
                    target_link_libraries(crayon crayon_core SDL2::SDL2main SDL2::SDL2)
                else()
                    target_link_libraries(crayon crayon_core SDL2::SDL2)
                endif()
                
                if(TARGET SDL2_ttf::SDL2_ttf)
                    target_link_libraries(crayon SDL2_ttf::SDL2_ttf)
                    target_compile_definitions(crayon PRIVATE HAVE_SDL_TTF)
                endif()
                message(STATUS "Building with SDL2 support (vcpkg)")
            else()
                target_include_directories(crayon PRIVATE ${SDL2_INCLUDE_DIRS})
                target_link_libraries(crayon crayon_core ${SDL2_LIBRARIES})
                message(STATUS "Building with SDL2 support (manual)")
            endif()
            
            target_compile_definitions(crayon PRIVATE ENABLE_SDL)
            
            if(WIN32 AND TARGET SDL2::SDL2)
                add_custom_command(TARGET crayon POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL2::SDL2>
                    $<TARGET_FILE_DIR:crayon>
                )
            endif()
        else()
            message(WARNING "SDL2 not found. Building headless-only version.")
            add_executable(crayon src/main.cpp)
            target_link_libraries(crayon crayon_core)
        endif()
    else()
        message(STATUS "SDL2 disabled, building headless-only version")
        add_executable(crayon src/main.cpp)
        target_link_libraries(crayon crayon_core)
    endif()
endif()

# Libretro core
if(BUILD_LIBRETRO)
    add_library(crayon_libretro SHARED src/libretro.cpp)
    target_link_libraries(crayon_libretro crayon_core)
    target_include_directories(crayon_libretro PRIVATE ${CMAKE_SOURCE_DIR}/include)
    set_target_properties(crayon_libretro PROPERTIES PREFIX "" OUTPUT_NAME "crayon_libretro")
    if(NOT WIN32)
        target_compile_options(crayon_libretro PRIVATE -fvisibility=hidden)
    endif()
endif()

# Install rules
if(BUILD_STANDALONE AND TARGET crayon)
    install(TARGETS crayon RUNTIME DESTINATION bin)
    if(WIN32 AND TARGET SDL2::SDL2)
        install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION bin)
    endif()
endif()
if(BUILD_LIBRETRO AND TARGET crayon_libretro)
    install(TARGETS crayon_libretro LIBRARY DESTINATION lib RUNTIME DESTINATION lib)
    install(FILES ${CMAKE_SOURCE_DIR}/crayon_libretro.info DESTINATION lib)
endif()

# Testing
if(BUILD_TESTS)

    # Boot diagnostic tool (links only against core, no SDL needed)
    add_executable(boot_diag tools/boot_diag.cpp)
    target_link_libraries(boot_diag crayon_core)

    # ROM disassembly tool
    add_executable(disasm_roms tools/disasm_roms.cpp)
    target_link_libraries(disasm_roms crayon_core)

    enable_testing()
    
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_GetProperties(googletest)
    if(NOT googletest_POPULATED)
        FetchContent_Populate(googletest)
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    FetchContent_Declare(
        rapidcheck
        GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
        GIT_TAG master
    )
    FetchContent_GetProperties(rapidcheck)
    if(NOT rapidcheck_POPULATED)
        FetchContent_Populate(rapidcheck)
        add_subdirectory(${rapidcheck_SOURCE_DIR} ${rapidcheck_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    
    # Test executable — starts empty, tests added per milestone
    add_executable(crayon_tests
        tests/test_placeholder.cpp
        tests/test_cpu_interrupts.cpp
    )
    
    target_link_libraries(crayon_tests crayon_core gtest_main rapidcheck)
    
    # Only add ImGui + SDL2 to tests when SDL2 is available
    if(TARGET SDL2::SDL2)
        target_sources(crayon_tests PRIVATE ${IMGUI_SOURCES})
        target_include_directories(crayon_tests PRIVATE 
            ${imgui_SOURCE_DIR} 
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(crayon_tests SDL2::SDL2)
        if(TARGET SDL2_ttf::SDL2_ttf)
            target_link_libraries(crayon_tests SDL2_ttf::SDL2_ttf)
        endif()
    elseif(SDL2_FOUND)
        target_sources(crayon_tests PRIVATE ${IMGUI_SOURCES})
        target_include_directories(crayon_tests PRIVATE 
            ${imgui_SOURCE_DIR} 
            ${imgui_SOURCE_DIR}/backends
            ${SDL2_INCLUDE_DIRS}
        )
        target_link_libraries(crayon_tests ${SDL2_LIBRARIES})
    endif()
    
    include(GoogleTest)
    gtest_discover_tests(crayon_tests)
endif()
