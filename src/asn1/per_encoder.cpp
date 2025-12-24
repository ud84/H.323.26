
#include <h323_26/asn1/per_encoder.hpp>
#include <bit>

namespace h323_26::asn1 {

    Result<void> PerEncoder::encode_constrained_integer(core::BitWriter& writer, uint64_t value, uint64_t min, uint64_t max) {
        // 1. Валидация входных данных
        if (value < min || value > max) {
            return std::unexpected(Error{
                ErrorCode::InvalidConstraint,
                "Value is out of ASN.1 constrained range"
                });
        }

        uint64_t range = max - min + 1;

        // 2. Если диапазон равен 1 (например, INTEGER (5..5)), число занимает 0 бит
        if (range == 1) {
            return {};
        }

        // 3. Вычисляем количество бит (так же, как в декодере)
        // std::bit_width(range - 1) дает минимальное кол-во бит для упаковки смещения
        uint32_t bits_to_write = std::bit_width(range - 1);

        // 4. Записываем смещение (value - min)
        uint64_t offset = value - min;
        return writer.write_bits(offset, bits_to_write);
    }

    Result<void> PerEncoder::encode_extension_marker(core::BitWriter& writer, bool extended) {
        return writer.write_bits(extended ? 1 : 0, 1);
    }

    Result<void> PerEncoder::encode_sequence_preamble(core::BitWriter& writer, uint64_t preamble, size_t count) {
        if (count == 0) return {};
        return writer.write_bits(preamble, count);
    }

    Result<void> PerEncoder::encode_choice_index(core::BitWriter& writer, uint32_t index, uint32_t num_options, bool extensible) {
        if (extensible) {
            // Сначала пишем бит: расширенный это выбор или нет
            // Пока наш стек поддерживает только базовые варианты
            auto res = encode_extension_marker(writer, false);
            if (!res) return res;
        }

        if (num_options <= 1) return {};

        uint32_t bits = std::bit_width(num_options - 1);
        return writer.write_bits(index, bits);
    }

    Result<void> PerEncoder::encode_extensible_constrained_integer(core::BitWriter& writer, uint64_t value, uint64_t min, uint64_t max) {
        if (value >= min && value <= max) {
            // Значение в базовом диапазоне: бит 0 + само число
            auto res = encode_extension_marker(writer, false);
            if (!res) return res;
            return encode_constrained_integer(writer, value, min, max);
        }
        else {
            // Значение в расширении: бит 1 + Normally Small Integer
            auto res = encode_extension_marker(writer, true);
            if (!res) return res;

            // Кодируем как Normally Small (бит 0 + 6 бит значения)
            // Для H.323.26 пока ограничимся 0..63 для расширений
            auto res_ns = writer.write_bits(0, 1); // Маленький формат
            if (!res_ns) return res_ns;
            return writer.write_bits(value, 6);
        }
    }

    Result<void> PerEncoder::encode_length_determinant(core::BitWriter& writer, size_t length) {
        if (length < 128) {
            // Случай 1: 0 + 7 бит длины
            return writer.write_bits(length, 8); // Старший бит будет 0 автоматически
        }
        else if (length < 16384) {
            // Случай 2: 10 + 14 бит длины
            // Устанавливаем два старших бита в 10
            uint64_t val = 0x8000 | (length & 0x3FFF);
            return writer.write_bits(val, 16);
        }
        return std::unexpected(Error{ ErrorCode::UnsupportedFeature, "Huge lengths not supported" });
    }

    Result<void> PerEncoder::encode_ia5_string(core::BitWriter& writer, std::string_view value) {
        // 1. Кодируем длину
        auto res = encode_length_determinant(writer, value.length());
        if (!res) return res;

        // 2. Выравнивание перед данными (как договорились в декодере)
        writer.align_to_byte();

        // 3. Кодируем символы (8 бит на символ в нашем упрощенном H.225 варианте)
        for (char ch : value) {
            auto char_res = writer.write_bits(static_cast<uint8_t>(ch), 8);
            if (!char_res) return char_res;
        }
        return {};
    }

    Result<void> PerEncoder::encode_oid(core::BitWriter& writer, const std::vector<uint32_t>& nodes) {
        if (nodes.size() < 2) return std::unexpected(Error{ ErrorCode::InvalidConstraint, "OID must have at least 2 nodes" });

        core::BitWriter temp_writer; // Временный буфер для BER-части OID

        // Первый байт: X*40 + Y
        (void)temp_writer.write_bits(nodes[0] * 40 + nodes[1], 8);

        // Остальные узлы (Base-128)
        for (size_t i = 2; i < nodes.size(); ++i) {
            uint32_t val = nodes[i];
            if (val == 0) {
                (void)temp_writer.write_bits(0, 8);
                continue;
            }

            // Кодируем Base-128 (7 бит + бит продолжения)
            std::vector<uint8_t> bytes;
            while (val > 0) {
                uint8_t b = val & 0x7F;
                if (!bytes.empty()) b |= 0x80; // Бит продолжения
                bytes.push_back(b);
                val >>= 7;
            }
            for (auto it = bytes.rbegin(); it != bytes.rend(); ++it) {
                (void)temp_writer.write_bits(*it, 8);
            }
        }

        // Теперь пишем PER длину и сами байты OID
        auto len_res = encode_length_determinant(writer, temp_writer.data().size());
        if (!len_res) return len_res;

        writer.align_to_byte();
        for (auto b : temp_writer.data()) {
            auto b_res = writer.write_bits(static_cast<uint8_t>(b), 8);
            if (!b_res) return b_res;
        }
        return {};
    }

} // namespace h323_26::asn1
