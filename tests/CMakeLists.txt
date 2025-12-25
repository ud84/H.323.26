add_executable(unit_tests 
    unit/test_bit_reader.cpp
    unit/test_per_decoder.cpp
    unit/test_h225_ras.cpp
)

target_link_libraries(unit_tests 
    PRIVATE 
    h323_26_lib
    Catch2::Catch2WithMain
)

include(CTest)
add_test(NAME h323_unit_tests COMMAND unit_tests)

option(BUILD_COMPLIANCE_TESTS "Build compliance generation tools" ON)

if(BUILD_COMPLIANCE_TESTS)
    add_executable(gen_h225_ras_grq compliance/H225_RAS_GRQ/main.cpp)
    target_link_libraries(gen_h225_ras_grq PRIVATE h323_26_lib)
endif()
