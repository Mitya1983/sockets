#include "include/ipc_socket.hpp"
#include "include/socket_error.hpp"

#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/un.h>
#include <unistd.h>

#include <algorithm>

mt::sockets::IpcSocket::IpcSocket(const SocketType p_socket_type) :
    m_socket(-1),
    m_type(p_socket_type),
    m_global_namespace(false),
    m_peer_global_namespace(false),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_connected(false) {
    if (m_type == SocketType::STREAM) {
        m_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    } else {
        m_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    }
    if (m_socket < 0) {
        Error error{};
        switch (errno) {
            case EPROTONOSUPPORT: {
                error = Error::SOCKET_PROTOCOL_NOT_SUPPORTED;
                break;
            }
            case EMFILE: {
                error = Error::SOCKET_PROCESS_TABLE_IS_FULL;
                break;
            }
            case ENFILE: {
                error = Error::SOCKET_SYSTEM_TABLE_IS_FULL;
                break;
            }
            case EACCES: {
                error = Error::SOCKET_NOT_ENOUGH_PERMISSIONS;
                break;
            }
            case ENOSR: {
                error = Error::SOCKET_NOT_ENOUGH_MEMORY;
                break;
            }
            case EPROTOTYPE: {
                error = Error::SOCKET_WRONG_PROTOCOL;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
}

mt::sockets::IpcSocket::~IpcSocket() {
    close();
}

void mt::sockets::IpcSocket::setName(const std::string& p_name, const bool p_global_namespace) {
    m_global_namespace = p_global_namespace;
    if (m_global_namespace) {
        m_name = "#";
    }
    m_name += p_name;
}

void mt::sockets::IpcSocket::setPeerName(const std::string& p_name, const bool p_global_namespace) {
    m_peer_global_namespace = p_global_namespace;
    if (m_peer_global_namespace) {
        m_peer_name = "#";
    }
    m_peer_name += p_name;
}

void mt::sockets::IpcSocket::setNonBlocking(const bool p_non_blocking) {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    int32_t status;
    if (p_non_blocking) {
        status = fcntl(m_socket, F_SETFL, O_NONBLOCK);
    } else {
        status = fcntl(m_socket, F_SETFL, 0);
    }
    if (status < 0) {
        m_error = makeError(Error::SOCKET_FCNTL_ERROR);
        return;
    }
    m_non_blocking = p_non_blocking;
}

void mt::sockets::IpcSocket::resetError() {
    m_error = makeError(Error::SUCCESS);
}

void mt::sockets::IpcSocket::bind() {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_bound) {
        return;
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, m_name.c_str());
    if (m_name.at(0) == '#') {
        address.sun_path[ 0 ] = 0;
    }
    const auto address_length = sizeof(address.sun_family) + m_name.size();
    if (const auto status = ::bind(m_socket, reinterpret_cast< const struct sockaddr * >(&address), address_length); status < 0) {
        Error error{};
        switch (errno) {
            case EACCES: {
                error = Error::BIND_NOT_ENOUGH_PERMISSIONS;
                break;
            }
            case EADDRINUSE: {
                error = Error::BIND_ADDRESS_IN_USE;
                break;
            }
            case EBADF: {
                error = Error::BIND_BAD_FILE_DESCRIPTOR;
                break;
            }
            case EINVAL: {
                error = Error::BIND_ALREADY_BOUND;
                break;
            }
            case ENOTSOCK: {
                error = Error::BIND_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EADDRNOTAVAIL: {
                error = Error::BIND_ADDRESS_NOT_AVAILABLE;
                break;
            }
            case EFAULT: {
                error = Error::BIND_ADDRESS_OUTSIDE_USER_SPACE;
                break;
            }
            case ELOOP: {
                error = Error::BIND_TO_MANY_SYMBOLIC_LINKS;
                break;
            }
            case ENAMETOOLONG: {
                error = Error::BIND_NAME_TO_LONG;
                break;
            }
            case ENOENT: {
                error = Error::BIND_NO_ENTRY;
                break;
            }
            case ENOMEM: {
                error = Error::BIND_NO_MEMORY;
                break;
            }
            case ENOTDIR: {
                error = Error::BIND_NOT_DIRECTORY;
                break;
            }
            case EROFS: {
                error = Error::BIND_READ_ONLY_FS;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
    if (not m_error) {
        m_bound = true;
    }
}

void mt::sockets::IpcSocket::listen(const uint32_t p_connection_count_limit) {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_connected) {
        m_error = makeError(Error::LISTEN_ALREADY_CONNECTED);
        return;
    }
    if (not m_bound) {
        m_error = makeError(Error::LISTEN_NOT_BOUND);
        return;
    }
    if (const auto status = ::listen(m_socket, static_cast< int32_t >(p_connection_count_limit)); status < 0) {
        Error error{};
        switch (errno) {
            case EADDRINUSE: {
                error = Error::LISTEN_ADDRESS_IN_USE;
                break;
            }
            case EBADF: {
                error = Error::LISTEN_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ENOTSOCK: {
                error = Error::LISTEN_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = Error::LISTEN_PROTOCOL_NOT_SUPPORTED;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
    m_listening = true;
}

void mt::sockets::IpcSocket::connect() {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_listening) {
        m_error = makeError(Error::CONNECT_SOCKET_IS_IN_LISTEN_MODE);
        return;
    }
    if (not m_connected) {
        sockaddr_un peer_address{};
        peer_address.sun_family = AF_UNIX;
        strcpy(peer_address.sun_path, m_peer_name.c_str());
        if (m_peer_name.at(0) == '#') {
            peer_address.sun_path[ 0 ] = 0;
        }
        const auto address_length = sizeof(peer_address.sun_family) + m_peer_name.size();
        if (const auto status = ::connect(m_socket, reinterpret_cast< struct sockaddr * >(&peer_address), address_length); status < 0) {
            Error error{};
            switch (errno) {
                case EACCES: {
                    [[fallthrough]];
                }
                case EPERM: {
                    error = Error::CONNECT_NOT_ENOUGH_PERMISSIONS;
                    break;
                }
                case EADDRINUSE: {
                    error = Error::CONNECT_ADDRESS_IN_USE;
                    break;
                }
                case EADDRNOTAVAIL: {
                    error = Error::CONNECT_ADDRESS_NOT_AVAILABLE;
                    break;
                }
                case EAFNOSUPPORT: {
                    error = Error::CONNECT_AF_NOT_SUPPORTED;
                    break;
                }
                case EAGAIN: {
                    error = Error::CONNECT_TRY_AGAIN;
                    break;
                }
                case EALREADY: {
                    error = Error::CONNECT_ALREADY_IN_PROCESS;
                    break;
                }
                case EBADF: {
                    error = Error::CONNECT_BAD_FILE_DESCRIPTOR;
                    break;
                }
                case ECONNREFUSED: {
                    error = Error::CONNECT_CONNECTION_REFUSED;
                    break;
                }
                case EFAULT: {
                    error = Error::CONNECT_ADDRESS_OUTSIDE_USER_SPACE;
                    break;
                }
                case EINPROGRESS: {
                    error = Error::CONNECT_IN_PROGRESS;
                    break;
                }
                case EINTR: {
                    error = Error::CONNECT_INTERRUPTED;
                    break;
                }
                case EISCONN: {
                    error = Error::CONNECT_CONNECTED;
                    break;
                }
                case ENETUNREACH: {
                    error = Error::CONNECT_NETWORK_UNREACHABLE;
                    break;
                }
                case ENOTSOCK: {
                    error = Error::CONNECT_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                    break;
                }
                case EPROTOTYPE: {
                    error = Error::CONNECT_PROTOCOL_NOT_SUPPORTED;
                    break;
                }
                case ETIMEDOUT: {
                    error = Error::SOCKET_TIMED_OUT;
                    break;
                }
                default: {
                    m_error = std::error_code(errno, std::system_category());
                }
            }
            if (not m_error) {
                m_error = makeError(error);
            }
            return;
        }
        m_connected = true;
    }
}

void mt::sockets::IpcSocket::close() const {
    ::close(m_socket);
    ::unlink(m_name.c_str());
}

void mt::sockets::IpcSocket::shutdown() {
    if (const auto status = ::shutdown(m_socket, SHUT_RDWR); status < 0) {
        Error error{};
        switch (errno) {
            case EBADF: {
                error = Error::SHUTDOWN_INVALID_SOCKET_ARGUMENT;
                break;
            }
            case EINVAL: {
                error = Error::SHUTDOWN_INVALID_SHUTDOWN_OPTION_PROVIDED;
                break;
            }
            case ENOTCONN: {
                error = Error::SHUTDOWN_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = Error::SHUTDOWN_INVALID_FILE_DESCRIPTOR;
                break;
            }
            case ENOBUFS: {
                error = Error::SHUTDOWN_NOT_ENOUGH_MEMORY;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
}

auto mt::sockets::IpcSocket::accept() -> std::optional< std::unique_ptr< IpcSocket > > {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return std::nullopt;
    }
    if (not m_listening) {
        m_error = makeError(Error::ACCEPT_SOCKET_IS_NOT_IN_LISTEN_MODE);
        return std::nullopt;
    }
    if (m_connected) {
        m_error = makeError(Error::ACCEPT_ALREADY_CONNECTED);
        return std::nullopt;
    }
    if (not m_bound) {
        m_error = makeError(Error::ACCEPT_NOT_BOUND);
        return std::nullopt;
    }
    sockaddr_un peer_address{};
    uint32_t address_length = sizeof(peer_address);
    std::unique_ptr< IpcSocket > socket(new IpcSocket(true));
    socket->m_type = m_type;
    socket->m_socket = ::accept(m_socket, reinterpret_cast< struct sockaddr * >(&peer_address), &address_length);
    if (socket->m_socket < 0) {
        Error error{};
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
                error = Error::ACCEPT_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::ACCEPT_TRY_AGAIN;
                break;
            }
#endif
            case EBADF: {
                error = Error::ACCEPT_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNABORTED: {
                error = Error::ACCEPT_CONNECTION_ABORTED;
                break;
            }
            case EFAULT: {
                error = Error::ACCEPT_ADDRESS_OUTSIDE_USER_SPACE;
                break;
            }
            case EINTR: {
                error = Error::ACCEPT_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = Error::ACCEPT_INVALID_VALUE;
                break;
            }
            case EMFILE: {
                error = Error::ACCEPT_PER_PROCESS_LIMIT_REACHED;
                break;
            }
            case ENFILE: {
                error = Error::ACCEPT_SYSTEM_WIDE_LIMIT_REACHED;
                break;
            }
            case ENOBUFS: {
                [[fallthrough]];
            }
            case ENOMEM: {
                error = Error::ACCEPT_NOT_ENOUGH_MEMORY;
                break;
            }
            case ENOTSOCK: {
                error = Error::ACCEPT_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EPERM: {
                error = Error::ACCEPT_FIREWALL;
                break;
            }
            case EOPNOTSUPP: {
                error = Error::ACCEPT_OPTION_IS_NOT_SUPPORTED;
                break;
            }
            case EPROTO: {
                error = Error::ACCEPT_PROTOCOL_ERROR;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
        return std::nullopt;
    }

    if (m_non_blocking) {
        socket->setNonBlocking();
    }
    socket->m_name = m_name;
    if (peer_address.sun_path[ 0 ] == 0) {
        socket->m_peer_name = std::string(peer_address.sun_path + 1);
        socket->m_peer_global_namespace = m_peer_global_namespace;
    } else {
        socket->m_peer_name = std::string(peer_address.sun_path);
    }
    socket->m_connected = true;
    socket->m_global_namespace = m_global_namespace;
    return socket;
}

auto mt::sockets::IpcSocket::read() -> std::byte {
    std::byte byte{0};

    const auto status = ::recv(m_socket, &byte, 1, 0);
    if (status < 0) {
        Error error{};
        switch (errno) {
            case EAGAIN: {
                error = Error::READ_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::READ_TRY_AGAIN;
                break;
            }
#endif
            case EBADF: {
                error = Error::READ_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNREFUSED: {
                error = Error::READ_CONNECTION_REFUSED;
                break;
            }
            case EFAULT: {
                error = Error::READ_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = Error::READ_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = Error::READ_INVALID_FILE_DESCRIPTOR;
                break;
            }
            case ENOMEM: {
                error = Error::READ_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = Error::READ_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = Error::READ_NOT_SOCKET;
                break;
            }
            case ECONNRESET: {
                error = Error::READ_CONNECTION_RESET;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
    if (status == 0 or byte == std::byte{255}) {
        m_error = makeError(Error::READ_EOF);
        byte = std::byte{0};
    }
    return byte;
}

auto mt::sockets::IpcSocket::read(const uint16_t p_size) -> std::vector< std::byte > {
    if (p_size == 0) {
        return {};
    }

    std::vector< std::byte > data;
    data.resize(p_size);
    if (const auto status = ::recv(m_socket, data.data(), p_size, 0); status < 0) {
        Error error{};
        switch (errno) {
            case EAGAIN: {
                error = Error::READ_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::READ_TRY_AGAIN;
                break;
            }
#endif
            case EBADF: {
                error = Error::READ_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNREFUSED: {
                error = Error::READ_CONNECTION_REFUSED;
                break;
            }
            case EFAULT: {
                error = Error::READ_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = Error::READ_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = Error::READ_INVALID_FILE_DESCRIPTOR;
                break;
            }
            case ENOMEM: {
                error = Error::READ_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = Error::READ_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = Error::READ_NOT_SOCKET;
                break;
            }
            case ECONNRESET: {
                error = Error::READ_CONNECTION_RESET;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    } else if (status == 0) {
        m_error = makeError(Error::READ_EOF);
    }
    data.shrink_to_fit();
    if (data.at(0) == std::byte{0}) {
        return {};
    }
    return data;
}

auto mt::sockets::IpcSocket::name() const noexcept -> const std::string& {
    return m_name;
}

auto mt::sockets::IpcSocket::peerName() const noexcept -> const std::string& {
    return m_peer_name;
}

auto mt::sockets::IpcSocket::error() const noexcept -> std::error_code {
    return m_error;
}

auto mt::sockets::IpcSocket::nonBlocking() const noexcept -> bool {
    return m_non_blocking;
}

auto mt::sockets::IpcSocket::connected() const noexcept -> bool {
    return m_connected;
}

mt::sockets::IpcSocket::IpcSocket(bool) :
    m_socket(-1),
    m_type(SocketType::STREAM),
    m_global_namespace(false),
    m_peer_global_namespace(false),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_connected(false) {
}

void mt::sockets::IpcSocket::write_byte(const std::byte p_byte) {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
    }
    uint8_t bytes_sent = 0;
    if (m_connected) {
        bytes_sent = ::send(m_socket, &p_byte, 1, MSG_NOSIGNAL);
    } else {
        if (m_type == SocketType::STREAM) {
            m_error = makeError(Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_un peer_address{};
            peer_address.sun_family = AF_UNIX;
            strcpy(peer_address.sun_path, m_peer_name.c_str());
            if (m_peer_name.at(0) == '#') {
                peer_address.sun_path[ 0 ] = 0;
            }
            const auto address_length = sizeof(peer_address.sun_family) + m_peer_name.size();
            bytes_sent = ::sendto(m_socket, &p_byte, 1, MSG_NOSIGNAL, reinterpret_cast< struct sockaddr * >(&peer_address), address_length);
        }
    }
    if (static_cast< int8_t >(bytes_sent) < 0) {
        Error error{};
        switch (errno) {
            case EACCES: {
                error = Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                error = Error::WRITE_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::WRITE_TRY_AGAIN;
                break;
            }
#endif
            case EALREADY: {
                error = Error::WRITE_ALREADY;
                break;
            }
            case EBADF: {
                error = Error::WRITE_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNRESET: {
                error = Error::WRITE_CONNECTION_RESET;
                break;
            }
            case EDESTADDRREQ: {
                error = Error::WRITE_DESTINATION_ADDRESS;
                break;
            }
            case EFAULT: {
                error = Error::WRITE_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = Error::WRITE_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = Error::WRITE_INVALID_ARGUMENT;
                break;
            }
            case EISCONN: {
                error = Error::WRITE_IS_CONNECTED;
                break;
            }
            case EMSGSIZE: {
                error = Error::WRITE_MESSAGE_SIZE;
                break;
            }
            case ENOBUFS: {
                error = Error::WRITE_NO_BUFFER;
                break;
            }
            case ENOMEM: {
                error = Error::WRITE_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = Error::WRITE_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = Error::WRITE_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = Error::WRITE_NOT_SUPPORTED;
                break;
            }
            case EPIPE: {
                error = Error::WRITE_PIPE;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
}

auto mt::sockets::IpcSocket::write_range(const std::byte* p_bytes, const uint64_t p_size) -> uint64_t {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return 0;
    }
    if (p_size == 0) {
        return 0;
    }

    uint64_t bytes_sent = 0;
    if (m_connected) {
        bytes_sent = ::send(m_socket, p_bytes, p_size, MSG_NOSIGNAL);
    } else {
        if (m_type == SocketType::STREAM) {
            m_error = makeError(Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_un peer_address{};
            peer_address.sun_family = AF_UNIX;
            strcpy(peer_address.sun_path, m_peer_name.c_str());
            if (m_peer_name.at(0) == '#') {
                peer_address.sun_path[ 0 ] = 0;
            }
            const auto address_length = sizeof(peer_address.sun_family) + m_peer_name.size();
            bytes_sent = ::sendto(m_socket, p_bytes, p_size, MSG_NOSIGNAL, reinterpret_cast< struct sockaddr * >(&peer_address), address_length);
        }
    }
    if (static_cast< int8_t >(bytes_sent) < 0) {
        Error error{};
        switch (errno) {
            case EACCES: {
                error = Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                error = Error::WRITE_TRY_AGAIN;
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                error = tristan::sockets::Error::WRITE_TRY_AGAIN;
                break;
            }
#endif
            case EALREADY: {
                error = Error::WRITE_ALREADY;
                break;
            }
            case EBADF: {
                error = Error::WRITE_BAD_FILE_DESCRIPTOR;
                break;
            }
            case ECONNRESET: {
                error = Error::WRITE_CONNECTION_RESET;
                break;
            }
            case EDESTADDRREQ: {
                error = Error::WRITE_DESTINATION_ADDRESS;
                break;
            }
            case EFAULT: {
                error = Error::WRITE_BUFFER_OUT_OF_RANGE;
                break;
            }
            case EINTR: {
                error = Error::WRITE_INTERRUPTED;
                break;
            }
            case EINVAL: {
                error = Error::WRITE_INVALID_ARGUMENT;
                break;
            }
            case EISCONN: {
                error = Error::WRITE_IS_CONNECTED;
                break;
            }
            case EMSGSIZE: {
                error = Error::WRITE_MESSAGE_SIZE;
                break;
            }
            case ENOBUFS: {
                error = Error::WRITE_NO_BUFFER;
                break;
            }
            case ENOMEM: {
                error = Error::WRITE_NO_MEMORY;
                break;
            }
            case ENOTCONN: {
                error = Error::WRITE_NOT_CONNECTED;
                break;
            }
            case ENOTSOCK: {
                error = Error::WRITE_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = Error::WRITE_NOT_SUPPORTED;
                break;
            }
            case EPIPE: {
                error = Error::WRITE_PIPE;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
    return bytes_sent;
}

auto mt::sockets::IpcSocket::read_until(const std::byte p_delimiter) -> std::vector< std::byte > {
    std::vector< std::byte > data;

    while (true) {
        const auto byte = read();
        if (m_error or byte == std::byte{0}) {
            break;
        }
        if (byte == p_delimiter) {
            m_error = makeError(Error::READ_DONE);
            break;
        }
        data.push_back(byte);
    }
    if (not data.empty()) {
        data.shrink_to_fit();
    }
    return data;
}

auto mt::sockets::IpcSocket::read_until(const std::byte* p_delimiter, const int64_t p_delimiter_size)
    -> std::vector< std::byte > {
    std::vector< std::byte > data;
    data.reserve(p_delimiter_size);
    while (true) {
        const auto byte = read();
        if (m_error or byte == std::byte{0}) {
            break;
        }
        data.push_back(byte);
        if (std::ssize(data) >= p_delimiter_size) {
            if (std::ranges::equal(data.end() - p_delimiter_size, data.end(), p_delimiter, p_delimiter + p_delimiter_size)) {
                m_error = makeError(Error::READ_DONE);
                break;
            }
        } else if (std::ranges::equal(data.begin(), data.end(), p_delimiter, p_delimiter + p_delimiter_size)) {
            m_error = makeError(Error::READ_DONE);
            break;
        }
    }

    if (m_error.value() == static_cast< int32_t >(Error::READ_DONE)) {
        data.erase(data.end() - p_delimiter_size, data.end());
    }
    if (not data.empty()) {
        data.shrink_to_fit();
    }
    return data;
}
