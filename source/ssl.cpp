#include "ssl.hpp"
#include "socket_error.hpp"

#include <openssl/crypto.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

mt::sockets::Ssl::Ssl(int32_t socket) :
    m_context(nullptr),
    m_ssl(nullptr),
    m_server_certificate(nullptr),
    method(nullptr) {

    OpenSSL_add_all_algorithms();
    method = TLS_client_method();
    if (method == nullptr) {
        auto error = mt::sockets::makeError(mt::sockets::Error::SSL_METHOD_ERROR);
        throw std::system_error(error);
    }
    SSL_load_error_strings();
    m_context = SSL_CTX_new(method);
    if (m_context == nullptr) {
        auto error = mt::sockets::makeError(mt::sockets::Error::SSL_CONTEXT_ERROR);
        throw std::system_error(error);
    }
    m_ssl = SSL_new(m_context);
    if (m_ssl == nullptr) {
        auto error = mt::sockets::makeError(mt::sockets::Error::SSL_INIT_ERROR);
        throw std::system_error(error);
    }
    SSL_set_fd(m_ssl, socket);
}

auto mt::sockets::Ssl::create(int32_t socket) -> std::unique_ptr< Ssl > { return std::unique_ptr< Ssl >(new mt::sockets::Ssl(socket)); }

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
    auto status = SSL_connect(m_ssl);
    if (status < 0) {
        auto error = SSL_get_error(m_ssl, status);
        switch (error) {
            case SSL_ERROR_WANT_WRITE:{//NOLINT
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_READ:{
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_CONNECT: {
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_ACCEPT: {
                return mt::sockets::makeError(mt::sockets::Error::SSL_TRY_AGAIN);
            }
            default: {
                return mt::sockets::makeError(mt::sockets::Error::SSL_CONNECT_ERROR);
            }
        }
    }

    m_server_certificate = SSL_get_peer_certificate(m_ssl);
    if (m_server_certificate == nullptr) {
        auto error = mt::sockets::makeError(mt::sockets::Error::SSL_CERTIFICATE_ERROR);
        throw std::system_error(error);
    }
    return {};
}

auto mt::sockets::Ssl::verifyHost(const std::string& host) -> bool {
    if (host.empty()) {
        return false;
    }
    auto status = X509_check_host(m_server_certificate, host.c_str(), host.size(), 0, nullptr);
    if (status < -1) {
        return false;
    }
    return true;
}

auto mt::sockets::Ssl::verifyIp(uint32_t ip) -> bool {
    if (ip == 0) {
        return false;
    }
    auto status = X509_check_ip(m_server_certificate, reinterpret_cast< const unsigned char* >(ip), sizeof(ip), 0);
    if (status < -1) {
        return false;
    }
    return true;
}

auto mt::sockets::Ssl::verifyIp(uint64_t ip) -> bool {
    if (ip == 0) {
        return false;
    }
    auto status = X509_check_ip(m_server_certificate, reinterpret_cast< const unsigned char* >(ip), sizeof(ip), 0);
    if (status < -1) {
        return false;
    }
    return true;
}

auto mt::sockets::Ssl::verifyStartDate() -> bool {
    ASN1_TIME* start_date = X509_getm_notBefore(m_server_certificate);
    int day = 0;
    int sec = 0;
    ASN1_TIME_diff(&day, &sec, nullptr, start_date);
    if (day < 0 || (day == 0 && sec < 0)) {
        return false;
    }

    return true;
}

auto mt::sockets::Ssl::verifyEndDate() -> bool {
    ASN1_TIME* start_date = X509_getm_notBefore(m_server_certificate);
    int day = 0;
    int sec = 0;
    ASN1_TIME_diff(&day, &sec, nullptr, start_date);
    if (day > 0 || (day == 0 && sec > 0)) {
        return false;
    }

    return true;
}

auto mt::sockets::Ssl::write(uint8_t byte) -> std::pair< std::error_code, uint8_t > {
    uint8_t bytes_writen = 0;
    std::error_code error_code;
    auto status = SSL_write_ex(m_ssl, &byte, 1, reinterpret_cast< uint64_t * >(&bytes_writen));
    if (status <= 0) {
        int32_t error = SSL_get_error(m_ssl, status);
        switch (error) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ: {
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_WRITE: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_UNKNOWN_ERROR);
            }
        }
    }
    return {error_code, bytes_writen};
}

auto mt::sockets::Ssl::write(const std::vector< uint8_t >& data, uint16_t size, uint64_t offset) -> std::pair< std::error_code, uint64_t > {
    uint64_t bytes_writen = 0;
    std::error_code error_code;
    auto status = SSL_write_ex(m_ssl, data.data() + offset, size, &bytes_writen);
    if (status <= 0) {
        int32_t error = SSL_get_error(m_ssl, status);
        switch (error) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ: {
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_WRITE: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_UNKNOWN_ERROR);
            }
        }
    }
    return {error_code, bytes_writen};
}

auto mt::sockets::Ssl::read() -> std::pair< std::error_code, uint8_t > {
    uint8_t byte = 0;
    std::error_code error_code;

    auto status = SSL_read(m_ssl, &byte, 1);
    if (status <= 0) {
        int32_t error = SSL_get_error(m_ssl, status);
        switch (error) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ: {
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_WRITE: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_UNKNOWN_ERROR);
            }
        }
    }

    return {error_code, byte};
}
auto mt::sockets::Ssl::read(std::vector<uint8_t>& data, uint16_t size) -> std::pair< std::error_code, std::vector< uint8_t > > {
    if (size == 0) {
        return {{}, {}};
    }

    data.resize(size);
    std::error_code error_code;

    uint64_t bytes_read = 0;

    auto status = SSL_read_ex(m_ssl, data.data(), size, &bytes_read);

    if (status <= 0) {
        int32_t error = SSL_get_error(m_ssl, status);
        switch (error) {
            case SSL_ERROR_NONE: {
                break;
            }
            case SSL_ERROR_ZERO_RETURN: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_CLOSED_BY_PEER);
                break;
            }
            case SSL_ERROR_WANT_READ: {
                [[fallthrough]];
            }
            case SSL_ERROR_WANT_WRITE: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_TRY_AGAIN);
                break;
            }
            case SSL_ERROR_SYSCALL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_IO_ERROR);
                break;
            }
            case SSL_ERROR_SSL: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_FATAL_ERROR);
                break;
            }
            default: {
                error_code = mt::sockets::makeError(mt::sockets::Error::SSL_UNKNOWN_ERROR);
            }
        }
    }

    if (data.size() != bytes_read) {
        data.resize(bytes_read);
    }

    return {error_code, data};
}

void mt::sockets::Ssl::shutdown() { SSL_shutdown(m_ssl); }
