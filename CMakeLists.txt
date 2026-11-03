cmake_minimum_required(VERSION 3.21)

project(traffic_ca VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(OpenMP REQUIRED COMPONENTS CXX)
find_package(Qt6 6.2 COMPONENTS Core Widgets QUIET)
if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "Qt 6 (Core and Widgets) was not found. Install Qt 6 with the MSVC kit "
        "using Qt Online Installer, then configure with "
        "-DCMAKE_PREFIX_PATH=<path-to-Qt-kit> (for example C:/Qt/6.x.x/msvc20xx_64).")
endif()

add_library(traffic_core
    src/config.hpp
    src/engine.cpp
    src/engine.hpp
    src/grid.hpp
    src/metrics.hpp
    src/model.hpp
    src/rng.hpp
    src/types.hpp
)
target_include_directories(traffic_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(traffic_core PUBLIC OpenMP::OpenMP_CXX)

add_executable(traffic_ca
    src/main.cpp
    src/scenario_io.hpp
    src/scenario_path.hpp
)
target_link_libraries(traffic_ca PRIVATE traffic_core Qt6::Core)

add_executable(traffic_benchmark
    src/benchmark.cpp
    src/scenario_io.hpp
    src/scenario_path.hpp
)
target_link_libraries(traffic_benchmark PRIVATE traffic_core Qt6::Core)

qt_add_executable(traffic_ca_gui WIN32
    src/gui.cpp
    src/scenario_io.hpp
    src/scenario_path.hpp
)
target_link_libraries(traffic_ca_gui PRIVATE traffic_core Qt6::Widgets)

if(WIN32)
    get_filename_component(QT6_INSTALL_PREFIX "${Qt6_DIR}/../../.." ABSOLUTE)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt.exe windeployqt
        HINTS "${QT6_INSTALL_PREFIX}/bin"
        REQUIRED
    )
    foreach(target traffic_ca traffic_benchmark traffic_ca_gui)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<TARGET_FILE:${target}>"
            COMMENT "Deploying Qt runtime for ${target}"
            VERBATIM
        )
    endforeach()
endif()

foreach(target traffic_core traffic_ca traffic_benchmark traffic_ca_gui)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/config")
configure_file(config/scenarios.json "${CMAKE_BINARY_DIR}/config/scenarios.json" COPYONLY)

include(CTest)
if(BUILD_TESTING)
    add_executable(traffic_tests tests/test_all.cpp)
    target_link_libraries(traffic_tests PRIVATE traffic_core)
    if(MSVC)
        target_compile_options(traffic_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(traffic_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME traffic_core_tests COMMAND traffic_tests)
endif()
