#ifndef OPEN_SSL_HPP
#define OPEN_SSL_HPP

#include <string>
#include <vector>
#include <memory>

struct ssl_ctx_st;
struct ssl_st;
struct x509_st;
struct ssl_method_st;

namespace mt::sockets {

    class Ssl {
        friend class TcpSocket;

    public:
        explicit Ssl(int32_t socket);
        Ssl(const Ssl& other) = delete;
        Ssl(Ssl&& other) = delete;
        Ssl& operator=(const Ssl& other) = delete;
        Ssl& operator=(Ssl&& other) = delete;
        ~Ssl();

    private:

        [[nodiscard]] auto connect() -> std::error_code;
        [[nodiscard]] auto verifyHost(const std::string& host) const -> bool;
        [[nodiscard]] auto verifyIp(uint32_t ip) const -> bool;
        [[nodiscard]] auto verifyIp(uint64_t ip) const -> bool;
        [[nodiscard]] auto verifyStartDate() const -> bool;
        [[nodiscard]] auto verifyEndDate() const -> bool;

        [[nodiscard]] auto write(std::byte byte) const -> std::error_code;
        [[nodiscard]] auto write(const std::byte* bytes, uint64_t size) const -> std::pair< std::error_code, uint64_t >;

        [[nodiscard]] auto read() const -> std::pair< std::error_code, std::byte >;
        [[nodiscard]] auto read(uint16_t size) const -> std::pair< std::error_code, std::vector< std::byte > >;

        void shutdown() const;

        ssl_ctx_st* m_context;
        ssl_st* m_ssl;
        x509_st* m_server_certificate;

        const ssl_method_st* method;
    };

}  // namespace tristan::sockets

#endif  //OPEN_SSL_HPP
