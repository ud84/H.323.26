#include <catch2/catch_test_macros.hpp>
#include <h323_26/h225/ras.hpp>
#include <h323_26/h225/ras_message.hpp>
#include <h323_26/asn1/per_decoder.hpp>
#include <h323_26/core/bit_reader.hpp>
#include <h323_26/core/bit_writer.hpp>
#include <vector>
#include <cstddef>

using namespace h323_26;

TEST_CASE("H.225.0 RAS: GatekeeperRequest decoding", "[h225][ras]") {
    SECTION("Simple GRQ (No extension, No optional fields, SeqNum = 1)") {
        // Разбираем по битам:
        // 1. Extension Marker (для SEQUENCE): 0 (1 бит)
        // 2. Preamble (для 1 опционального поля nonStandardData): 0 (1 бит)
        // 3. requestSeqNum (INTEGER 1..65535): 
        //    Диапазон 65535, нужно 16 бит. Значение 1 кодируется как (1-1) = 0.
        //    Биты: 00000000 00000000 (16 бит)
        //
        // Итого битовая последовательность: 00 00000000 00000000
        // Выравниваем до байта (добавляем нули в конце): 00000000 00000000 -> 0x00, 0x00

        // Нужно минимум 3 байта, чтобы вместить 1+1+16 = 18 бит
        std::vector<std::byte> data = { std::byte{0x00}, std::byte{0x00}, std::byte{0x00} };
        core::BitReader reader(data);

        auto result = h225::GatekeeperRequest::decode(reader);

        REQUIRE(result.has_value());
        CHECK(result->requestSeqNum == 1);
    }

    SECTION("GRQ with SeqNum = 257") {
        // 1. Extension: 0 (1 бит)
        // 2. Preamble: 0 (1 бит)
        // 3. SeqNum 257: (257-1) = 256. В 16 битах это 0x0100.
        // Биты: 00 00000001 00000000
        // В байтах (MSB): 
        // Первый байт: 00000000 (0x00)
        // Второй байт: 01000000 (0x40) - потому что сдвиг на 2 бита из-за заголовка
        // Третий байт: 000000...

        std::vector<std::byte> data = { std::byte{0x00}, std::byte{0x40}, std::byte{0x00} };
        core::BitReader reader(data);

        auto result = h225::GatekeeperRequest::decode(reader);

        REQUIRE(result.has_value());
        CHECK(result->requestSeqNum == 257);
    }
}

TEST_CASE("H.225.0 RAS: CHOICE decoding (RasPDU)", "[h225][ras]") {
    SECTION("Decode GatekeeperRequest (Index 0)") {
        core::BitWriter writer;
        // Кодируем индекс 0 (GRQ), extensible = true
        REQUIRE(asn1::PerEncoder::encode_choice_index(writer, 0, 2, true).has_value());

        // Кодируем само тело GRQ (SeqNum=1, Alias="H323")
        h225::GatekeeperRequest grq{ .requestSeqNum = 1, .endpointAlias = "H323" };
        REQUIRE(grq.encode(writer).has_value());

        core::BitReader reader(writer.data());
        auto result = h225::RasPDU::decode(reader);

        REQUIRE(result.has_value());
        CHECK(std::holds_alternative<h225::GatekeeperRequest>(*result));
    }

    SECTION("Decode GatekeeperConfirm (Index 1)") {
        // Бит 1 (Choice Extension): 0
        // Бит 2 (Choice Index 1): 1
        // Итого начало: 01...
        // Дальше данные GCF (те же 00 0000...01)
        // В байтах: 0100 0000 ... -> 0x40, 0x00, ...

        std::vector<std::byte> data = { std::byte{0x40}, std::byte{0x00}, std::byte{0x00} };
        core::BitReader reader(data);

        auto result = h225::RasPDU::decode(reader);
        REQUIRE(result.has_value());
        CHECK(std::holds_alternative<h225::GatekeeperConfirm>(*result));
    }
}

TEST_CASE("H.225.0 RAS: GRQ with Alias Final", "[h225][ras]") {
    std::vector<std::byte> data = { 
        std::byte{0x40}, // 01000000 (Ext=0, Pre=1, + 6 бит нулей)
        std::byte{0x00}, std::byte{0x01}, // SeqNum = 1 (в 16 битах)
        std::byte{0x04}, // Длина строки = 4
        std::byte{'H'}, std::byte{'3'}, std::byte{'2'}, std::byte{'3'} 
    };
    
    core::BitReader reader(data);
    auto result = h225::GatekeeperRequest::decode(reader);

    if (!result) {
        FAIL("Error: " << result.error().message);
    }

    CHECK(result->requestSeqNum == 1);
    REQUIRE(result->endpointAlias.has_value());
    CHECK(*result->endpointAlias == "H323");
}
TEST_CASE("H.225.0 RAS: Encode-Decode Symmetry", "[h225][ras]") {
    core::BitWriter writer;

    // Пишем ровно то, что ожидает decode() в ras.hpp:
    (void)writer.write_bits(0, 1);       // 1. Extension Marker = 0
    (void)writer.write_bits(1, 1);       // 2. Preamble (Alias present) = 1
    (void)writer.write_bits(256, 16);    // 3. SeqNum = 257 (257-1)

    // 4. IA5String "H323"
    // Сначала длина 4 (в UNALIGNED PER для 4 это 0 + 7 бит значения)
    (void)writer.write_bits(4, 8);       // Пишем 8 бит (0 + 0000100)

    writer.align_to_byte();              // Выравнивание перед данными строки
    (void)writer.write_bits('H', 8);
    (void)writer.write_bits('3', 8);
    (void)writer.write_bits('2', 8);
    (void)writer.write_bits('3', 8);

    (void)writer.write_bits(0, 8);       // ЗАПАС: целый байт в конце

    core::BitReader reader(writer.data());
    auto result = h225::GatekeeperRequest::decode(reader);

    if (!result) {
        FAIL("Symmetry failed! Error: " << result.error().message
            << " Bits left: " << reader.bits_left());
    }

    CHECK(result->requestSeqNum == 257);
    REQUIRE(result->endpointAlias.has_value());
    CHECK(*result->endpointAlias == "H323");
}

TEST_CASE("H.225.0 RAS: Full Symmetry Test (The 2026 Standard)", "[h225][ras]") {
    h225::GatekeeperRequest original{
        .requestSeqNum = 42000,
        .endpointAlias = "Modern-CPP-Stack"
    };

    core::BitWriter writer;
    auto encode_res = original.encode(writer);
    REQUIRE(encode_res.has_value());

    core::BitReader reader(writer.data());
    auto decode_res = h225::GatekeeperRequest::decode(reader);

    REQUIRE(decode_res.has_value());
    CHECK(decode_res->requestSeqNum == original.requestSeqNum);
    REQUIRE(decode_res->endpointAlias.has_value());
    CHECK(*decode_res->endpointAlias == *original.endpointAlias);
}

TEST_CASE("H.225.0 RAS: CHOICE Symmetry (RasPDU)", "[h225][ras]") {
    core::BitWriter writer;

    // Кодируем индекс 1 (GatekeeperConfirm)
    auto res = asn1::PerEncoder::encode_choice_index(writer, 1, 2, true);
    REQUIRE(res.has_value());

    // Допишем SeqNum для GCF (просто чтобы данные были)
    (void)asn1::PerEncoder::encode_extension_marker(writer, false);
    (void)asn1::PerEncoder::encode_sequence_preamble(writer, 0, 1);
    (void)asn1::PerEncoder::encode_constrained_integer(writer, 999, 1, 65535);

    core::BitReader reader(writer.data());
    auto decoded_pdu = h225::RasPDU::decode(reader);

    REQUIRE(decoded_pdu.has_value());
    // Должен быть вариант 1 (GatekeeperConfirm)
    CHECK(std::holds_alternative<h225::GatekeeperConfirm>(*decoded_pdu));
}
