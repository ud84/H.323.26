#include <h323_26/core/bit_writer.hpp>

namespace h323_26::core {

    Result<void> BitWriter::write_bits(uint64_t value, size_t count) {
        if (count == 0) return {};
        if (count > 64) return std::unexpected(Error{ ErrorCode::InvalidConstraint, "Max 64 bits" });

        // Маскируем значение, чтобы не было мусора выше count
        if (count < 64) value &= (1ULL << count) - 1;

        for (size_t i = 0; i < count; ++i) {
            size_t byte_idx = bit_offset_ / 8;
            size_t bit_in_byte = bit_offset_ % 8;

            // Если буфер закончился, добавляем новый байт
            if (byte_idx >= buffer_.size()) {
                buffer_.push_back(std::byte{ 0 });
            }

            // Вытаскиваем нужный бит из value (начиная с MSB для ASN.1)
            uint64_t bit = (value >> (count - 1 - i)) & 1;

            if (bit) {
                uint8_t shift = 7 - bit_in_byte;
                buffer_[byte_idx] |= static_cast<std::byte>(1 << shift);
            }
            bit_offset_++;
        }
        return {};
    }

    void BitWriter::align_to_byte() {
        if (bit_offset_ % 8 != 0) {
            bit_offset_ += (8 - (bit_offset_ % 8));
            // ГАРАНТИРУЕМ, что вектор расширился до этого байта
            size_t needed_bytes = (bit_offset_ + 7) / 8;
            while (buffer_.size() < needed_bytes) {
                buffer_.push_back(std::byte{ 0 });
            }
        }
    }

} // namespace h323_26::core
