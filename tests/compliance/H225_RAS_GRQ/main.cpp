#include <h323_26/h225/ras.hpp>
#include <h323_26/h225/ras_message.hpp>
#include <h323_26/core/bit_writer.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <iomanip>

using namespace h323_26;

int main() {
    h225::GatekeeperRequest grq{
        .requestSeqNum = 1,
        .protocolIdentifier = {0, 0, 8, 2250, 0, 7}
    };
    h225::RasMessage msg = grq;

    core::BitWriter writer;

    if (auto res = h225::RasPDU::encode(writer, msg); !res) {
        std::cerr << "Fail: " << res.error().message << std::endl;
        return 1;
    }

    const auto& data = writer.data();

    // Выводим в формате, который Wireshark понимает "из коробки"
    std::cout << "\n--- Copy this Hex to Wireshark (Import from Hex Dump) ---\n" << std::endl;
    std::cout << "000000 "; // Смещение для Wireshark
    for (auto b : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(b) << " ";
    }
    std::cout << std::dec << "\n\n---------------------------------------------------------" << std::endl;

    // Сохраняем в текущую папку
    std::ofstream file("sample.bin", std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), data.size());

    std::cout << "Generated H225_RAS_GRQ sample.bin ("
        << data.size() << " bytes)" << std::endl;

    return 0;
}