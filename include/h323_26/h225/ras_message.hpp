#pragma once

#include <h323_26/h225/ras.hpp>
#include <variant>

namespace h323_26::h225 {

    using RasMessage = std::variant<GatekeeperRequest, GatekeeperConfirm>;

    struct RasPDU {
        static Result<void> encode(core::BitWriter& writer, const RasMessage& msg) {
            // 1. Кодируем индекс CHOICE. 
            // В H.225.0 для RasMessage: GRQ - это индекс 3.
            // Используем 33 варианта (как в базе v7), это даст 6 бит.
            uint32_t index = 0;
            if (std::holds_alternative<GatekeeperRequest>(msg)) index = 3;
            else if (std::holds_alternative<GatekeeperConfirm>(msg)) index = 1;

            // Пишем CHOICE index (extensible=true, num_options=33)
            auto res = asn1::PerEncoder::encode_choice_index(writer, index, 33, true);
            if (!res) return res;

            // 2. Кодируем тело выбранного сообщения
            return std::visit([&](auto&& arg) {
                return arg.encode(writer);
                }, msg);
        }
    };

} // namespace h323_26::h225
