#include "include/inet_socket.hpp"
#include "include/socket_error.hpp"
#include "include/ssl.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>

mt::sockets::InetSocket::InetSocket(const SocketType p_socket_type) :
    m_socket(-1),
    m_ip(0),
    m_port(0),
    m_type(p_socket_type),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_not_ssl_connected(false),
    m_connected(false) {

    const auto protocol = getprotobyname("tcp");
    if (m_type == SocketType::STREAM) {
        m_socket = socket(AF_INET, SOCK_STREAM, protocol->p_proto);
    } else {
        m_socket = socket(AF_INET, SOCK_DGRAM, protocol->p_proto);
    }
    if (m_socket < 0) {
        Error error;
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

mt::sockets::InetSocket::~InetSocket() {
    close();
}

void mt::sockets::InetSocket::setHost(const uint32_t p_ip, std::string p_host_name) {
    m_ip = p_ip;
    if (not p_host_name.empty()) {
        m_host_name = std::move(p_host_name);
    }
}

void mt::sockets::InetSocket::setPort(const uint16_t p_port) {
    m_port = p_port;
}

void mt::sockets::InetSocket::setNonBlocking(const bool p_non_blocking) {
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

void mt::sockets::InetSocket::setTimeOut(const std::chrono::seconds p_seconds) {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_non_blocking) {
        return;
    }
    timeval l_timeval{};
    l_timeval.tv_sec = p_seconds.count();
    auto status = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &l_timeval, sizeof(timeval));
    if (status == -1) {
        m_error = makeError(Error::SOCKET_SET_TIMEOUT_ERROR);
        return;
    }
    status = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &l_timeval, sizeof(timeval));
    if (status == -1) {
        m_error = makeError(Error::SOCKET_SET_TIMEOUT_ERROR);
    }
}

void mt::sockets::InetSocket::resetError() {
    m_error = makeError(Error::SUCCESS);
}

void mt::sockets::InetSocket::bind() {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_bound) {
        return;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = m_ip;
    address.sin_port = m_port;
    if (const auto status = ::bind(m_socket, reinterpret_cast< struct sockaddr * >(&address), sizeof(address)); status < 0) {
        Error error;
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

void mt::sockets::InetSocket::listen(const uint32_t p_connection_count_limit) {
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
        Error error;
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

void mt::sockets::InetSocket::connect(const bool p_ssl) {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_listening) {
        m_error = makeError(Error::CONNECT_SOCKET_IS_IN_LISTEN_MODE);
        return;
    }
    if (not m_not_ssl_connected) {
        sockaddr_in remote_address{};
        remote_address.sin_family = AF_INET;
        remote_address.sin_addr.s_addr = m_ip;
        remote_address.sin_port = m_port;
        if (const int32_t status = ::connect(m_socket, reinterpret_cast< struct sockaddr * >(&remote_address), sizeof(remote_address)); status < 0) {
            Error error;
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
                    if (m_non_blocking) {
                        error = Error::CONNECT_TRY_AGAIN;
                    } else {
                        error = Error::SOCKET_TIMED_OUT;
                    }
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
                    if (m_non_blocking) {
                        error = Error::CONNECT_IN_PROGRESS;
                    } else {
                        error = Error::SOCKET_TIMED_OUT;
                    }
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
                    throw std::runtime_error("Unknown error occurred");
                }
            }
            m_error = makeError(error);
            return;
        }
        m_not_ssl_connected = true;
        if (not p_ssl) {
            m_connected = true;
        }
    }
    if (p_ssl) {
        try {
            if (not m_ssl) {
                m_ssl = std::make_unique< Ssl >(m_socket);
            }
        } catch (const std::system_error& error) {
            m_error = error.code();
            return;
        }

        m_error = m_ssl->connect();

        if (m_error.value() == static_cast< int >(Error::SSL_TRY_AGAIN)) {
            m_error = makeError(Error::CONNECT_TRY_AGAIN);
        }

        if (m_error) {
            return;
        }

        bool certificate_verified;
        if (not m_host_name.empty()) {
            certificate_verified = m_ssl->verifyHost(m_host_name);
        } else {
            certificate_verified = m_ssl->verifyIp(m_ip);
        }
        const bool start_date_is_valid = m_ssl->verifyStartDate();
        const bool end_date_is_valid = m_ssl->verifyEndDate();
        if (not certificate_verified and not start_date_is_valid and not end_date_is_valid) {
            m_error = makeError(Error::SSL_CERTIFICATE_VERIFICATION_HOST);
            return;
        }
        m_connected = true;
    }
}

void mt::sockets::InetSocket::close() {
    if (m_ssl) {
        if (m_error.value() != static_cast< int >(Error::SSL_IO_ERROR) and m_error.value() != static_cast< int >(Error::SSL_FATAL_ERROR)) {
            m_ssl->shutdown();
        }
        m_ssl.reset();
    }
    ::close(m_socket);
}

void mt::sockets::InetSocket::shutdown() {
    if (const auto status = ::shutdown(m_socket, SHUT_RDWR); status < 0) {
        Error error;
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

auto mt::sockets::InetSocket::accept() -> std::optional< std::unique_ptr< mt::sockets::InetSocket > > {

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

    sockaddr_in peer_address{};
    uint32_t peer_address_length = sizeof(peer_address);
    std::unique_ptr< mt::sockets::InetSocket > socket(new mt::sockets::InetSocket(true));
    socket->m_type = m_type;
    socket->m_socket = ::accept(m_socket, reinterpret_cast< struct sockaddr * >(&peer_address), &peer_address_length);

    if (socket->m_socket < 0) {
        Error error;
        switch (errno) {
            case EAGAIN:
            case ENETDOWN:
            case ENOPROTOOPT:
            case EHOSTDOWN:
            case ENONET:
            case EHOSTUNREACH:
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
            case ENOBUFS:
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
    socket->setPort(peer_address.sin_port);
    socket->setHost(peer_address.sin_addr.s_addr);
    socket->m_connected = true;
    return socket;
}

auto mt::sockets::InetSocket::read() -> std::byte {

    std::byte byte{0};

    if (m_ssl) {
        auto [ error, value ] = m_ssl->read();
        byte = value;
        if (error and error.value() == static_cast< int >(Error::SSL_TRY_AGAIN)) {
            m_error = makeError(Error::WRITE_TRY_AGAIN);
        } else {
            m_error = error;
        }
        return byte;
    }

    const auto status = ::recv(m_socket, &byte, 1, 0);
    if (status < 0) {
        Error error;
        switch (errno) {
            case EAGAIN: {
                if (m_non_blocking) {
                    error = Error::READ_TRY_AGAIN;
                } else {
                    error = Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                if (m_non_blocking) {
                    error = tristan::sockets::Error::READ_TRY_AGAIN;
                } else {
                    error = tristan::sockets::Error::SOCKET_TIMED_OUT;
                }
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

auto mt::sockets::InetSocket::read(const uint16_t p_size) -> std::vector< std::byte > {

    if (p_size == 0) {
        return {};
    }

    std::vector< std::byte > data;

    if (m_ssl) {
        auto&& [ error, l_data ] = m_ssl->read(p_size);
        if (error and error.value() == static_cast< int >(Error::SSL_TRY_AGAIN)) {
            m_error = makeError(Error::WRITE_TRY_AGAIN);
        } else {
            m_error = error;
        }
        return l_data;
    }

    data.resize(p_size);
    if (const auto status = ::recv(m_socket, data.data(), p_size, 0); status < 0) {
        Error error;
        switch (errno) {
            case EAGAIN: {
                if (m_non_blocking) {
                    error = Error::READ_TRY_AGAIN;
                } else {
                    error = Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                if (m_non_blocking) {
                    error = tristan::sockets::Error::READ_TRY_AGAIN;
                } else {
                    error = tristan::sockets::Error::SOCKET_TIMED_OUT;
                }
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

auto mt::sockets::InetSocket::ip() const noexcept -> uint32_t {
    return m_ip;
}

auto mt::sockets::InetSocket::port() const noexcept -> uint16_t {
    return m_port;
}

auto mt::sockets::InetSocket::error() const noexcept -> std::error_code {
    return m_error;
}

auto mt::sockets::InetSocket::nonBlocking() const noexcept -> bool {
    return m_non_blocking;
}

auto mt::sockets::InetSocket::connected() const noexcept -> bool {
    return m_connected;
}

mt::sockets::InetSocket::InetSocket(bool) :
    m_socket(-1),
    m_ip(0),
    m_port(0),
    m_type(SocketType::STREAM),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_not_ssl_connected(false),
    m_connected(false) {
}

void mt::sockets::InetSocket::write_byte(const std::byte p_byte) {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
    }
    if (p_byte == std::byte{0}) {
    }

    uint8_t bytes_sent = 0;

    if (m_connected) {
        if (m_ssl) {
            if (auto error = m_ssl->write(p_byte); error and error.value() == static_cast< int >(Error::SSL_TRY_AGAIN)) {
                m_error = makeError(Error::WRITE_TRY_AGAIN);
            } else {
                m_error = error;
            }
        }
        bytes_sent = ::send(m_socket, &p_byte, 1, MSG_NOSIGNAL);
    } else {
        if (m_type == SocketType::STREAM) {
            m_error = makeError(Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_in remote_address{};
            remote_address.sin_family = AF_INET;
            remote_address.sin_addr.s_addr = m_ip;
            remote_address.sin_port = m_port;
            bytes_sent = ::sendto(m_socket, &p_byte, 1, MSG_NOSIGNAL, reinterpret_cast< sockaddr * >(&remote_address), sizeof(remote_address));
        }
    }
    if (static_cast< int8_t >(bytes_sent) < 0) {
        Error error;
        switch (errno) {
            case EACCES: {
                error = Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                if (m_non_blocking) {
                    error = Error::WRITE_TRY_AGAIN;
                } else {
                    error = Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                if (m_non_blocking) {
                    error = tristan::sockets::Error::WRITE_TRY_AGAIN;
                } else {
                    error = tristan::sockets::Error::SOCKET_TIMED_OUT;
                }
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

auto mt::sockets::InetSocket::write_range(const std::byte* p_bytes, const uint64_t p_size) -> uint64_t {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return 0;
    }
    if (p_size == 0) {
        return 0;
    }

    uint64_t bytes_sent = 0;

    if (m_connected) {
        if (m_ssl) {
            auto [ error, bytes_written ] = m_ssl->write(p_bytes, p_size);
            bytes_sent = bytes_written;
            if (error and error.value() == static_cast< int >(Error::SSL_TRY_AGAIN)) {
                m_error = makeError(Error::WRITE_TRY_AGAIN);
            } else {
                m_error = error;
            }
            return bytes_sent;
        }
        bytes_sent = ::send(m_socket, p_bytes, p_size, MSG_NOSIGNAL);
    } else {
        if (m_type == SocketType::STREAM) {
            m_error = makeError(Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_in remote_address{};
            remote_address.sin_family = AF_INET;
            remote_address.sin_addr.s_addr = m_ip;
            remote_address.sin_port = m_port;
            bytes_sent = ::sendto(m_socket, p_bytes, p_size, MSG_NOSIGNAL, reinterpret_cast< sockaddr * >(&remote_address), sizeof(remote_address));
        }
    }
    if (static_cast< int64_t >(bytes_sent) < 0) {
        Error error;
        switch (errno) {
            case EACCES: {
                error = Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                if (m_non_blocking) {
                    error = Error::WRITE_TRY_AGAIN;
                } else {
                    error = Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) and EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK: {
                if (m_non_blocking) {
                    error = tristan::sockets::Error::WRITE_TRY_AGAIN;
                } else {
                    error = tristan::sockets::Error::SOCKET_TIMED_OUT;
                }
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

auto mt::sockets::InetSocket::read_until(const std::byte p_delimiter) -> std::vector< std::byte > {
    std::vector< std::byte > data;

    while (true) {
        std::byte byte = read();
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

auto mt::sockets::InetSocket::read_until(const std::byte* p_delimiter, int64_t p_delimiter_size)
    -> std::vector< std::byte > {
    std::vector< std::byte > data;
    data.reserve(p_delimiter_size);
    while (true) {
        std::byte byte = read();
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

    if (m_error.value() == static_cast< int >(Error::READ_DONE)) {
        data.erase(data.end() - p_delimiter_size, data.end());
    }
    if (not data.empty()) {
        data.shrink_to_fit();
    }
    return data;
}
