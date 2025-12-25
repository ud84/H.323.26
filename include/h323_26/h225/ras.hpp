#pragma once

#include <h323_26/asn1/per_decoder.hpp>
#include <h323_26/asn1/per_encoder.hpp>
#include <h323_26/core/bit_writer.hpp>
#include <optional>
#include <string>

namespace h323_26::h225 {

    struct GatekeeperRequest {
        uint16_t requestSeqNum;
        std::vector<uint32_t> protocolIdentifier;
        std::optional<std::string> endpointAlias;

        static Result<GatekeeperRequest> decode(core::BitReader& reader) {
            struct DecodeState {
                uint16_t seq;
                uint64_t preamble;
                std::vector<uint32_t> oid;
            };

            return asn1::PerDecoder::decode_extension_marker(reader)
                .and_then([&](bool) {
                return asn1::PerDecoder::decode_sequence_preamble(reader, 1);
                    })
                .and_then([&](uint64_t preamble) {
                return asn1::PerDecoder::decode_constrained_integer(reader, 1, 65535)
                    .transform([preamble](uint64_t seq) {
                    return std::pair{ static_cast<uint16_t>(seq), preamble };
                        });
                    })
                .and_then([&](auto pair) {
                return asn1::PerDecoder::decode_oid(reader)
                    .transform([&pair](std::vector<uint32_t> oid) {
                    return DecodeState{ pair.first, pair.second, std::move(oid) };
                        });
                    })
                .and_then([&](DecodeState state) -> Result<GatekeeperRequest> {
                std::optional<std::string> alias;
                if (state.preamble == 1) {
                    auto str_res = asn1::PerDecoder::decode_ia5_string(reader);
                    if (!str_res) return std::unexpected(str_res.error());
                    alias = std::move(*str_res);
                }

                return GatekeeperRequest{
                    .requestSeqNum = state.seq,
                    .protocolIdentifier = std::move(state.oid),
                    .endpointAlias = std::move(alias)
                };
                    });
        }

        Result<void> encode(core::BitWriter& writer) const {
            // Extension Marker
            if (auto res = asn1::PerEncoder::encode_extension_marker(writer, false); !res) return res;

            // Преамбула (endpointAlias)
            if (auto res = asn1::PerEncoder::encode_sequence_preamble(writer, endpointAlias.has_value() ? 1 : 0, 1); !res) return res;

            // SeqNum
            if (auto res = asn1::PerEncoder::encode_constrained_integer(writer, requestSeqNum, 1, 65535); !res) return res;

            // OID (Обязательное поле)
            if (auto res = asn1::PerEncoder::encode_oid(writer, protocolIdentifier); !res) return res;

            // Alias
            if (endpointAlias) {
                if (auto res = asn1::PerEncoder::encode_ia5_string(writer, *endpointAlias); !res) return res;
            }
            return {};
        }
    };

} // namespace h323_26::h225
