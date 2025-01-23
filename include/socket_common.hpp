#ifndef SOCKETS_SOCKET_COMMON_HPP
#define SOCKETS_SOCKET_COMMON_HPP

#include <cstdint>
#include <concepts>
namespace mt::sockets {

    namespace concepts {
        template < class Type >
        concept write_compatible = std::is_integral_v< std::decay_t< Type > >;
        template < class Type >
        concept delimiter_compatible = write_compatible< Type >;
    }  // namespace concepts

    enum class SocketType : uint8_t {
        STREAM,
        DATA
    };

} //End of tristan::sockets namespace

#endif  //SOCKETS_SOCKET_COMMON_HPP
