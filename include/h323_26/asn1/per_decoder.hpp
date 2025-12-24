#pragma once
#include <h323_26/core/bit_reader.hpp>
#include <concepts>

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
    };

} // namespace h323_26::asn1
