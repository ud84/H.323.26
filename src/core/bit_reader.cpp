#include <h323_26/core/bit_reader.hpp>
#include <algorithm>

namespace h323_26 {

    Result<uint64_t> BitReader::read_bits(size_t count) {
        if (count > 64) {
            return std::unexpected(Error{ ErrorCode::InvalidConstraint, "Max 64 bits" });
        }
        if (count > bits_left()) {
            return std::unexpected(Error{ ErrorCode::EndOfStream, "Not enough bits" });
        }

        uint64_t result = 0;

        while (count > 0) {
            size_t byte_idx = bit_offset_ / 8;
            size_t bit_in_byte_offset = bit_offset_ % 8;
            size_t bits_to_read_in_current_byte = std::min(count, 8 - bit_in_byte_offset);

            // Достаем нужные биты из текущего байта
            uint8_t byte = static_cast<uint8_t>(data_[byte_idx]);
            uint8_t mask = (1 << bits_to_read_in_current_byte) - 1;
            uint8_t shift = 8 - bit_in_byte_offset - bits_to_read_in_current_byte;

            uint64_t val = (byte >> shift) & mask;

            // Сдвигаем результат и добавляем новые биты
            result = (result << bits_to_read_in_current_byte) | val;

            bit_offset_ += bits_to_read_in_current_byte;
            count -= bits_to_read_in_current_byte;
        }

        return result;
    }

    void BitReader::align_to_byte() {
        if (bit_offset_ % 8 != 0) {
            bit_offset_ += (8 - (bit_offset_ % 8));
        }
    }

} // namespace h323_26
