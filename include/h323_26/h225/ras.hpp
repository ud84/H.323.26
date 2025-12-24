#pragma once

#include <h323_26/asn1/per_decoder.hpp>
#include <h323_26/asn1/per_encoder.hpp>
#include <h323_26/core/bit_writer.hpp>
#include <optional>
#include <string>

namespace h323_26::h225 {

    struct GatekeeperRequest {
        uint16_t requestSeqNum;
        std::optional<std::string> endpointAlias;

        static Result<GatekeeperRequest> decode(core::BitReader& reader) {
            // Промежуточная структура для цепочки and_then
            struct DecodeState {
                uint16_t seq;
                uint64_t preamble;
            };

            return asn1::PerDecoder::decode_extension_marker(reader)
                .and_then([&](bool /*ext*/) {
                // Читаем 1 бит преамбулы для endpointAlias
                return asn1::PerDecoder::decode_sequence_preamble(reader, 1);
                    })
                .and_then([&](uint64_t preamble) {
                return asn1::PerDecoder::decode_constrained_integer(reader, 1, 65535)
                    .transform([preamble](uint64_t seq) {
                    return DecodeState{ static_cast<uint16_t>(seq), preamble };
                        });
                    })
                .and_then([&](DecodeState state) -> Result<GatekeeperRequest> {
                std::optional<std::string> alias;

                // Если первый бит преамбулы установлен (endpointAlias присутствует)
                if (state.preamble == 1) {
                    auto str_res = asn1::PerDecoder::decode_ia5_string(reader);
                    if (!str_res) return std::unexpected(str_res.error());
                    alias = std::move(*str_res);
                }

                return GatekeeperRequest{
                    .requestSeqNum = state.seq,
                    .endpointAlias = std::move(alias)
                };
                    });
        }

        Result<void> encode(core::BitWriter& writer) const {
            // 1. Extension Marker
            auto res = asn1::PerEncoder::encode_extension_marker(writer, false);
            if (!res) return res;

            // 2. Преамбула: 1 бит. ОЧЕНЬ ВАЖНО: 1 если Alias есть, 0 если нет.
            res = asn1::PerEncoder::encode_sequence_preamble(writer, endpointAlias.has_value() ? 1 : 0, 1);
            if (!res) return res;

            // 3. SeqNum
            res = asn1::PerEncoder::encode_constrained_integer(writer, requestSeqNum, 1, 65535);
            if (!res) return res;

            // 4. Alias
            if (endpointAlias) {
                res = asn1::PerEncoder::encode_ia5_string(writer, *endpointAlias);
                if (!res) return res;
            }
            return {};
        }
    };

} // namespace h323_26::h225
