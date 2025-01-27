#ifndef INET_SOCKET_HPP
#define INET_SOCKET_HPP

#include "socket_common.hpp"

#include <chrono>

namespace mt::sockets {

    class Ssl;

    class InetSocket {
      public:
        explicit InetSocket(SocketType p_socket_type = SocketType::STREAM);
        InetSocket(const InetSocket&) = delete;
        InetSocket(InetSocket&&) = delete;
        InetSocket& operator=(const InetSocket&) = delete;
        InetSocket& operator=(InetSocket&&) = delete;
        ~InetSocket();

        void setHost(uint32_t p_ip, std::string p_host_name = "");
        void setPort(uint16_t p_port);
        void setNonBlocking(bool p_non_blocking = true);
        void setTimeOut(std::chrono::seconds p_seconds);
        void resetError();
        void bind();
        void listen(uint32_t p_connection_count_limit);
        void connect(bool p_ssl = true);
        void close();
        void shutdown();
        template < class ValueType >
            requires std::is_same_v< ValueType, std::byte > or concepts::write_compatible< ValueType >
        void write(ValueType p_value);
        auto write(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> uint64_t
            requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
                 and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >);
        auto write(std::ranges::input_range auto&& range) -> uint64_t;
        [[nodiscard]] auto accept() -> std::optional< std::unique_ptr< InetSocket > >;
        [[nodiscard]] auto read() -> std::byte;
        [[nodiscard]] auto read(uint16_t p_size) -> std::vector< std::byte >;
        template < class ValueType >
            requires std::is_same_v< std::decay_t< ValueType >, std::byte > or concepts::delimiter_compatible< ValueType >
        [[nodiscard]] auto readUntil(ValueType p_value) -> std::vector< std::byte >;
        [[nodiscard]] auto readUntil(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> std::vector< std::byte >
            requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
                 and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >);
        [[nodiscard]] auto readUntil(std::ranges::input_range auto&& range) -> std::vector< std::byte >;
        [[nodiscard]] auto ip() const noexcept -> uint32_t;
        [[nodiscard]] auto port() const noexcept -> uint16_t;
        [[nodiscard]] auto error() const noexcept -> std::error_code;
        [[nodiscard]] auto nonBlocking() const noexcept -> bool;
        [[nodiscard]] auto connected() const noexcept -> bool;

      private:
        explicit InetSocket(bool);
        void write_byte(std::byte p_byte);
        auto write_range(const std::byte *p_bytes, uint64_t p_size) -> uint64_t;
        auto read_until(std::byte p_delimiter) -> std::vector< std::byte >;
        auto read_until(const std::byte* p_delimiter, int64_t p_delimiter_size) -> std::vector< std::byte >;

        std::string m_host_name;

        int32_t m_socket;
        uint32_t m_ip;

        std::error_code m_error;

        uint16_t m_port;

        std::unique_ptr< Ssl > m_ssl;
        SocketType m_type;

        bool m_non_blocking;
        bool m_bound;
        bool m_listening;
        bool m_not_ssl_connected;
        bool m_connected;
    };

    template < class ValueType >
        requires std::is_same_v< ValueType, std::byte > or concepts::write_compatible< ValueType >
    void InetSocket::write(ValueType p_value) {
        if constexpr (constexpr auto value_size = sizeof(std::decay_t< ValueType >); value_size == 1) {
            if constexpr (std::is_same_v< ValueType, std::byte >) {
                write_byte(p_value);
            } else {
                write_byte(static_cast< std::byte >(p_value));
            }
        } else {
            write_range(reinterpret_cast< std::byte * >(&p_value), value_size);
        }
    }

    auto InetSocket::write(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> uint64_t
        requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
             and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >)
    {
        if constexpr (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte >) {
            return write_range(&*begin, end - begin);
        } else {
            std::vector< std::byte > data;
            if constexpr (constexpr auto value_size = sizeof(std::decay_t< decltype(*begin) >); value_size == 1) {
                return write_range(reinterpret_cast<std::byte*>(&*begin), end - begin);
            } else {
                uint64_t bytes_written{0};
                while (begin != end) {
                    bytes_written += write_range(reinterpret_cast< std::byte* >(&*begin), value_size);
                    ++begin;
                }
                return bytes_written;
            }
        }
    }

    auto InetSocket::write(std::ranges::input_range auto&& range) -> uint64_t {
        return write(range.begin(), range.end());
    }

    template < class ValueType >
        requires std::is_same_v< std::decay_t< ValueType >, std::byte > or concepts::delimiter_compatible< ValueType >
    auto InetSocket::readUntil(ValueType p_value) -> std::vector< std::byte > {
        if constexpr (constexpr auto value_size = sizeof(std::decay_t< ValueType >); value_size == 1) {
            if constexpr (std::is_same_v< ValueType, std::byte >) {
                return read_until(p_value);
            } else {
                return read_until(static_cast< std::byte >(p_value));
            }
        } else {
            return read_until(reinterpret_cast< std::byte * >(&p_value), value_size);
        }
    }

    auto InetSocket::readUntil(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> std::vector< std::byte >
        requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
              && (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >)
    {
        if constexpr (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte >) {
            return read_until(&*begin, end - begin);
        } else {
            if constexpr (constexpr auto value_size = sizeof(std::decay_t< decltype(*begin) >); value_size == 1) {
                return read_until(reinterpret_cast<std::byte*>(&*begin), end - begin);
            } else {
                auto size = (end - begin) * value_size;
                std::vector<std::byte> delimiter;
                delimiter.reserve(size);
                while (begin != end) {
                    std::copy_n(reinterpret_cast< std::byte* >(&*begin), value_size, std::back_inserter(delimiter));
                    ++begin;
                }
                return read_until(delimiter.data(), std::ssize(delimiter));
            }
        }
    }

    auto InetSocket::readUntil(std::ranges::input_range auto&& range) -> std::vector< std::byte > {
        return readUntil(range.begin(), range.end());
    }
}  // namespace mt::sockets

#endif  //INET_SOCKET_HPP
