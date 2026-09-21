cmake_minimum_required(VERSION 3.16)
project(ProjectDex VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Network QUIET)
find_package(Qt6 COMPONENTS Quick QUIET)

if(NOT Qt6Core_FOUND OR NOT Qt6Network_FOUND)
    message(FATAL_ERROR "Qt6 Core and Network are required (qt6-base-dev / qt6-base-dev-tools).")
endif()

# --- Headless Steam sync test ---
add_executable(sync_test
    src/steam_client.cpp
    src/sync_test_main.cpp
)
target_link_libraries(sync_test PRIVATE Qt6::Core Qt6::Network)

# --- Headless install-state detection test ---
add_executable(install_state_test
    src/vdf_parser.cpp
    src/library_scanner.cpp
    src/install_state_test_main.cpp
)
target_link_libraries(install_state_test PRIVATE Qt6::Core)

# --- Full QML shell (only built once Qt6 Quick is available) ---
if(Qt6Quick_FOUND)
    message(STATUS "Qt6 Quick found -- building full Project Dex QML app")
    qt_add_executable(project_dex
        src/main.cpp
    )
    qt_add_qml_module(project_dex
        URI ProjectDex
        VERSION 1.0
        QML_FILES src/qml/Main.qml
    )
    target_link_libraries(project_dex PRIVATE Qt6::Core Qt6::Network Qt6::Quick)
else()
    message(STATUS "Qt6 Quick not found -- skipping full QML app")
endif()
