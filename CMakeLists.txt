cmake_minimum_required(VERSION 4.4.3)
project(Combatant VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COMBATANT_COVERAGE "Build with gcov instrumentation" ON)

add_subdirectory(src)
add_subdirectory(tests)

add_executable(geoscape src/app/main.cpp)
if(WIN32)
    target_sources(geoscape PRIVATE src/app/present_win32.cpp)
    target_link_libraries(geoscape PRIVATE gdi32 user32 msimg32 gdiplus)
endif()
target_link_libraries(geoscape PRIVATE combatant)
target_compile_definitions(geoscape PRIVATE "COMBATANT_DATA_DIR=\"${CMAKE_SOURCE_DIR}/data\"")