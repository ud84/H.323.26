add_executable(unit_tests 
    test_bit_reader.cpp
)

target_link_libraries(unit_tests 
    PRIVATE 
    h323_26_lib
    Catch2::Catch2WithMain
)

include(CTest)
add_test(NAME h323_unit_tests COMMAND unit_tests)
