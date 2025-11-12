cmake_minimum_required(VERSION 3.16)
project(ConnectedVehicles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


# ===============================
#  Dependencies
# ===============================

# Boost (for graph structures)
find_package(Boost REQUIRED)


# Qt (for GUI visualization with QtLocation)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui Core Network Quick Qml Location Positioning)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui Core Network Quick Qml Location Positioning)


# PROJ (geographic projection library)
include_directories(/usr/include)
link_libraries(proj)

# libosmium
include_directories(/usr/include/osmium)


# ===============================
#  Source files
# ===============================
file(GLOB SOURCES src/*.cpp include/*.h)

# QML resources
qt_add_resources(QML_RESOURCES resources.qrc)

add_executable(ConnectedVehicles ${SOURCES} ${QML_RESOURCES})

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
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Location
    Qt${QT_VERSION_MAJOR}::Positioning
)

message(STATUS "Qt version: ${Qt${QT_VERSION_MAJOR}_VERSION}")

