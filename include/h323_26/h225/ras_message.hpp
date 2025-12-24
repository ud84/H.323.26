#pragma once

#include <h323_26/h225/ras.hpp>
#include <variant>

namespace h323_26::h225 {

    // Заглушка для второго типа сообщения
    struct GatekeeperConfirm {
        uint16_t requestSeqNum;
        static Result<GatekeeperConfirm> decode(core::BitReader& reader) {
            // Логика аналогична GRQ для этого примера
            return GatekeeperRequest::decode(reader).transform([](auto grq) {
                return GatekeeperConfirm{ grq.requestSeqNum };
                });
        }
    };

    using RasMessage = std::variant<GatekeeperRequest, GatekeeperConfirm>;

    struct RasPDU {
        static Result<RasMessage> decode(core::BitReader& reader) {
            // RAS PDU — это extensible CHOICE. 
            // В нашем примере 2 варианта: GRQ (0) и GCF (1)
            return asn1::PerDecoder::decode_choice_index(reader, 2, true)
                .and_then([&](uint32_t index) -> Result<RasMessage> {
                switch (index) {
                case 0: return GatekeeperRequest::decode(reader);
                case 1: return GatekeeperConfirm::decode(reader);
                default: return std::unexpected(Error{ ErrorCode::UnsupportedFeature, "Unknown RAS message index" });
                }
                    });
        }
    };

} // namespace h323_26::h225
