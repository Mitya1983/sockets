#include "inet_socket.hpp"
#include "socket_error.hpp"
#include "ssl.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>

mt::sockets::InetSocket::InetSocket(mt::sockets::SocketType p_socket_type) :
    m_socket(-1),
    m_ip(0),
    m_port(0),
    m_type(p_socket_type),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_not_ssl_connected(false),
    m_connected(false) {

    auto protocol = getprotobyname("tcp");
    if (m_type == mt::sockets::SocketType::STREAM) {
        m_socket = socket(AF_INET, SOCK_STREAM, protocol->p_proto);
    } else {
        m_socket = socket(AF_INET, SOCK_DGRAM, protocol->p_proto);
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

mt::sockets::InetSocket::~InetSocket() { InetSocket::close(); }

void mt::sockets::InetSocket::setHost(uint32_t p_ip, const std::string& p_host_name) {
    m_ip = p_ip;
    if (not p_host_name.empty()) {
        m_host_name = p_host_name;
    }
}

void mt::sockets::InetSocket::setPort(uint16_t p_port) { m_port = p_port; }

void mt::sockets::InetSocket::setNonBlocking(bool p_non_blocking) {
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

void mt::sockets::InetSocket::setTimeOut(std::chrono::seconds p_seconds) {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_non_blocking){
        return;
    }
    struct timeval l_timeval{};
    l_timeval.tv_sec = p_seconds.count();
    auto status = setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &l_timeval, sizeof (struct timeval));
    if (status == -1){
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_SET_TIMEOUT_ERROR);
        return;
    }
    status = setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &l_timeval, sizeof (struct timeval));
    if (status == -1){
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_SET_TIMEOUT_ERROR);
        return;
    }
}

void mt::sockets::InetSocket::resetError() { m_error = mt::sockets::makeError(mt::sockets::Error::SUCCESS); }

void mt::sockets::InetSocket::bind() {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_bound) {
        return;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = m_ip;
    address.sin_port = m_port;
    auto status = ::bind(m_socket, reinterpret_cast< struct sockaddr* >(&address), sizeof(address));
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
        }
        m_error = mt::sockets::makeError(error);
    }
    if (not m_error) {
        m_bound = true;
    }
}

void mt::sockets::InetSocket::listen(uint32_t p_connection_count_limit) {
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

void mt::sockets::InetSocket::connect(bool p_ssl) {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_listening) {
        m_error = mt::sockets::makeError(mt::sockets::Error::CONNECT_SOCKET_IS_IN_LISTEN_MODE);
        return;
    }
    if (not m_not_ssl_connected) {
        sockaddr_in remote_address{};
        remote_address.sin_family = AF_INET;
        remote_address.sin_addr.s_addr = m_ip;
        remote_address.sin_port = m_port;
        int32_t status = ::connect(m_socket, reinterpret_cast< struct sockaddr* >(&remote_address), sizeof(remote_address));
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
                    if (m_non_blocking) {
                        error = mt::sockets::Error::CONNECT_TRY_AGAIN;
                    } else {
                        error = mt::sockets::Error::SOCKET_TIMED_OUT;
                    }
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
                    if (m_non_blocking) {
                        error = mt::sockets::Error::CONNECT_IN_PROGRESS;
                    } else {
                        error = mt::sockets::Error::SOCKET_TIMED_OUT;
                    }
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
            }
            m_error = mt::sockets::makeError(error);
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
                m_ssl = mt::sockets::Ssl::create(m_socket);
            }
        } catch (const std::system_error& error) {
            m_error = error.code();
            return;
        }

        m_error = m_ssl->connect();

        if (m_error.value() == static_cast< int >(mt::sockets::Error::SSL_TRY_AGAIN)) {
            m_error = mt::sockets::makeError(mt::sockets::Error::CONNECT_TRY_AGAIN);
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
        bool start_date_is_valid = m_ssl->verifyStartDate();
        bool end_date_is_valid = m_ssl->verifyEndDate();
        if (not certificate_verified && not start_date_is_valid && not end_date_is_valid) {
            m_error = mt::sockets::makeError(mt::sockets::Error::SSL_CERTIFICATE_VERIFICATION_HOST);
            return;
        }
        m_connected = true;
    }
}

void mt::sockets::InetSocket::close() {
    if (m_ssl) {
        if (m_error.value() != static_cast< int >(mt::sockets::Error::SSL_IO_ERROR)
            && m_error.value() != static_cast< int >(mt::sockets::Error::SSL_FATAL_ERROR)) {
            m_ssl->shutdown();
        }
        m_ssl.reset();
    }
    ::close(m_socket);
}

void mt::sockets::InetSocket::shutdown() {
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

auto mt::sockets::InetSocket::accept() -> std::optional< std::unique_ptr< mt::sockets::InetSocket > > {

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

    sockaddr_in peer_address{};
    uint32_t peer_address_length = sizeof(peer_address);
    std::unique_ptr< mt::sockets::InetSocket > socket(new mt::sockets::InetSocket(true));
    socket->m_type = m_type;
    socket->m_socket = ::accept(m_socket, reinterpret_cast< struct sockaddr* >(&peer_address), &peer_address_length);

    if (socket->m_socket < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EAGAIN: {//NOLINT
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
    socket->setPort(peer_address.sin_port);
    socket->setHost(peer_address.sin_addr.s_addr);
    socket->m_connected = true;
    return socket;
}

auto mt::sockets::InetSocket::write(uint8_t p_byte) -> uint8_t {
    if (m_socket == -1) {
        m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_INITIALISED);
        return 0;
    }
    if (p_byte == 0) {
        return 0;
    }

    uint8_t bytes_sent = 0;

    if (m_connected) {
        if (m_ssl) {
            auto ssl_write_result = m_ssl->write(p_byte);
            bytes_sent = ssl_write_result.second;
            if (ssl_write_result.first && ssl_write_result.first.value() == static_cast< int >(mt::sockets::Error::SSL_TRY_AGAIN)) {
                m_error = mt::sockets::makeError(mt::sockets::Error::WRITE_TRY_AGAIN);
            } else {
                m_error = ssl_write_result.first;
            }
            return bytes_sent;
        }
        bytes_sent = ::send(m_socket, &p_byte, 1, MSG_NOSIGNAL);
    } else {
        if (m_type == mt::sockets::SocketType::STREAM) {
            m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_in remote_address{};
            remote_address.sin_family = AF_INET;
            remote_address.sin_addr.s_addr = m_ip;
            remote_address.sin_port = m_port;
            bytes_sent = ::sendto(m_socket, &p_byte, 1, MSG_NOSIGNAL, reinterpret_cast< struct sockaddr* >(&remote_address), sizeof(remote_address));
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
                if (m_non_blocking) {
                    error = mt::sockets::Error::WRITE_TRY_AGAIN;
                } else {
                    error = mt::sockets::Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
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

auto mt::sockets::InetSocket::write(const std::vector< uint8_t >& p_data, uint16_t p_size, uint64_t p_offset) -> uint64_t {

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
        if (m_ssl) {
            auto ssl_write_result = m_ssl->write(p_data, l_size, p_offset);
            bytes_sent = ssl_write_result.second;
            if (ssl_write_result.first && ssl_write_result.first.value() == static_cast< int >(mt::sockets::Error::SSL_TRY_AGAIN)) {
                m_error = mt::sockets::makeError(mt::sockets::Error::WRITE_TRY_AGAIN);
            } else {
                m_error = ssl_write_result.first;
            }
            return bytes_sent;
        }
        bytes_sent = ::send(m_socket, p_data.data() + p_offset, l_size, MSG_NOSIGNAL);
    } else {
        if (m_type == mt::sockets::SocketType::STREAM) {
            m_error = mt::sockets::makeError(mt::sockets::Error::SOCKET_NOT_CONNECTED);
        } else {
            sockaddr_in remote_address{};
            remote_address.sin_family = AF_INET;
            remote_address.sin_addr.s_addr = m_ip;
            remote_address.sin_port = m_port;
            bytes_sent
                = ::sendto(m_socket, p_data.data() + p_offset, l_size, MSG_NOSIGNAL, reinterpret_cast< struct sockaddr* >(&remote_address), sizeof(remote_address));
        }
    }
    if (static_cast< int64_t >(bytes_sent) < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EACCES: {
                error = mt::sockets::Error::WRITE_ACCESS;
                break;
            }
            case EAGAIN: {
                if (m_non_blocking) {
                    error = mt::sockets::Error::WRITE_TRY_AGAIN;
                } else {
                    error = mt::sockets::Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
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

auto mt::sockets::InetSocket::read() -> uint8_t {

    uint8_t byte = 0;

    if (m_ssl) {
        auto ssl_read_status = m_ssl->read();
        byte = ssl_read_status.second;
        if (ssl_read_status.first && ssl_read_status.first.value() == static_cast< int >(mt::sockets::Error::SSL_TRY_AGAIN)) {
            m_error = mt::sockets::makeError(mt::sockets::Error::WRITE_TRY_AGAIN);
        } else {
            m_error = ssl_read_status.first;
        }
        return byte;
    }

    auto status = ::recv(m_socket, &byte, 1, 0);
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EAGAIN: {
                if (m_non_blocking) {
                    error = mt::sockets::Error::READ_TRY_AGAIN;
                } else {
                    error = mt::sockets::Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
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

auto mt::sockets::InetSocket::read(uint16_t p_size) -> std::vector< uint8_t > {

    if (p_size == 0) {
        return {};
    }

    std::vector< uint8_t > data;

    if (m_ssl) {
        auto ssl_read_status = m_ssl->read(data, p_size);
        if (ssl_read_status.first && ssl_read_status.first.value() == static_cast< int >(mt::sockets::Error::SSL_TRY_AGAIN)) {
            m_error = mt::sockets::makeError(mt::sockets::Error::WRITE_TRY_AGAIN);
        } else {
            m_error = ssl_read_status.first;
        }
        return data;
    }

    data.resize(p_size);
    auto status = ::recv(m_socket, data.data(), p_size, 0);
    if (status < 0) {
        mt::sockets::Error error{};
        switch (errno) {
            case EAGAIN: {
                if (m_non_blocking) {
                    error = mt::sockets::Error::READ_TRY_AGAIN;
                } else {
                    error = mt::sockets::Error::SOCKET_TIMED_OUT;
                }
                break;
            }
#if defined(EWOULDBLOCK) && EWOULDBLOCK != EAGAIN
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
    } else if (status == 0){
        m_error = mt::sockets::makeError(mt::sockets::Error::READ_EOF);
    }
    data.shrink_to_fit();
    if (data.at(0) == 0) {
        return {};
    }
    return data;
}

auto mt::sockets::InetSocket::readUntil(uint8_t p_delimiter) -> std::vector< uint8_t > {

    std::vector< uint8_t > data;

    while (true) {
        uint8_t byte = InetSocket::read();
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

auto mt::sockets::InetSocket::readUntil(const std::vector< uint8_t >& p_delimiter) -> std::vector< uint8_t > {

    std::vector< uint8_t > data;
    data.reserve(p_delimiter.size());
    while (true) {
        uint8_t byte = InetSocket::read();
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

auto mt::sockets::InetSocket::ip() const noexcept -> uint32_t { return m_ip; }

auto mt::sockets::InetSocket::port() const noexcept -> uint16_t { return m_port; }

auto mt::sockets::InetSocket::error() const noexcept -> std::error_code { return m_error; }

auto mt::sockets::InetSocket::nonBlocking() const noexcept -> bool { return m_non_blocking; }

auto mt::sockets::InetSocket::connected() const noexcept -> bool { return m_connected; }

mt::sockets::InetSocket::InetSocket(bool) :
    m_socket(-1),
    m_ip(0),
    m_port(0),
    m_type(mt::sockets::SocketType::STREAM),
    m_non_blocking(false),
    m_bound(false),
    m_listening(false),
    m_not_ssl_connected(false),
    m_connected(false) { }
