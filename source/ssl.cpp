#include "include/ssl.hpp"
#include "include/socket_error.hpp"

#include <openssl/crypto.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

mt::sockets::Ssl::Ssl(const int32_t socket) :
    m_context(nullptr),
    m_ssl(nullptr),
    m_server_certificate(nullptr),
    method(nullptr) {

    OpenSSL_add_all_algorithms();
    method = TLS_client_method();
    if (method == nullptr) {
        const auto error = makeError(Error::SSL_METHOD_ERROR);
        throw std::system_error(error);
    }
    SSL_load_error_strings();
    m_context = SSL_CTX_new(method);
    if (m_context == nullptr) {
        const auto error = makeError(Error::SSL_CONTEXT_ERROR);
        throw std::system_error(error);
    }
    m_ssl = SSL_new(m_context);
    if (m_ssl == nullptr) {
        const auto error = makeError(Error::SSL_INIT_ERROR);
        throw std::system_error(error);
    }
    SSL_set_fd(m_ssl, socket);
}

mt::sockets::Ssl::~Ssl() {
    if (m_server_certificate != nullptr) {
        X509_free(m_server_certificate);
    }
    if (m_ssl != nullptr) {
        SSL_free(m_ssl);
    }
    if (m_context != nullptr) {
        SSL_CTX_free(m_context);
    }
}

auto mt::sockets::Ssl::connect() -> std::error_code {
    if (const auto status = SSL_connect(m_ssl); status < 0) {
        switch (SSL_get_error(m_ssl, status)) {
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT: {
                return makeError(Error::SSL_TRY_AGAIN);
            }
            default: {
                return makeError(Error::SSL_CONNECT_ERROR);
            }
        }
    }
    m_server_certificate = SSL_get_peer_certificate(m_ssl);
    if (m_server_certificate == nullptr) {
        const auto error = makeError(Error::SSL_CERTIFICATE_ERROR);
        throw std::system_error(error);
    }
    return {};
}

auto mt::sockets::Ssl::verifyHost(const std::string& host) const -> bool {
    if (host.empty()) {
        return false;
    }
    if (const auto status = X509_check_host(m_server_certificate, host.c_str(), host.size(), 0, nullptr); status < -1) {
        return false;
    }
    return true;
}

auto mt::sockets::Ssl::verifyIp(const uint32_t ip) const -> bool {
    if (ip == 0) {
        return false;
    }
    if (const auto status = X509_check_ip(m_server_certificate, reinterpret_cast< const unsigned char * >(ip), sizeof(ip), 0); status < -1) {
        return false;
    }
    return true;
}

auto mt::sockets::Ssl::verifyIp(const uint64_t ip) const -> bool {
    if (ip == 0) {
        return false;
    }
    if (const auto status = X509_check_ip(m_server_certificate, reinterpret_cast< const unsigned char * >(ip), sizeof(ip), 0); status < -1) {
        return false;
    }
    return true;
}

auto mt::sockets::Ssl::verifyStartDate() const -> bool {
    const ASN1_TIME *start_date = X509_getm_notBefore(m_server_certificate);
    int32_t day = 0;
    int32_t sec = 0;
    ASN1_TIME_diff(&day, &sec, nullptr, start_date);
    if (day < 0 or (day == 0 and sec < 0)) {
        return false;
    }

    return true;
}

auto mt::sockets::Ssl::verifyEndDate() const -> bool {
    const ASN1_TIME *start_date = X509_getm_notBefore(m_server_certificate);
    int32_t day = 0;
    int32_t sec = 0;
    ASN1_TIME_diff(&day, &sec, nullptr, start_date);
    if (day > 0 or (day == 0 and sec > 0)) {
        return false;
    }

    return true;
}

auto mt::sockets::Ssl::write(const std::byte byte) const -> std::error_code {
    uint8_t bytes_writen = 0;
    std::error_code error_code;
    if (const auto status = SSL_write_ex(m_ssl, &byte, 1, reinterpret_cast< uint64_t * >(&bytes_writen)); status <= 0) {
        switch (SSL_get_error(m_ssl, status)) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = makeError(Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE: {
                error_code = makeError(Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = makeError(Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = makeError(Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = makeError(Error::SSL_UNKNOWN_ERROR);
            }
        }
    }
    return error_code;
}

auto mt::sockets::Ssl::write(const std::vector< std::byte >::const_iterator p_begin, const std::vector< std::byte >::const_iterator p_end) const
    -> std::pair< std::error_code, uint64_t > {
    uint64_t bytes_writen = 0;
    std::error_code error_code;
    if (const auto status = SSL_write_ex(m_ssl, &*p_begin, p_end - p_begin, &bytes_writen); status <= 0) {
        switch (SSL_get_error(m_ssl, status)) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = makeError(Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE: {
                error_code = makeError(Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = makeError(Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = makeError(Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = makeError(Error::SSL_UNKNOWN_ERROR);
            }
        }
    }
    return {error_code, bytes_writen};
}

auto mt::sockets::Ssl::read() const -> std::pair< std::error_code, std::byte > {
    std::byte byte{0};
    std::error_code error_code;

    if (const auto status = SSL_read(m_ssl, &byte, 1); status <= 0) {
        switch (SSL_get_error(m_ssl, status)) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = makeError(Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE: {
                error_code = makeError(Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = makeError(Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = makeError(Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = makeError(Error::SSL_UNKNOWN_ERROR);
            }
        }
    }

    return {error_code, byte};
}

auto mt::sockets::Ssl::read(const uint16_t size) const -> std::pair< std::error_code, std::vector< std::byte > > {
    if (size == 0) {
        return {{}, {}};
    }
    std::vector< std::byte > data;
    data.resize(size);
    std::error_code error_code;

    uint64_t bytes_read = 0;

    if (const auto status = SSL_read_ex(m_ssl, data.data(), size, &bytes_read); status <= 0) {
        switch (SSL_get_error(m_ssl, status)) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = makeError(Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE: {
                error_code = makeError(Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = makeError(Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = makeError(Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = makeError(Error::SSL_UNKNOWN_ERROR);
            }
        }
    }

    if (data.size() != bytes_read) {
        data.resize(bytes_read);
    }

    return {error_code, data};
}

void mt::sockets::Ssl::shutdown() const {
    SSL_shutdown(m_ssl);
}
