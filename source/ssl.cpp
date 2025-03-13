#include "include/ssl.hpp"

#include "include/socket_error.hpp"

#include <openssl/crypto.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <filesystem>

namespace {
    [[nodiscard]] auto getDefaultSslCertificate() -> std::filesystem::path;
}  // namespace

auto mt::sockets::certificateCallBack(SSL *p_ssl, void *arg) -> int32_t {
    auto *ssl = static_cast< mt::sockets::Ssl * >(arg);
    if (ssl->m_certificate_path.empty()) {
        ssl->m_certificate_path = getDefaultSslCertificate();
    }
    if (not ssl->m_certificate_path.empty()) {
        if (SSL_use_certificate_file(p_ssl, ssl->m_certificate_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
            ssl->setError();
            return 0;
        }
    }
    if (not ssl->m_key_path.empty()) {
        if (SSL_use_PrivateKey_file(p_ssl, ssl->m_key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
            ssl->setError();
            return 0;
        }
    }
    return 1;
}

// void message_callback(const int32_t write_p, [[maybe_unused]] const int32_t version, [[maybe_unused]] const int32_t content_type,
//                       const void *buf, const size_t len, [[maybe_unused]] SSL *ssl, [[maybe_unused]] void *arg) {
//     std::string message = write_p ? "Sent: " : "Received: ";
//     message.append(static_cast<const char*>(buf), len);
//     if (isatty(STDOUT_FILENO))
//     {
//         write(STDOUT_FILENO, message.c_str(), message.size());
//     }
// }

mt::sockets::Ssl::Ssl(const int32_t socket) :
    m_context(nullptr),
    m_ssl(nullptr),
    m_server_certificate(nullptr),
    m_method(nullptr) {

    SSL_library_init();
    ERR_load_crypto_strings();

    CONF_modules_load(nullptr, nullptr, 0);
    m_method = TLS_client_method();
    if (m_method == nullptr) {
        const auto error = makeError(Error::SSL_METHOD_ERROR);
        throw std::system_error(error);
    }
    m_context = SSL_CTX_new(m_method);
    if (m_context == nullptr) {
        const auto error = makeError(Error::SSL_CONTEXT_ERROR);
        throw std::system_error(error);
    }
    SSL_CTX_set_min_proto_version(m_context, TLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(m_context, TLS1_3_VERSION);
    constexpr uint64_t flags = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    SSL_CTX_set_options(m_context, flags);
    SSL_CTX_set_cert_cb(m_context, mt::sockets::certificateCallBack, this);
    m_ssl = SSL_new(m_context);
    if (m_ssl == nullptr) {
        const auto error = makeError(Error::SSL_INIT_ERROR);
        throw std::system_error(error);
    }
    // SSL_set_msg_callback(m_ssl, message_callback);
    [[maybe_unused]] auto min_tls_version = SSL_ctrl(m_ssl, SSL_CTRL_GET_MIN_PROTO_VERSION, 0, nullptr);
    [[maybe_unused]] auto max_tls_version = SSL_ctrl(m_ssl, SSL_CTRL_GET_MAX_PROTO_VERSION, 0, nullptr);

    SSL_set_fd(m_ssl, socket);
}

mt::sockets::Ssl::~Ssl() {
    CONF_modules_unload(1);
    EVP_cleanup();
    ERR_free_strings();
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

void mt::sockets::Ssl::setCertificate(std::filesystem::path p_certificate) {
    m_certificate_path = std::move(p_certificate);
}

void mt::sockets::Ssl::setKey(std::filesystem::path p_key) {
    m_key_path = std::move(p_key);
}

void mt::sockets::Ssl::setHost(const std::string& p_host) {
    SSL_set_tlsext_host_name(m_ssl, p_host.c_str());
}

auto mt::sockets::Ssl::connect() -> std::error_code {
    if (const auto status = SSL_connect(m_ssl); status < 0) {
        switch (const auto error = SSL_get_error(m_ssl, status); error) {
            case SSL_ERROR_WANT_WRITE:
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_CONNECT:
            case SSL_ERROR_WANT_ACCEPT: {
                return makeError(Error::SSL_TRY_AGAIN);
            }
            default: {
                setError();
                return makeError(Error::SSL_CONNECT_ERROR);
            }
        }
    }
    m_server_certificate = SSL_get_peer_certificate(m_ssl);
    if (m_server_certificate == nullptr) {
        setError();
        return makeError(Error::SSL_CERTIFICATE_ERROR);
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

auto mt::sockets::Ssl::write(const std::byte *bytes, const uint64_t size) const -> std::pair< std::error_code, uint64_t > {
    uint64_t bytes_writen = 0;
    std::error_code error_code;
    if (const auto status = SSL_write_ex(m_ssl, bytes, size, &bytes_writen); status <= 0) {
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

void mt::sockets::Ssl::setError() {
    if (not m_ssl_error_description.empty()) {
        std::ranges::fill(m_ssl_error_description, 0);
    } else {
        m_ssl_error_description.resize(256);
    }
    const auto error_code = ERR_get_error();
    ERR_error_string_n(error_code, m_ssl_error_description.data(), m_ssl_error_description.size());
}

namespace {
    [[nodiscard]] auto getDefaultSslCertificate() -> std::filesystem::path {
        return std::filesystem::path{X509_get_default_cert_file()};
    }
}  // namespace