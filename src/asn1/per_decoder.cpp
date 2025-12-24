#include <h323_26/asn1/per_decoder.hpp>
#include <bit>
#include <algorithm>

namespace h323_26::asn1 {

    // Вспомогательный метод (Normally Small Integer)
    static Result<uint64_t> decode_normally_small_integer(core::BitReader& reader) {
        auto bit = reader.read_bits(1);
        if (!bit) return std::unexpected(bit.error());

        if (*bit == 0) {
            // Если 0, то следующие 6 бит — это само число (0..63)
            return reader.read_bits(6);
        }
        else {
            // Если 1, то это "длинный" вариант (пока кинем ошибку или реализуем позже)
            return std::unexpected(Error{ ErrorCode::UnsupportedFeature, "Long normally small integer not implemented" });
        }
    }

    Result<uint64_t> PerDecoder::decode_constrained_integer(
        core::BitReader& reader,
        uint64_t min,
        uint64_t max)
    {
        if (max < min) {
            return std::unexpected(Error{ ErrorCode::InvalidConstraint, "Max < Min in integer constraint" });
        }

        uint64_t range = max - min + 1;

        // Если диапазон равен 1 (например, INTEGER (5..5)), число в потоке занимает 0 бит
        if (range == 1) {
            return min;
        }

        // Вычисляем количество бит, необходимых для упаковки диапазона
        // std::bit_width(range - 1) дает количество бит для представления числа от 0 до range-1
        uint32_t bits_to_read = std::bit_width(range - 1);

        auto raw_value = reader.read_bits(bits_to_read);
        if (!raw_value) {
            return std::unexpected(raw_value.error());
        }

        // В PER записывается смещение от минимального значения
        uint64_t final_value = min + *raw_value;

        if (final_value > max) {
            return std::unexpected(Error{ ErrorCode::InvalidConstraint, "Decoded value exceeds max constraint" });
        }

        return final_value;
    }

    Result<uint64_t> PerDecoder::decode_extensible_constrained_integer(
        core::BitReader& reader,
        uint64_t min,
        uint64_t max)
    {
        auto extension_marker = reader.read_bits(1);
        if (!extension_marker) return std::unexpected(extension_marker.error());

        if (*extension_marker == 0) {
            // Мы в пределах известного диапазона
            return decode_constrained_integer(reader, min, max);
        }
        else {
            // Мы вышли за пределы (Extension bit = 1)
            // В PER это обычно кодируется как "unconstrained" или "normally small"
            // Для простоты сейчас реализуем чтение "Normally Small Integer" (X.691, 10.6)
            // Это часто используется для индексов в таблицах расширений.
            return decode_normally_small_integer(reader);
        }
    }

    Result<uint64_t> PerDecoder::decode_sequence_preamble(core::BitReader& reader, size_t optional_count) {
        if (optional_count == 0) return 0;
        if (optional_count > 64) {
            return std::unexpected(Error{ ErrorCode::UnsupportedFeature, "Too many optional fields" });
        }
        return reader.read_bits(optional_count);
    }

    Result<bool> PerDecoder::decode_extension_marker(core::BitReader& reader) {
        auto bit = reader.read_bits(1);
        if (!bit) return std::unexpected(bit.error());
        return *bit == 1;
    }

    Result<uint32_t> PerDecoder::decode_choice_index(core::BitReader& reader, uint32_t num_options, bool extensible) {
        if (extensible) {
            auto is_extended = reader.read_bits(1);
            if (!is_extended) return std::unexpected(is_extended.error());
            if (*is_extended) {
                return std::unexpected(Error{ ErrorCode::UnsupportedFeature, "Extension choices not implemented yet" });
            }
        }

        if (num_options <= 1) return 0;

        // Количество бит = log2(num_options)
        uint32_t bits = std::bit_width(num_options - 1);
        auto index = reader.read_bits(bits);
        if (!index) return std::unexpected(index.error());

        return static_cast<uint32_t>(*index);
    }

    Result<size_t> PerDecoder::decode_length_determinant(core::BitReader& reader) {
        // В UNALIGNED PER для длин < 128 выравнивание не всегда нужно, 
        // но для простоты и соответствия большинству полей H.225:
        auto first_bit = reader.read_bits(1);
        if (!first_bit) return std::unexpected(first_bit.error());

        if (*first_bit == 0) {
            // Случай 1: длина 0..127 (7 бит)
            auto val = reader.read_bits(7);
            if (!val) return std::unexpected(val.error());
            return static_cast<size_t>(*val);
        }

        auto second_bit = reader.read_bits(1);
        if (!second_bit) return std::unexpected(second_bit.error());

        if (*second_bit == 0) {
            // Случай 2: длина 128..16383 (14 бит)
            auto val = reader.read_bits(14);
            if (!val) return std::unexpected(val.error());
            return static_cast<size_t>(*val);
        }

        return std::unexpected(Error{ ErrorCode::UnsupportedFeature, "Huge lengths (>16k) not implemented" });
    }

    Result<std::string> PerDecoder::decode_ia5_string(core::BitReader& reader, std::optional<size_t> fixed_size) {
        size_t length = 0;
        if (fixed_size) {
            length = *fixed_size;
        }
        else {
            auto decoded_len = decode_length_determinant(reader);
            if (!decoded_len) return std::unexpected(decoded_len.error());
            length = *decoded_len;
        }

        if (length == 0) return "";

        // IA5String символы занимают 7 бит в UNALIGNED PER, 
        // НО если строка длинная или не ограничена, они часто пакуются в 8-битные октеты.
        // Для H.225 Alias обычно используются 8-битные выровненные октеты.
        reader.align_to_byte();

        std::string res;
        res.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            auto ch = reader.read_bits(8);
            if (!ch) return std::unexpected(ch.error());
            res.push_back(static_cast<char>(*ch));
        }
        return res;
    }

    Result<std::vector<uint32_t>> PerDecoder::decode_oid(core::BitReader& reader) {
        auto length_res = decode_length_determinant(reader);
        if (!length_res) return std::unexpected(length_res.error());

        size_t len = *length_res;
        if (len == 0) return std::vector<uint32_t>{};

        // OID всегда выровнен по байту в PER
        reader.align_to_byte();

        std::vector<uint32_t> nodes;
        // Первый байт: X*40 + Y
        auto first_byte_res = reader.read_bits(8);
        if (!first_byte_res) return std::unexpected(first_byte_res.error());

        uint8_t first = static_cast<uint8_t>(*first_byte_res);
        nodes.push_back(first / 40);
        nodes.push_back(first % 40);

        size_t bytes_read = 1;
        while (bytes_read < len) {
            uint32_t node_val = 0;
            uint8_t b;
            do {
                auto b_res = reader.read_bits(8);
                if (!b_res) return std::unexpected(b_res.error());
                b = static_cast<uint8_t>(*b_res);
                node_val = (node_val << 7) | (b & 0x7F);
                bytes_read++;
            } while (b & 0x80); // Пока 8-й бит равен 1
            nodes.push_back(node_val);
        }
        return nodes;
    }

} // namespace h323_26::asn1
