#pragma once
#include <h323_26/asn1/per_decoder.hpp>
#include <optional>
#include <vector>

namespace h323_26::h225 {

    struct GatekeeperRequest {
        uint16_t requestSeqNum;
        // Для простоты пока опустим сложный Object ID и NonStandardData
        // Но добавим их как заглушки, чтобы понять логику SEQUENCE

        static Result<GatekeeperRequest> decode(core::BitReader& reader) {
            return asn1::PerDecoder::decode_extension_marker(reader)
                .and_then([&](bool /*ext*/) {
                return asn1::PerDecoder::decode_sequence_preamble(reader, 1);
                    })
                .and_then([&](uint64_t /*preamble*/) {
                return asn1::PerDecoder::decode_constrained_integer(reader, 1, 65535);
                    })
                .transform([](uint64_t seq_num) {
                // transform вызывается только если все шаги выше вернули value
                // он автоматически оборачивает результат в Result<GatekeeperRequest>
                return GatekeeperRequest{ .requestSeqNum = static_cast<uint16_t>(seq_num) };
                    });
        }

        /*static Result<GatekeeperRequest> decode(core::BitReader& reader) {
            // 1. SEQUENCE в H.225 почти всегда расширяемый. Читаем маркер (...)
            auto is_extended = asn1::PerDecoder::decode_extension_marker(reader);
            if (!is_extended) return std::unexpected(is_extended.error());

            // 2. Читаем преамбулу для OPTIONAL полей. 
            // Допустим, у нас только 1 опциональное поле (nonStandardData)
            auto preamble = asn1::PerDecoder::decode_sequence_preamble(reader, 1);
            if (!preamble) return std::unexpected(preamble.error());

            // 3. Декодируем обязательные поля
            auto seq_num = asn1::PerDecoder::decode_constrained_integer(reader, 1, 65535);
            if (!seq_num) return std::unexpected(seq_num.error());

            // 4. Если в преамбуле бит был 1, тут бы мы читали NonStandardData
            bool has_non_standard = (*preamble & 0x01);
            if (has_non_standard) {
                // Пропускаем или парсим дальше (пока пропустим)
            }

            return GatekeeperRequest{
                .requestSeqNum = static_cast<uint16_t>(*seq_num)
            };
        }*/
    };

} // namespace h323_26::h225
