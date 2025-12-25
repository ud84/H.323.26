#include <catch2/catch_test_macros.hpp>
#include <h323_26/h225/ras.hpp>
#include <h323_26/h225/ras_message.hpp>
#include <h323_26/asn1/per_decoder.hpp>
#include <h323_26/core/bit_reader.hpp>
#include <h323_26/core/bit_writer.hpp>
#include <vector>
#include <cstddef>

using namespace h323_26;

TEST_CASE("H.225.0 RAS: Global Symmetry", "[h225]") {
    h225::GatekeeperRequest grq{
        .requestSeqNum = 1234,
        .protocolIdentifier = {0, 0, 8, 2250, 0, 7}
    };

    core::BitWriter writer;
    REQUIRE(grq.encode(writer).has_value());

    core::BitReader reader(writer.data());
    auto decoded = h225::GatekeeperRequest::decode(reader);

    REQUIRE(decoded.has_value());
    CHECK(decoded->requestSeqNum == 1234);
    CHECK(decoded->protocolIdentifier == std::vector<uint32_t>{0, 0, 8, 2250, 0, 7});
    //REQUIRE(decoded->endpointAlias.has_value());
    //CHECK(*decoded->endpointAlias == "H.323.26-Terminal");
}

TEST_CASE("H.225.0 RAS: Optional field absence", "[h225]") {
    h225::GatekeeperRequest grq{
        .requestSeqNum = 1,
        .protocolIdentifier = {0, 0, 8, 2250, 0, 7},
        .endpointAlias = std::nullopt // Поля нет
    };

    core::BitWriter writer;
    REQUIRE(grq.encode(writer).has_value());

    core::BitReader reader(writer.data());
    auto decoded = h225::GatekeeperRequest::decode(reader);

    REQUIRE(decoded.has_value());
    CHECK(decoded->requestSeqNum == 1);
    CHECK_FALSE(decoded->endpointAlias.has_value()); // Проверяем отсутствие
}

