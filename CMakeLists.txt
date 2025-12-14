cmake_minimum_required(VERSION 3.16)
project(ConnectedVehicles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


# ===============================
#  Dependencies
# ===============================

# Boost (for graph structures)
# Help FindBoost locate MSYS2 MinGW64 installation when used on Windows
if (WIN32)
    # Adjust these paths if your MSYS2 is installed elsewhere
    set(BOOST_ROOT "C:/msys64/mingw64" CACHE PATH "Boost root (MSYS2 mingw64)")
    set(BOOST_INCLUDEDIR "C:/msys64/mingw64/include" CACHE PATH "Boost include dir")
    set(BOOST_LIBRARYDIR "C:/msys64/mingw64/lib" CACHE PATH "Boost library dir")
    set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "Use static Boost libraries")
    set(Boost_USE_MULTITHREADED ON CACHE BOOL "Use multithreaded Boost")
    set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "Use static runtime for Boost")
endif()

# Request the graph component explicitly (used by this project)
find_package(Boost REQUIRED COMPONENTS graph)


# Qt (for GUI visualization)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui Core Network Svg Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui Core Network Svg Concurrent)


# PROJ (geographic projection library)
# On Unix systems the system include paths are under /usr/include.
# On Windows/MSYS2 we rely on CMAKE_PREFIX_PATH (/mingw64) so avoid hardcoded /usr/include.
if (UNIX)
    include_directories(/usr/include)
endif()

link_libraries(proj)

# libosmium
# Only add the Linux-specific include path on UNIX. On MSYS2/Windows the
# libraries/headers should be found via CMAKE_PREFIX_PATH (e.g. /mingw64).
if (UNIX)
    include_directories(/usr/include/osmium)
endif()


# ===============================
#  Source files
# ===============================
file(GLOB SOURCES src/*.cpp include/*.h)

add_executable(ConnectedVehicles ${SOURCES})

# Include headers
target_include_directories(ConnectedVehicles PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


# ===============================
#  Linking
# ===============================
target_link_libraries(ConnectedVehicles
    ${Boost_LIBRARIES}
    proj
    bz2
    z
    expat
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Concurrent
)

message(STATUS "Qt version: ${Qt${QT_VERSION_MAJOR}_VERSION}")

