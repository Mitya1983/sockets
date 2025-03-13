// Created by Mitia Tristan on 29.01.25.

#include "include/udp_socket.hpp"
#include "include/socket_error.hpp"

#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>

mt::sockets::UdpSocket::UdpSocket() {
    const auto protocol = getprotobyname("udp");
    m_socket = socket(AF_INET, SOCK_DGRAM, protocol->p_proto);
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

mt::sockets::UdpSocket::~UdpSocket() {
    close();
}

void mt::sockets::UdpSocket::setDestinationHost(const uint32_t p_ip) {
    m_destination_ip = p_ip;
}

void mt::sockets::UdpSocket::setLocalHost(const uint32_t p_ip) {
    m_local_ip = p_ip;
}

void mt::sockets::UdpSocket::setLocalPort(const uint16_t p_port) {
    m_local_port = p_port;
}

void mt::sockets::UdpSocket::setDestinationPort(const uint16_t p_port) {
    m_destination_port = p_port;
}

void mt::sockets::UdpSocket::setNonBlocking(const bool p_non_blocking) {
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

void mt::sockets::UdpSocket::setTimeOut(std::chrono::seconds p_seconds) {
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

void mt::sockets::UdpSocket::resetError() {
    m_error = makeError(Error::SUCCESS);
}

void mt::sockets::UdpSocket::bind() {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return;
    }
    if (m_bound) {
        return;
    }
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = m_local_ip;
    address.sin_port = m_local_port;

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

void mt::sockets::UdpSocket::close() const {
    ::close(m_socket);
}

void mt::sockets::UdpSocket::shutdown() {
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

auto mt::sockets::UdpSocket::read() -> std::vector< std::byte > {
    if (m_socket == -1) {
        m_error = makeError(Error::SOCKET_NOT_INITIALISED);
        return {};
    }
    std::vector< std::byte > buffer{};
    buffer.resize(512);
    sockaddr_in sender{};
    sender.sin_family = AF_INET;
    sender.sin_addr.s_addr = m_local_port;
    sender.sin_port = m_local_ip;
    auto sockaddre_size = sizeof(sender);
    const auto bytes_read
        = ::recvfrom(m_socket, buffer.data(), std::ssize(buffer), MSG_WAITALL, reinterpret_cast< sockaddr * >(&sender), reinterpret_cast< socklen_t * >(&sockaddre_size));
    if (bytes_read < 0) {
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
            case EADDRINUSE: {
                error = Error::LISTEN_ADDRESS_IN_USE;
                break;
            }
            case EBADF: {
                error = Error::LISTEN_BAD_FILE_DESCRIPTOR;
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
            case ENOTSOCK: {
                error = Error::LISTEN_FILE_DESCRIPTOR_IS_NOT_SOCKET;
                break;
            }
            case EOPNOTSUPP: {
                error = Error::LISTEN_PROTOCOL_NOT_SUPPORTED;
                break;
            }
            case ETIMEDOUT: {
                error = Error::SOCKET_TIMED_OUT;
                break;
            }
            case ENOBUFS: {
                error = Error::READ_NO_BUFFER;
                break;
            }
            case ENOMEM: {
                error = Error::READ_NO_MEMORY;
                break;
            }
            default: {
                throw std::runtime_error("Unknown error occurred");
            }
        }
        m_error = makeError(error);
    }
    if (bytes_read > 0) {
        buffer.resize(bytes_read);
    }
    return buffer;
}

auto mt::sockets::UdpSocket::ip() const noexcept -> uint32_t {
    return m_destination_ip;
}

auto mt::sockets::UdpSocket::port() const noexcept -> uint16_t {
    return m_destination_port;
}

auto mt::sockets::UdpSocket::error() const noexcept -> std::error_code {
    return m_error;
}

auto mt::sockets::UdpSocket::nonBlocking() const noexcept -> bool {
    return m_non_blocking;
}

auto mt::sockets::UdpSocket::write(const std::byte *p_bytes, const uint64_t p_size) -> uint64_t {
    sockaddr_in remote_address{};
    remote_address.sin_family = AF_INET;
    remote_address.sin_addr.s_addr = m_destination_ip;
    remote_address.sin_port = m_destination_port;
    int64_t status;
    if (status = ::sendto(m_socket, p_bytes, p_size, MSG_NOSIGNAL, reinterpret_cast< sockaddr * >(&remote_address), sizeof(remote_address)); status < 0) {
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
    return status;
}
