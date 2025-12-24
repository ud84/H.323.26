#include <catch2/catch_test_macros.hpp>
#include <h323_26/asn1/per_decoder.hpp>
#include <vector>

using namespace h323_26;
using namespace h323_26::asn1;

TEST_CASE("PER Constrained Integer Decoding", "[asn1]") {
    SECTION("Small range (fits in 2 bits)") {
        // Допустим, кодируем INTEGER (1..4), значение 3.
        // Диапазон 4, значит нужно 2 бита. Значение 3 кодируется как (3-1) = 2 (бинарно 10)
        std::vector<std::byte> data = { std::byte{0b10'000000} };
        core::BitReader reader(data);

        auto res = PerDecoder::decode_constrained_integer(reader, 1, 4);
        REQUIRE(res.has_value());
        CHECK(*res == 3);
    }

    SECTION("Full byte range (0..255)") {
        // INTEGER (0..255), значение 128 (0x80)
        std::vector<std::byte> data = { std::byte{0x80} };
        core::BitReader reader(data);

        auto res = PerDecoder::decode_constrained_integer(reader, 0, 255);
        REQUIRE(res.has_value());
        CHECK(*res == 128);
    }

    SECTION("Fixed value (0 bits)") {
        // INTEGER (10..10) - не должно читать ни одного бита
        std::vector<std::byte> data = { std::byte{0xFF} };
        core::BitReader reader(data);

        auto res = PerDecoder::decode_constrained_integer(reader, 10, 10);
        REQUIRE(res.has_value());
        CHECK(*res == 10);
        CHECK(reader.bits_left() == 8); // Убеждаемся, что указатель не сдвинулся
    }
}

TEST_CASE("PER Extensible Constrained Integer", "[asn1]") {
    SECTION("Extension bit is 0 (Normal range)") {
        // Мы хотим прочитать INTEGER (1..4) со значением 3.
        // Бит расширения: 0
        // Значение (3-1 = 2): 10 в 2 битах
        // Итого: 0 10 -> 01000000 (0x40)
        std::vector<std::byte> data = { std::byte{0x40} };
        core::BitReader reader(data);

        auto res = PerDecoder::decode_extensible_constrained_integer(reader, 1, 4);
        REQUIRE(res.has_value());
        CHECK(*res == 3);
        CHECK(reader.bits_left() == 5); // 1 бит маркер + 2 бита значение = 3 бита съедено
    }

    SECTION("Extension bit is 1 (Normally Small Integer)") {
        // Мы хотим прочитать число за пределами диапазона.
        // Бит расширения: 1
        // Normally Small Integer (бит 0 + 6 бит значения): 0 000111 (число 7)
        // Итого: 1 0 000111 -> 10000111 (0x87)
        std::vector<std::byte> data = { std::byte{0x87} };
        core::BitReader reader(data);

        auto res = PerDecoder::decode_extensible_constrained_integer(reader, 1, 4);
        REQUIRE(res.has_value());
        CHECK(*res == 7);
        CHECK(reader.bits_left() == 0); // 1 бит маркер + 1 бит префикс + 6 бит = 8 бит
    }
}
