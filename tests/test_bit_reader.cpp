#include <catch2/catch_test_macros.hpp>
#include <h323_26/core/bit_reader.hpp>
#include <vector>
#include <cstddef>

using namespace h323_26;

TEST_CASE("BitReader basic functionality", "[core]") {
    // 0xAA = 10101010, 0xFF = 11111111
    std::vector<std::byte> data = { std::byte{0xAA}, std::byte{0xFF} };
    BitReader reader(data);

    SECTION("Read bits within single byte") {
        auto val = reader.read_bits(1);
        REQUIRE(val.has_value());
        CHECK(val.value() == 1); // Первый бит 0xAA

        val = reader.read_bits(3);
        REQUIRE(val.has_value());
        CHECK(val.value() == 0b010); // Следующие 3 бита: 010
    }

    SECTION("Read bits crossing byte boundary") {
        reader.read_bits(6); // Сместились на 6 бит
        // Осталось 2 бита от 0xAA (10) и берем 2 бита от 0xFF (11)
        auto val = reader.read_bits(4);
        REQUIRE(val.has_value());
        CHECK(val.value() == 0b1011);
    }

    SECTION("Check EndOfStream") {
        reader.read_bits(16);
        auto val = reader.read_bits(1);
        CHECK_FALSE(val.has_value());
        CHECK(val.error().code == ErrorCode::EndOfStream);
    }
}
