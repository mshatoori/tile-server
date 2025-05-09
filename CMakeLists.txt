cmake_minimum_required(VERSION 3.16)
project(osm_mapnik_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Dependencies ---

# Find Boost components as before
find_package(Boost 1.71 REQUIRED COMPONENTS system thread filesystem program_options date_time)

# Use PkgConfig to find Mapnik
find_package(PkgConfig REQUIRED)
pkg_check_modules(MAPNIK REQUIRED IMPORTED_TARGET mapnik)
# This command does the following:
# 1. Calls `pkg-config --exists mapnik` (checks if mapnik.pc exists)
# 2. If it exists, it creates an IMPORTED target called PkgConfig::MAPNIK
# 3. This target automatically gets the necessary include directories,
#    library paths, link flags, and definitions from mapnik.pc.
# 4. If mapnik.pc is not found, it fails because of REQUIRED.

# Find Threads as before
find_package(Threads REQUIRED)

# --- Include Directories ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    # No need for ${MAPNIK_INCLUDE_DIRS} when using IMPORTED_TARGET
)

# --- Source Files ---
add_executable(osm_mapnik_server
    src/main.cpp
    src/tile_renderer.cpp
    src/http_server.cpp
)

# --- Link Libraries ---
target_link_libraries(osm_mapnik_server PRIVATE
    Threads::Threads
    Boost::system
    Boost::thread
    Boost::filesystem
    Boost::program_options
    Boost::date_time
    PkgConfig::MAPNIK # <-- Link against the imported target
)

# --- Build Options ---
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# --- Installation ---
install(TARGETS osm_mapnik_server DESTINATION bin)
install(FILES styles/basic_style.xml DESTINATION share/osm_mapnik_server/styles)