cmake_minimum_required(VERSION 3.16)
project(hft_engine)

set(CMAKE_CXX_STANDARD 20)

# Force a Debug build type (disables heavy optimizations, keeps debug symbols)
set(CMAKE_BUILD_TYPE Debug)

# find DPDK
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

# your executable
add_executable(hft_engine main.cpp)

# include paths
target_include_directories(hft_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${DPDK_INCLUDE_DIRS}
)

# link DPDK
target_link_libraries(hft_engine PRIVATE
    ${DPDK_LIBRARIES}
)

# --- ADD THE '-g' FLAG HERE ---
target_compile_options(hft_engine PRIVATE
    ${DPDK_CFLAGS_OTHER}
    -g
    -O0
)