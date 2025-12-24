#pragma once

#include <h323_26/core/error.hpp>

#include <cstdint>
#include <span>
#include <cstddef>

namespace h323_26::core {

    class BitReader {
    public:
        explicit BitReader(std::span<const std::byte> data) : data_(data) {}

        // Reads N bits and returns them as a uint64_t
        Result<uint64_t> read_bits(size_t count);

        // Peeks N bits without advancing the internal cursor
        Result<uint64_t> peek_bits(size_t count) const;

        // Skips N bits
        Result<void> skip_bits(size_t count);

        // Aligns the cursor to the next byte boundary (crucial for some H.323 fields)
        void align_to_byte();

        [[nodiscard]] size_t bits_left() const {
            return (data_.size() * 8) - bit_offset_;
        }

    private:
        std::span<const std::byte> data_;
        size_t bit_offset_ = 0; // Global bit offset from the start
    };

} // namespace h323_26
