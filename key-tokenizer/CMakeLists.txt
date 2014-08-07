cmake_minimum_required (VERSION 2.6)
project (VCC)

enable_testing()

find_package(Boost COMPONENTS thread system REQUIRED)
include_directories(include)
include_directories(${Boost_INCLUDE_DIRS})

add_library(
    keytokenizer
    src/action.cxx
    src/element.cxx
    src/inventory.cxx
)

find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

add_subdirectory(test)


