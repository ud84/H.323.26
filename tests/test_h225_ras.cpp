#include <catch2/catch_test_macros.hpp>
#include <h323_26/h225/ras.hpp>
#include <h323_26/h225/ras_message.hpp>
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
        // Бит 1 (Choice Extension): 0
        // Бит 2 (Choice Index 0..1): 0 (так как вариантов 2, нужен 1 бит)
        // Итого начало: 00...
        // Дальше данные GRQ (Extension 0, Preamble 0, SeqNum 1 = 00 00000000 00000000)
        // Суммарно: 0000 00000000 00000000 -> 0x00, 0x00, 0x00

        std::vector<std::byte> data = { std::byte{0x00}, std::byte{0x00}, std::byte{0x00} };
        core::BitReader reader(data);

        auto result = h225::RasPDU::decode(reader);
        REQUIRE(result.has_value());
        // Проверяем, что внутри именно GatekeeperRequest
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
