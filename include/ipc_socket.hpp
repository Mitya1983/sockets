#ifndef IPC_SOCKET_HPP
#define IPC_SOCKET_HPP

#include "socket_common.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <memory>
#include <vector>

namespace mt::sockets {

    class IpcSocket {
      public:
        explicit IpcSocket(SocketType p_socket_type = SocketType::STREAM);
        IpcSocket(const IpcSocket&) = delete;
        IpcSocket(IpcSocket&&) = delete;
        IpcSocket& operator=(const IpcSocket&) = delete;
        IpcSocket& operator=(IpcSocket&&) = delete;
        ~IpcSocket();

        void setName(const std::string& p_name, bool p_global_namespace);
        void setPeerName(const std::string& p_name, bool p_global_namespace);
        void setNonBlocking(bool p_non_blocking = true);
        void resetError();
        void bind();
        void listen(uint32_t p_connection_count_limit);
        void connect();
        void close() const;
        void shutdown();
        template < class ValueType >
            requires std::is_same_v< ValueType, std::byte > or concepts::write_compatible< ValueType >
        void write(ValueType p_value);
        auto write(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> uint64_t
            requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
                 and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >);
        auto write(std::ranges::input_range auto&& range) -> uint64_t;
        [[nodiscard]] auto accept() -> std::optional< std::unique_ptr< IpcSocket > >;
        [[nodiscard]] auto read() -> std::byte;
        [[nodiscard]] auto read(uint16_t p_size) -> std::vector< std::byte >;
        template < class ValueType >
            requires std::is_same_v< std::decay_t< ValueType >, std::byte > or concepts::delimiter_compatible< ValueType >
        [[nodiscard]] auto readUntil(ValueType p_value) -> std::vector< std::byte >;
        [[nodiscard]] auto readUntil(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> std::vector< std::byte >
            requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
                 and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >);
        [[nodiscard]] auto readUntil(std::ranges::input_range auto&& range) -> std::vector< std::byte >;
        [[nodiscard]] auto name() const noexcept -> const std::string&;
        [[nodiscard]] auto peerName() const noexcept -> const std::string&;
        [[nodiscard]] auto error() const noexcept -> std::error_code;
        [[nodiscard]] auto nonBlocking() const noexcept -> bool;
        [[nodiscard]] auto connected() const noexcept -> bool;

      private:
        explicit IpcSocket(bool);
        void write_byte(std::byte p_byte);
        auto write_vector(std::vector< std::byte >::const_iterator p_begin, std::vector< std::byte >::const_iterator p_end) -> uint64_t;
        auto read_until(std::byte p_delimiter) -> std::vector< std::byte >;
        auto read_until(std::vector< std::byte >::const_iterator p_delimiter_begin, std::vector< std::byte >::const_iterator p_delimiter_end) -> std::vector< std::byte >;

        std::string m_name;
        std::string m_peer_name;

        std::error_code m_error;

        int32_t m_socket;

        SocketType m_type;

        bool m_global_namespace;
        bool m_peer_global_namespace;

        bool m_non_blocking;
        bool m_bound;
        bool m_listening;
        bool m_connected;
    };

    template < class ValueType >
        requires std::is_same_v< ValueType, std::byte > or concepts::write_compatible< ValueType >
    void IpcSocket::write(ValueType p_value) {
        if constexpr (constexpr auto value_size = sizeof(std::decay_t< ValueType >); value_size == 1) {
            if constexpr (std::is_same_v< ValueType, std::byte >) {
                write_byte(p_value);
            } else {
                write_byte(static_cast< std::byte >(p_value));
            }
        } else {
            const std::vector< std::byte > data{reinterpret_cast< std::byte * >(&p_value), reinterpret_cast< std::byte * >(&p_value) + value_size};
            write_vector(data.begin(), data.end());
        }
    }

    auto IpcSocket::write(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> uint64_t
        requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
             and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >)
    {
        if constexpr (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte >) {
            return write_vector(begin, end);
        } else {
            std::vector< std::byte > data;
            if constexpr (constexpr auto value_size = sizeof(std::decay_t< decltype(*begin) >); value_size == 1) {
                data.reserve(end - begin);
                std::transform(begin, end, data, []< typename ValueType >(ValueType&& value) -> std::byte {
                    return static_cast< std::byte >(value);
                });
            } else {
                auto size = (end - begin) * value_size;
                data.reserve(size);
                while (begin != end) {
                    std::copy_n(reinterpret_cast< std::byte >(&*begin), value_size, std::back_inserter(data));
                    ++begin;
                }
            }
            return write_vector(data.begin(), data.end());
        }
    }

    auto IpcSocket::write(std::ranges::input_range auto&& range) -> uint64_t {
        return write(range.begin(), range.end());
    }

    template < class ValueType >
        requires std::is_same_v< std::decay_t< ValueType >, std::byte > or concepts::delimiter_compatible< ValueType >
    auto IpcSocket::readUntil(ValueType p_value) -> std::vector< std::byte > {
        if constexpr (constexpr auto value_size = sizeof(std::decay_t< ValueType >); value_size == 1) {
            if constexpr (std::is_same_v< ValueType, std::byte >) {
                return read_until(p_value);
            } else {
                return read_until(static_cast< std::byte >(p_value));
            }
        } else {
            const std::vector< std::byte > data{reinterpret_cast< std::byte * >(&p_value), reinterpret_cast< std::byte * >(&p_value) + value_size};
            return read_until(data.begin(), data.end());
        }
    }

    auto IpcSocket::readUntil(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> std::vector< std::byte >
        requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
              && (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >)
    {
        if constexpr (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte >) {
            return read_until(begin, end);
        } else {
            std::vector< std::byte > data;
            if constexpr (constexpr auto value_size = sizeof(std::decay_t< decltype(*begin) >); value_size == 1) {
                data.reserve(end - begin);
                std::transform(begin, end, data, []< typename ValueType >(ValueType&& value) -> std::byte {
                    return static_cast< std::byte >(value);
                });
            } else {
                auto size = (end - begin) * value_size;
                data.reserve(size);
                while (begin != end) {
                    std::copy_n(reinterpret_cast< std::byte >(&*begin), value_size, std::back_inserter(data));
                    ++begin;
                }
            }
            return read_until(data.begin(), data.end());
        }
    }

    auto IpcSocket::readUntil(std::ranges::input_range auto&& range) -> std::vector< std::byte > {
        return readUntil(range.begin(), range.end());
    }
}  // namespace mt::sockets
#endif  //IPC_SOCKET_HPP
