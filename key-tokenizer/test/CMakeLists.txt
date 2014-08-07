add_executable(test_element element.cxx)
add_executable(test_action action.cxx)

add_test(test_element test_element)
add_test(test_action test_action)

find_package(Boost COMPONENTS test_exec_monitor unit_test_framework thread system REQUIRED)

target_link_libraries(test_element ${Boost_LIBRARIES} keytokenizer)
target_link_libraries(test_action ${Boost_LIBRARIES} keytokenizer)
