#include "ipc_socket.hpp"
#include "socket_error.hpp"

#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <unistd.h>

mt::sockets::IpcSocket::IpcSocket(SocketType p_socket_type) :
    m_socket(-1),
    m_type(p_socket_type),
    m_global_namespace(false),
    m_peer_global_namespace(false),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_connected(false) {
    if (m_type == mt::sockets::SocketType::STREAM) {
        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    } else {
        m_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    }
    if (m_socket < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EPROTONOSUPPORT: {
                error = mt::sockets::Error::SOCKET_PROTOCOL_NOT_SUPPORTED;
                break;
            }
            case EMFILE: {
                error = mt::sockets::Error::SOCKET_PROCESS_TABLE_IS_FULL;
                break;
            }
            case ENFILE: {
                error = mt::sockets::Error::SOCKET_SYSTEM_TABLE_IS_FULL;
                break;
            }
            case EACCES: {
                error = mt::sockets::Error::SOCKET_NOT_ENOUGH_PERMISSIONS;
                break;
            }
            case ENOSR: {
                error = mt::sockets::Error::SOCKET_NOT_ENOUGH_MEMORY;
                break;
            }
            case EPROTOTYPE: {
                error = mt::sockets::Error::SOCKET_WRONG_PROTOCOL;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
}

mt::sockets::IpcSocket::~IpcSocket() { IpcSocket::close(); }

void mt::sockets::IpcSocket::setName(const std::string& p_name, bool p_global_namespace) {
    m_global_namespace = p_global_namespace;
    if (m_global_namespace) {
        m_name = "#";
    }
    m_name += p_name;
}

void mt::sockets::IpcSocket::setPeerName(const std::string& p_name, bool p_global_namespace) {
    m_peer_global_namespace = p_global_namespace;
    if (m_peer_global_namespace) {
        m_peer_name = "#";
    }
    m_peer_name += p_name;
}

void mt::sockets::IpcSocket::setNonBlocking(bool p_non_blocking) {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    int32_t status;
    if (p_non_blocking) {
        status = fcntl(m_socket, F_SETFL, O_NONBLOCK);
    } else {
        status = fcntl(m_socket, F_SETFL, 0);
    }
    if (status < 0) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_FCNTL_ERROR);
        return;
    }
    m_non_blocking = p_non_blocking;
}

void mt::sockets::IpcSocket::resetError() { m_error = mt::sockets::makeError(mt::sockets::Error::SUCCESS); }

void mt::sockets::IpcSocket::bind() {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_bound) {
        return;
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, m_name.c_str());
    if (m_name.at(0) == '#') {
        address.sun_path[0] = 0;
    }
    auto address_length = sizeof(address.sun_family) + m_name.size();
    auto status = ::bind(m_socket, reinterpret_cast< const struct sockaddr* >(&address), address_length);
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EACCES: {
                error = mt::sockets::Error::BIND_NOT_ENOUGH_PERMISSIONS;
                break;
            }
            case EADDRINUSE: {
                error = mt::sockets::Error::BIND_ADDRESS_IN_USE;
                break;
            }
            case EBADF: {
                error = mt::sockets::Error::BIND_BAD_FILE_DESCRIPTOR;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::BIND_ALREADY_BOUND;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::BIND_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EADDRNOTAVAIL: {
                error = mt::sockets::Error::BIND_ADDRESS_NOT_AVAILABLE;
                break;
            }
            case EFAULT: {
                error = mt::sockets::Error::BIND_ADDRESS_OUTSIDE_USER_SPACE;
                break;
            }
            case ELOOP: {
                error = mt::sockets::Error::BIND_TO_MANY_SYMBOLIC_LINKS;
                break;
            }
            case ENAMETOOLONG: {
                error = mt::sockets::Error::BIND_NAME_TO_LONG;
                break;
            }
            case ENOENT: {
                error = mt::sockets::Error::BIND_NO_ENTRY;
                break;
            }
            case ENOMEM: {
                error = mt::sockets::Error::BIND_NO_MEMORY;
                break;
            }
            case ENOTDIR: {
                error = mt::sockets::Error::BIND_NOT_DIRECTORY;
                break;
            }
            case EROFS: {
                error = mt::sockets::Error::BIND_READ_ONLY_FS;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
    if (not m_error) {
        m_bound = true;
    }
}

void mt::sockets::IpcSocket::listen(uint32_t p_connection_count_limit) {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_connected) {
        m_error = mt::sockets::makeError(mt::sockets::Error::LISTEN_ALREADY_CONNECTED);
        return;
    }
    if (not m_bound) {
        m_error = mt::sockets::makeError(mt::sockets::Error::LISTEN_NOT_BOUND);
        return;
    }
    auto status = ::listen(m_socket, static_cast< int32_t >(p_connection_count_limit));
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EADDRINUSE: {
                error = mt::sockets::Error::LISTEN_ADDRESS_IN_USE;
                break;
            }
            case EBADF: {
                error = mt::sockets::Error::LISTEN_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::LISTEN_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = mt::sockets::Error::LISTEN_PROTOCOL_NOT_SUPPORTED;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
    m_listening = true;
}

void mt::sockets::IpcSocket::connect() {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_listening) {
        m_error = mt::sockets::makeError(mt::sockets::Error::CONNECT_SOCKET_IS_IN_LISTEN_MODE);
        return;
    }
    if (not m_connected) {
        sockaddr_un peer_address{};
        peer_address.sun_family = AF_UNIX;
        strcpy(peer_address.sun_path, m_peer_name.c_str());
        if (m_peer_name.at(0) == '#') {
            peer_address.sun_path[0] = 0;
        }
        auto address_length = sizeof(peer_address.sun_family) + m_peer_name.size();
        auto status = ::connect(m_socket, reinterpret_cast< struct sockaddr* >(&peer_address), address_length);
        if (status < 0) {
            mt::sockets::Error error{};
            switch (errno) {
                case EACCES: {
                    [[fallthrough]];
                }
                case EPERM: {
                    error = mt::sockets::Error::CONNECT_NOT_ENOUGH_PERMISSIONS;
                    break;
                }
                case EADDRINUSE: {
                    error = mt::sockets::Error::CONNECT_ADDRESS_IN_USE;
                    break;
                }
                case EADDRNOTAVAIL: {
                    error = mt::sockets::Error::CONNECT_ADDRESS_NOT_AVAILABLE;
                    break;
                }
                case EAFNOSUPPORT: {
                    error = mt::sockets::Error::CONNECT_AF_NOT_SUPPORTED;
                    break;
                }
                case EAGAIN: {
                    error = mt::sockets::Error::CONNECT_TRY_AGAIN;
                    break;
                }
                case EALREADY: {
                    error = mt::sockets::Error::CONNECT_ALREADY_IN_PROCESS;
                    break;
                }
                case EBADF: {
                    error = mt::sockets::Error::CONNECT_BAD_FILE_DESCRIPTOR;
                    break;
                }
                case ECONNREFUSED: {
                    error = mt::sockets::Error::CONNECT_CONNECTION_REFUSED;
                    break;
                }
                case EFAULT: {
                    error = mt::sockets::Error::CONNECT_ADDRESS_OUTSIDE_USER_SPACE;
                    break;
                }
                case EINPROGRESS: {
                    error = mt::sockets::Error::CONNECT_IN_PROGRESS;
                    break;
                }
                case EINTR: {
                    error = mt::sockets::Error::CONNECT_INTERRUPTED;
                    break;
                }
                case EISCONN: {
                    error = mt::sockets::Error::CONNECT_CONNECTED;
                    break;
                }
                case ENETUNREACH: {
                    error = mt::sockets::Error::CONNECT_NETWORK_UNREACHABLE;
                    break;
                }
                case ENOTSOCK: {
                    error = mt::sockets::Error::CONNECT_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                    break;
                }
                case EPROTOTYPE: {
                    error = mt::sockets::Error::CONNECT_PROTOCOL_NOT_SUPPORTED;
                    break;
                }
                case ETIMEDOUT: {
                    error = mt::sockets::Error::SOCKET_TIMED_OUT;
                    break;
                }
                default: {
                    m_error = std::error_code(errno, std::system_category());
                }
            }
            if (not m_error) {
                m_error = mt::sockets::makeError(error);
            }
            return;
        }
        m_connected = true;
    }
}

void mt::sockets::IpcSocket::close() {
    ::close(m_socket);
    ::unlink(m_name.c_str());
}

void mt::sockets::IpcSocket::shutdown() {
    auto status = ::shutdown(m_socket, SHUT_RDWR);
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EBADF: {
                error = mt::sockets::Error::SHUTDOWN_INVALID_SOCKET_ARGUMENT;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::SHUTDOWN_INVALID_SHUTDOWN_OPTION_PROVIDED;
                break;
            }
            case ENOTCONN: {
                error = mt::sockets::Error::SHUTDOWN_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::SHUTDOWN_INVALID_FILE_DESCRIPTOR;
                break;
            }
            case ENOBUFS: {
                error = mt::sockets::Error::SHUTDOWN_NOT_ENOUGH_MEMORY;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
}

auto mt::sockets::IpcSocket::accept() -> std::optional< std::unique_ptr< IpcSocket > > {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return std::nullopt;
    }
    if (not m_listening) {
        m_error = mt::sockets::makeError(mt::sockets::Error::ACCEPT_SOCKET_IS_NOT_IN_LISTEN_MODE);
        return std::nullopt;
    }
    if (m_connected) {
        m_error = mt::sockets::makeError(mt::sockets::Error::ACCEPT_ALREADY_CONNECTED);
        return std::nullopt;
    }
    if (not m_bound) {
        m_error = mt::sockets::makeError(mt::sockets::Error::ACCEPT_NOT_BOUND);
        return std::nullopt;
    }
    sockaddr_un peer_address{};
    uint32_t address_length = sizeof(peer_address);
    std::unique_ptr< mt::sockets::IpcSocket > socket(new mt::sockets::IpcSocket(true));
    socket->m_type = m_type;
    socket->m_socket = ::accept(m_socket, reinterpret_cast< struct sockaddr* >(&peer_address), &address_length);
    if (socket->m_socket < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EAGAIN: {
                [[fallthrough]];
            }
            case ENETDOWN: {
                [[fallthrough]];
            }
            case ENOPROTOOPT: {
                [[fallthrough]];
            }
            case EHOSTDOWN: {
                [[fallthrough]];
            }
            case ENONET: {
                [[fallthrough]];
            }
            case EHOSTUNREACH: {
                [[fallthrough]];
            }
            case ENETUNREACH: {
                error = mt::sockets::Error::ACCEPT_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::ACCEPT_TRY_AGAIN;
                break;
            }
#endif
            case EBADF: {
                error = mt::sockets::Error::ACCEPT_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNABORTED: {
                error = mt::sockets::Error::ACCEPT_CONNECTION_ABORTED;
                break;
            }
            case EFAULT: {
                error = mt::sockets::Error::ACCEPT_ADDRESS_OUTSIDE_USER_SPACE;
                break;
            }
            case EINTR: {
                error = mt::sockets::Error::ACCEPT_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::ACCEPT_INVALID_VALUE;
                break;
            }
            case EMFILE: {
                error = mt::sockets::Error::ACCEPT_PER_PROCESS_LIMIT_REACHED;
                break;
            }
            case ENFILE: {
                error = mt::sockets::Error::ACCEPT_SYSTEM_WIDE_LIMIT_REACHED;
                break;
            }
            case ENOBUFS: {
                [[fallthrough]];
            }
            case ENOMEM: {
                error = mt::sockets::Error::ACCEPT_NOT_ENOUGH_MEMORY;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::ACCEPT_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EPERM: {
                error = mt::sockets::Error::ACCEPT_FIREWALL;
                break;
            }
            case EOPNOTSUPP: {
                error = mt::sockets::Error::ACCEPT_OPTION_IS_NOT_SUPPORTED;
                break;
            }
            case EPROTO: {
                error = mt::sockets::Error::ACCEPT_PROTOCOL_ERROR;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
        return std::nullopt;
    }

    if (m_non_blocking) {
        socket->setNonBlocking();
    }
    socket->m_name = m_name;
    if (peer_address.sun_path[0] == 0){
        socket->m_peer_name = std::string(peer_address.sun_path + 1);
        socket->m_peer_global_namespace = m_peer_global_namespace;
    } else {
        socket->m_peer_name = std::string(peer_address.sun_path);
    }
    socket->m_connected = true;
    socket->m_global_namespace = m_global_namespace;
    return socket;
}

auto mt::sockets::IpcSocket::write(uint8_t p_byte) -> uint8_t {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return 0;
    }
    if (p_byte == 0) {
        return 0;
    }
    uint8_t bytes_sent = 0;
    if (m_connected) {
        bytes_sent = ::send(m_socket, &p_byte, 1, MSG_NOSIGNAL);
    } else {
        if (m_type == mt::sockets::SocketType::STREAM) {
            m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_un peer_address{};
            peer_address.sun_family = AF_UNIX;
            strcpy(peer_address.sun_path, m_peer_name.c_str());
            if (m_peer_name.at(0) == '#') {
                peer_address.sun_path[0] = 0;
            }
            auto address_length = sizeof(peer_address.sun_family) + m_peer_name.size();
            bytes_sent = ::sendto(m_socket, &p_byte, 1, MSG_NOSIGNAL, reinterpret_cast< struct sockaddr* >(&peer_address), address_length);
        }
    }
    if (static_cast< int8_t >(bytes_sent) < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EACCES: {
                error = mt::sockets::Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                error = mt::sockets::Error::WRITE_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::WRITE_TRY_AGAIN;
                break;
            }
#endif
            case EALREADY: {
                error = mt::sockets::Error::WRITE_ALREADY;
                break;
            }
            case EBADF: {
                error = mt::sockets::Error::WRITE_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNRESET: {
                error = mt::sockets::Error::WRITE_CONNECTION_RESET;
                break;
            }
            case EDESTADDRREQ: {
                error = mt::sockets::Error::WRITE_DESTINATION_ADDRESS;
                break;
            }
            case EFAULT: {
                error = mt::sockets::Error::WRITE_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = mt::sockets::Error::WRITE_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::WRITE_INVALID_ARGUMENT;
                break;
            }
            case EISCONN: {
                error = mt::sockets::Error::WRITE_IS_CONNECTED;
                break;
            }
            case EMSGSIZE: {
                error = mt::sockets::Error::WRITE_MESSAGE_SIZE;
                break;
            }
            case ENOBUFS: {
                error = mt::sockets::Error::WRITE_NO_BUFFER;
                break;
            }
            case ENOMEM: {
                error = mt::sockets::Error::WRITE_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = mt::sockets::Error::WRITE_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::WRITE_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = mt::sockets::Error::WRITE_NOT_SUPPORTED;
                break;
            }
            case EPIPE: {
                error = mt::sockets::Error::WRITE_PIPE;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
    return bytes_sent;
}

auto mt::sockets::IpcSocket::write(const std::vector< uint8_t >& p_data, uint16_t p_size, uint64_t p_offset) -> uint64_t {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return 0;
    }
    if (p_data.empty()) {
        return 0;
    }

    uint64_t bytes_sent = 0;
    uint64_t l_size = (p_size == 0 ? p_data.size() : p_size);
    if (m_connected) {
        bytes_sent = ::send(m_socket, p_data.data() + p_offset, l_size, MSG_NOSIGNAL);
    } else {
        if (m_type == mt::sockets::SocketType::STREAM) {
            m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_un peer_address{};
            peer_address.sun_family = AF_UNIX;
            strcpy(peer_address.sun_path, m_peer_name.c_str());
            if (m_peer_name.at(0) == '#') {
                peer_address.sun_path[0] = 0;
            }
            auto address_length = sizeof(peer_address.sun_family) + m_peer_name.size();
            bytes_sent = ::sendto(m_socket, p_data.data() + p_offset, l_size, MSG_NOSIGNAL, reinterpret_cast< struct sockaddr* >(&peer_address), address_length);
        }
    }
    if (static_cast< int8_t >(bytes_sent) < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EACCES: {
                error = mt::sockets::Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                error = mt::sockets::Error::WRITE_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::WRITE_TRY_AGAIN;
                break;
            }
#endif
            case EALREADY: {
                error = mt::sockets::Error::WRITE_ALREADY;
                break;
            }
            case EBADF: {
                error = mt::sockets::Error::WRITE_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNRESET: {
                error = mt::sockets::Error::WRITE_CONNECTION_RESET;
                break;
            }
            case EDESTADDRREQ: {
                error = mt::sockets::Error::WRITE_DESTINATION_ADDRESS;
                break;
            }
            case EFAULT: {
                error = mt::sockets::Error::WRITE_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = mt::sockets::Error::WRITE_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::WRITE_INVALID_ARGUMENT;
                break;
            }
            case EISCONN: {
                error = mt::sockets::Error::WRITE_IS_CONNECTED;
                break;
            }
            case EMSGSIZE: {
                error = mt::sockets::Error::WRITE_MESSAGE_SIZE;
                break;
            }
            case ENOBUFS: {
                error = mt::sockets::Error::WRITE_NO_BUFFER;
                break;
            }
            case ENOMEM: {
                error = mt::sockets::Error::WRITE_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = mt::sockets::Error::WRITE_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::WRITE_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = mt::sockets::Error::WRITE_NOT_SUPPORTED;
                break;
            }
            case EPIPE: {
                error = mt::sockets::Error::WRITE_PIPE;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
    return bytes_sent;
}

auto mt::sockets::IpcSocket::read() -> uint8_t {
    uint8_t byte = 0;

    auto status = ::recv(m_socket, &byte, 1, 0);
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EAGAIN: {
                error = mt::sockets::Error::READ_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::READ_TRY_AGAIN;
                break;
            }
#endif
            case EBADF: {
                error = mt::sockets::Error::READ_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNREFUSED: {
                error = mt::sockets::Error::READ_CONNECTION_REFUSED;
                break;
            }
            case EFAULT: {
                error = mt::sockets::Error::READ_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = mt::sockets::Error::READ_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::READ_INVALID_FILE_DESCRIPTOR;
                break;
            }
            case ENOMEM: {
                error = mt::sockets::Error::READ_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = mt::sockets::Error::READ_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::READ_NOT_SOCKET;
                break;
            }
            case ECONNRESET: {
                error = mt::sockets::Error::READ_CONNECTION_RESET;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    }
    if (status == 0 || byte == 255) {
        m_error = mt::sockets::makeError(mt::sockets::Error::READ_EOF);
        byte = 0;
    }
    return byte;
}

auto mt::sockets::IpcSocket::read(uint16_t p_size) -> std::vector< uint8_t > {
    if (p_size == 0) {
        return {};
    }

    std::vector< uint8_t > data;
    data.resize(p_size);
    auto status = ::recv(m_socket, data.data(), p_size, 0);
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EAGAIN: {
                error = mt::sockets::Error::READ_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::READ_TRY_AGAIN;
                break;
            }
#endif
            case EBADF: {
                error = mt::sockets::Error::READ_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNREFUSED: {
                error = mt::sockets::Error::READ_CONNECTION_REFUSED;
                break;
            }
            case EFAULT: {
                error = mt::sockets::Error::READ_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = mt::sockets::Error::READ_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = mt::sockets::Error::READ_INVALID_FILE_DESCRIPTOR;
                break;
            }
            case ENOMEM: {
                error = mt::sockets::Error::READ_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = mt::sockets::Error::READ_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = mt::sockets::Error::READ_NOT_SOCKET;
                break;
            }
            case ECONNRESET: {
                error = mt::sockets::Error::READ_CONNECTION_RESET;
                break;
            }
        }
        m_error = mt::sockets::makeError(error);
    } else if (status == 0) {
        m_error = mt::sockets::makeError(mt::sockets::Error::READ_EOF);
    }

    //    for (uint16_t i = 0; i < size; ++i) {
    //        uint8_t byte = InetSocket::read();
    //        if (m_error) {
    //            break;
    //        }
    //        data.push_back(byte);
    //    }
    data.shrink_to_fit();
    if (data.at(0) == 0) {
        return {};
    }
    return data;
}

auto mt::sockets::IpcSocket::readUntil(uint8_t p_delimiter) -> std::vector< uint8_t > {
    std::vector< uint8_t > data;

    while (true) {
        uint8_t byte = IpcSocket::read();
        if (m_error || byte == 0) {
            break;
        }
        if (byte == p_delimiter) {
            m_error = mt::sockets::makeError(mt::sockets::Error::READ_DONE);
            break;
        }
        data.push_back(byte);
    }
    if (not data.empty()) {
        data.shrink_to_fit();
    }
    return data;
}

auto mt::sockets::IpcSocket::readUntil(const std::vector< uint8_t >& p_delimiter) -> std::vector< uint8_t > {
    std::vector< uint8_t > data;
    data.reserve(p_delimiter.size());
    while (true) {
        uint8_t byte = IpcSocket::read();
        if (m_error || byte == 0) {
            break;
        }
        data.push_back(byte);
        if (data.size() >= p_delimiter.size()) {
            std::vector< uint8_t > to_compare(data.end() - static_cast< int64_t >(p_delimiter.size()), data.end());
            if (to_compare == p_delimiter) {
                m_error = mt::sockets::makeError(mt::sockets::Error::READ_DONE);
                break;
            }
        } else if (data.size() == p_delimiter.size() && data == p_delimiter) {
            m_error = mt::sockets::makeError(mt::sockets::Error::READ_DONE);
            break;
        }
    }

    if (m_error.value() == static_cast< int >(mt::sockets::Error::READ_DONE)) {
        data.erase(data.end() - static_cast< int64_t >(p_delimiter.size()), data.end());
    }
    if (not data.empty()) {
        data.shrink_to_fit();
    }
    return data;
}

auto mt::sockets::IpcSocket::name() const noexcept -> const std::string& { return m_name; }

auto mt::sockets::IpcSocket::peerName() const noexcept -> const std::string& { return m_peer_name; }

auto mt::sockets::IpcSocket::error() const noexcept -> std::error_code { return m_error; }

auto mt::sockets::IpcSocket::nonBlocking() const noexcept -> bool { return m_non_blocking; }

auto mt::sockets::IpcSocket::connected() const noexcept -> bool { return m_connected; }

mt::sockets::IpcSocket::IpcSocket(bool) :
    m_socket(-1),
    m_type(mt::sockets::SocketType::STREAM),
    m_global_namespace(false),
    m_peer_global_namespace(false),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_connected(false) { }
