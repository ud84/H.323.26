#pragma once
#include <h323_26/core/bit_writer.hpp>
#include <string_view>

namespace h323_26::asn1 {

    class PerEncoder {
    public:
        static Result<void> encode_constrained_integer(core::BitWriter& writer, uint64_t value, uint64_t min, uint64_t max);

        // Кодирование расширяемого целого
        static Result<void> encode_extensible_constrained_integer(core::BitWriter& writer, uint64_t value, uint64_t min, uint64_t max);

        // Кодирование маркера расширения (1 бит)
        static Result<void> encode_extension_marker(core::BitWriter& writer, bool extended);

        // Кодирование преамбулы SEQUENCE (n бит)
        static Result<void> encode_sequence_preamble(core::BitWriter& writer, uint64_t preamble, size_t count);

        // Кодирование индекса CHOICE
        static Result<void> encode_choice_index(core::BitWriter& writer, uint32_t index, uint32_t num_options, bool extensible);      

        static Result<void> encode_length_determinant(core::BitWriter& writer, size_t length);
        static Result<void> encode_ia5_string(core::BitWriter& writer, std::string_view value);
        static Result<void> encode_oid(core::BitWriter& writer, const std::vector<uint32_t>& nodes);
    };

} // namespace h323_26::asn1
