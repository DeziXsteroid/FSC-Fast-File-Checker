cmake_minimum_required(VERSION 3.16)
project(FileSearch LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ВАЖНО: на Windows собираем как GUI-приложение (без консоли)
if (WIN32)
    add_executable(FileSearch WIN32 main.cpp)
else()
    add_executable(FileSearch main.cpp)
endif()

target_link_libraries(FileSearch PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
