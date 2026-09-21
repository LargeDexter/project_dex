cmake_minimum_required(VERSION 3.16)
project(ProjectDex VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Network Concurrent QUIET)
find_package(Qt6 COMPONENTS Quick QuickControls2 QUIET)
find_package(Qt6 COMPONENTS Multimedia QUIET)

if(NOT Qt6Core_FOUND OR NOT Qt6Network_FOUND OR NOT Qt6Concurrent_FOUND)
    message(FATAL_ERROR "Qt6 Core, Network, and Concurrent are required (qt6-base-dev / qt6-base-dev-tools).")
endif()

# Trailer playback (Hero banner + expanded-view idle trailer) needs
# QtMultimedia. On Fedora/Bazzite this is `qt6-qtmultimedia-devel`, and
# actual mp4/H.264 *playback* additionally needs GStreamer's H.264 decoder,
# which stock Fedora's GStreamer omits for licensing reasons -- that comes
# from RPM Fusion's `gstreamer1-plugin-openh264` or `gstreamer1-libav`
# package. Without these, the app still builds and runs fine EXCEPT for
# trailer playback specifically, which is the only thing that depends on
# this component.
if(Qt6Quick_FOUND AND NOT Qt6Multimedia_FOUND)
    message(WARNING
        "Qt6 Multimedia not found -- building WITHOUT trailer "
        "playback. Install qt6-qtmultimedia-devel (and, for mp4 playback, a "
        "GStreamer H.264 plugin such as gstreamer1-plugin-openh264 or "
        "gstreamer1-libav from RPM Fusion) and re-run cmake to enable it.")
endif()

# Controller support (D-pad/left-stick navigation, A/B confirm/back) via
# SDL2's GameController API -- same graceful-degrade pattern as Multimedia
# above: without SDL2-devel, the app still builds and runs fine with
# keyboard/mouse only, just without gamepad input. Bazzite/SteamOS-like
# systems normally already have SDL2 installed for Steam itself; on plain
# Fedora it's `sudo dnf install SDL2-devel`.
find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
    message(WARNING
        "SDL2 not found -- building WITHOUT controller support. Install "
        "SDL2-devel and re-run cmake to enable it.")
endif()

# QR code rendering for the setup wizard's "scan to see how to get this
# key" step -- same graceful-degrade pattern as SDL2/Multimedia above:
# without it, the wizard just shows the instructions URL as plain text
# instead of a QR code. libqrencode doesn't ship a CMake config, only a
# pkg-config file, hence going via PkgConfig rather than find_package
# directly. On Fedora/Bazzite this is `qrencode-devel`.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(QRENCODE QUIET libqrencode)
endif()
if(NOT QRENCODE_FOUND)
    message(WARNING
        "libqrencode not found -- building WITHOUT QR codes in the setup "
        "wizard. Install qrencode-devel and re-run cmake to enable it.")
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

# --- Headless box art / metadata test ---
add_executable(artwork_test
    src/artwork_client.cpp
    src/artwork_test_main.cpp
)
target_link_libraries(artwork_test PRIVATE Qt6::Core Qt6::Network)

# --- Headless store details test ---
add_executable(store_details_test
    src/store_details_client.cpp
    src/store_details_test_main.cpp
)
target_link_libraries(store_details_test PRIVATE Qt6::Core Qt6::Network)

# --- Headless ProtonDB rating test ---
add_executable(proton_db_test
    src/proton_db_client.cpp
    src/proton_db_test_main.cpp
)
target_link_libraries(proton_db_test PRIVATE Qt6::Core Qt6::Network)

# --- Headless system-spec scan / compatibility-matcher test ---
add_executable(system_specs_test
    src/system_specs.h
    src/system_specs.cpp
    src/system_specs_test_main.cpp
)
target_link_libraries(system_specs_test PRIVATE Qt6::Core)
qt_add_resources(system_specs_test "hardware_data_test"
    PREFIX "/data"
    BASE "data"
    FILES data/hardware_benchmarks.json
)

# --- Full QML shell (only built once Qt6 Quick is available) ---
if(Qt6Quick_FOUND)
    message(STATUS "Qt6 Quick found -- building full Project Dex QML app")
    qt_add_executable(project_dex
        src/main.cpp
        src/theme.h
        src/input_state.h
        src/ui_state.h
        src/system_specs.h
        src/vdf_parser.cpp
        src/library_scanner.cpp
        src/steam_client.cpp
        src/artwork_client.cpp
        src/store_details_client.cpp
        src/proton_db_client.cpp
        src/trailer_cache.h
        src/trailer_cache.cpp
        src/system_specs.cpp
        src/library_sync_worker.cpp
        src/game_library_model.cpp
        src/gamepad_input.h
        src/gamepad_input.cpp
        src/app_settings.h
        src/app_settings.cpp
        src/settings_manager.h
        src/settings_manager.cpp
        src/qr_image_provider.h
        src/qr_image_provider.cpp
    )
    qt_add_resources(project_dex "hardware_data"
        PREFIX "/data"
        BASE "data"
        FILES data/hardware_benchmarks.json
    )
    if(SDL2_FOUND)
        # Bundled community controller-mapping database (see
        # gamepad_input.cpp's loadCommunityMappings()) -- the system's own
        # SDL2 package ships with whatever mappings existed when Fedora/
        # Bazzite last packaged it, which is often stale or missing entries
        # entirely for less mainstream pads. This is refreshed from
        # https://github.com/mdqinc/SDL_GameControllerDB periodically.
        qt_add_resources(project_dex "gamecontroller_data"
            PREFIX "/data"
            BASE "data"
            FILES data/gamecontrollerdb.txt
        )
    endif()
    qt_add_qml_module(project_dex
        URI ProjectDex
        VERSION 1.0
        NO_CACHEGEN
        QML_FILES
            src/qml/Main.qml
            src/qml/Hero.qml
            src/qml/GameRow.qml
            src/qml/GameTile.qml
            src/qml/DetailPanel.qml
            src/qml/ExpandedGameView.qml
            src/qml/Icon.qml
            src/qml/ControllerSetupScreen.qml
            src/qml/OnScreenKeyboard.qml
            src/qml/SetupWizardScreen.qml
            src/qml/SettingsScreen.qml
    )
    target_link_libraries(project_dex PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Concurrent
        Qt6::Quick
    )
    if(Qt6QuickControls2_FOUND)
        target_link_libraries(project_dex PRIVATE Qt6::QuickControls2)
    endif()
    if(Qt6Multimedia_FOUND)
        target_link_libraries(project_dex PRIVATE Qt6::Multimedia)
    endif()
    if(SDL2_FOUND)
        target_compile_definitions(project_dex PRIVATE PROJECT_DEX_HAS_SDL2)
        if(TARGET SDL2::SDL2)
            target_link_libraries(project_dex PRIVATE SDL2::SDL2)
        else()
            target_include_directories(project_dex PRIVATE ${SDL2_INCLUDE_DIRS})
            target_link_libraries(project_dex PRIVATE ${SDL2_LIBRARIES})
        endif()
    endif()
    if(QRENCODE_FOUND)
        target_compile_definitions(project_dex PRIVATE PROJECT_DEX_HAS_QRENCODE)
        target_include_directories(project_dex PRIVATE ${QRENCODE_INCLUDE_DIRS})
        target_link_libraries(project_dex PRIVATE ${QRENCODE_LIBRARIES})
    endif()
else()
    message(STATUS "Qt6 Quick not found -- skipping full QML app")
endif()
