cmake_minimum_required (VERSION 2.6)
project (VCC)

enable_testing()

find_package(Boost COMPONENTS thread program_options system REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(3pp/boost_process)

find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

include_directories(src)
add_subdirectory(key-tokenizer)
include_directories(boost_process)

include_directories(key-tokenizer/include)

add_library(th
    src/cmd-registry.cxx
    src/fmt.cxx
    src/hub.cxx
    src/process.cxx
    src/rs232_client.cxx
    src/signal_exit.cxx
    src/tty.cxx
)

add_executable(termhub src/main.cxx)

add_executable(key-printer src/key-printer.cxx)

install(TARGETS termhub key-printer DESTINATION bin)

target_link_libraries(
    termhub
    th
    keytokenizer
    ${CMAKE_THREAD_LIBS_INIT}
    ${Boost_LIBRARIES}
)

target_link_libraries(
    key-printer
    th
    keytokenizer
    ${Boost_LIBRARIES}
)

add_subdirectory(test)
