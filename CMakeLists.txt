cmake_minimum_required(VERSION 3.16)
project(ConnectedVehicles LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Gui Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Gui Core Network)

add_executable(ConnectedVehicles
    src/main.cpp
    src/map_view.cpp
    include/map_view.h
)

target_include_directories(ConnectedVehicles PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(ConnectedVehicles
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
)

message(STATUS "Qt version: ${Qt${QT_VERSION_MAJOR}_VERSION}")

