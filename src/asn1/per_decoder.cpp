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

} // namespace h323_26::asn1
