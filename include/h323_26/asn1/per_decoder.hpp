#pragma once
#include <h323_26/core/bit_reader.hpp>
#include <concepts>
#include <string>
#include <optional>
#include <vector>

namespace h323_26::asn1 {

    class PerDecoder {
    public:
        // Декодирование числа в заданном диапазоне [min, max]
        // Это база для H.225.0
        static Result<uint64_t> decode_constrained_integer(
            core::BitReader& reader,
            uint64_t min,
            uint64_t max);

        // Декодирование целого с поддержкой расширения (...,)
        static Result<uint64_t> decode_extensible_constrained_integer(
            core::BitReader& reader,
            uint64_t min,
            uint64_t max);

        // Читает карту опциональных полей (n бит -> std::vector<bool> или bitset)
        // В C++23 для этого идеально подходит std::vector<bool> или динамический битсет,
        // но для производительности лучше возвращать uint64_t (если полей < 64)
        static Result<uint64_t> decode_sequence_preamble(core::BitReader& reader, size_t optional_count);

        // Читает бит расширения (Extension Marker)
        static Result<bool> decode_extension_marker(core::BitReader& reader);

        // Декодирование индекса CHOICE (выбор из n вариантов)
        static Result<uint32_t> decode_choice_index(core::BitReader& reader, uint32_t num_options, bool extensible);

        // Декодирование определителя длины (Length Determinant)
        static Result<size_t> decode_length_determinant(core::BitReader& reader);

        // Декодирование строки IA5String (ASCII)
        static Result<std::string> decode_ia5_string(core::BitReader& reader, std::optional<size_t> fixed_size = std::nullopt);

        // Декодирование object identifier (OID)
        static Result<std::vector<uint32_t>> decode_oid(core::BitReader& reader);
    };

} // namespace h323_26::asn1
