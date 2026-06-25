# SPDX-License-Identifier: GPL-3.0-only
#
# Cracked / Offline-only module for Prism Launcher
#
# This file is meant to be included from the main launcher/CMakeLists.txt
# when ENABLE_CRACKED_MODE=ON (or the top-level option is enabled).
#
# Usage (in launcher/CMakeLists.txt):
#
#   option(ENABLE_CRACKED_MODE "Build Prism Launcher with offline-only / cracked support" ON)
#
#   if(ENABLE_CRACKED_MODE)
#       add_definitions(-DPRISMLAUNCHER_CRACKED)
#       add_subdirectory(cracked)
#       list(APPEND LAUNCHER_SOURCES
#           cracked/CrackedAccountManager.cpp
#           cracked/OfflineSkinManager.cpp
#           cracked/CrackedUtils.cpp
#           # add more .cpp here as the module grows
#       )
#       # Also expose the include dir so you can do #include "cracked/..."
#   endif()
#

set(CRACKED_SOURCES
    CrackedUtils.cpp
    CrackedAccountManager.cpp
    OfflineSkinManager.cpp
)

set(CRACKED_HEADERS
    CrackedAccountManager.h
    OfflineSkinManager.h
    CrackedUtils.h
)

# We create a small static library so the module is easy to link and test in isolation.
add_library(PrismLauncher_Cracked STATIC ${CRACKED_SOURCES} ${CRACKED_HEADERS})

target_include_directories(PrismLauncher_Cracked
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/launcher   # reach MinecraftAccount etc.
)

target_link_libraries(PrismLauncher_Cracked
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# Make sure the define is visible to everyone who links us
target_compile_definitions(PrismLauncher_Cracked PUBLIC PRISMLAUNCHER_CRACKED)

# Optional: export the headers for the main target
set_target_properties(PrismLauncher_Cracked PROPERTIES
    PUBLIC_HEADER "${CRACKED_HEADERS}"
)

message(STATUS "PrismLauncher-Cracked module enabled - offline features activated")
