# Создаем библиотеку
add_library(h323_26_lib
    core/bit_reader.cpp
    core/bit_writer.cpp
    asn1/per_decoder.cpp
    asn1/per_encoder.cpp
)

# Указываем пути к заголовкам
target_include_directories(h323_26_lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Если планируешь использовать только заголовки (header-only) в будущем, 
# можно поменять add_library на INTERFACE, но для стека лучше STATIC или SHARED.
set_target_properties(h323_26_lib PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "h323_26"
)
