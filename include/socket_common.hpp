#ifndef SOCKETS_SOCKET_COMMON_HPP
#define SOCKETS_SOCKET_COMMON_HPP

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>
#include <optional>
#include <memory>

namespace mt::sockets {

    enum class SocketType : uint8_t {
        STREAM,
        DATA
    };

} //End of tristan::sockets namespace

#endif  //SOCKETS_SOCKET_COMMON_HPP
