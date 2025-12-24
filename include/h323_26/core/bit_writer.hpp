#pragma once
#include <h323_26/core/error.hpp>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace h323_26::core {

    class BitWriter {
    public:
        BitWriter() = default;

        // Пишет n бит (до 64)
        Result<void> write_bits(uint64_t value, size_t count);

        // Выравнивает по границе байта (дописывает нули)
        void align_to_byte();

        // Возвращает готовый буфер байтов
        const std::vector<std::byte>& data() const { return buffer_; }

    private:
        std::vector<std::byte> buffer_;
        size_t bit_offset_ = 0; // Текущий бит в последнем байте или общий счетчик
    };

} // namespace h323_26::core
