#pragma once

#include <expected>
#include <string_view>

namespace h323_26 {

    enum class ErrorCode {
        Success = 0,
        EndOfStream,      // Not enough bits left in the buffer
        InvalidConstraint, // Value exceeds ASN.1 range
        AlignmentError,    // Failed to align to byte boundary
        BufferOverflow,
        UnsupportedFeature
    };

    struct Error {
        ErrorCode code;
        std::string_view message;
    };

    template <typename T>
    using Result = std::expected<T, Error>;

} // namespace h323_26
