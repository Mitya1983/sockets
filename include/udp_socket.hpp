// Created by Mitia Tristan on 29.01.25.
#ifndef INCLUDE_UDP_SOCKET_HPP
#define INCLUDE_UDP_SOCKET_HPP

#include "socket_common.hpp"

#include <string>
#include <system_error>
#include <optional>
#include <vector>
#include <algorithm>
#include <chrono>

namespace mt::sockets {

    class UdpSocket {
      public:
        explicit UdpSocket();
        UdpSocket(const UdpSocket&) = delete;
        UdpSocket(UdpSocket&&) = delete;
        UdpSocket& operator=(const UdpSocket&) = delete;
        UdpSocket& operator=(UdpSocket&&) = delete;
        ~UdpSocket();

        void setHost(uint32_t p_ip, std::string p_host_name = "");
        void setPort(uint16_t p_port);
        void setNonBlocking(bool p_non_blocking = true);
        void setTimeOut(std::chrono::seconds p_seconds);
        void resetError();
        void bind();
        void close() const;
        void shutdown();
        auto write(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> uint64_t
            requires std::is_same_v< std::decay_t< decltype(*begin) >, std::decay_t< decltype(*end) > >
                 and (std::is_same_v< std::decay_t< decltype(*begin) >, std::byte > or concepts::write_compatible< std::decay_t< decltype(*begin) > >);
        auto write(std::ranges::input_range auto&& range) -> uint64_t;
        [[nodiscard]] auto read() -> std::vector<std::byte>;
        [[nodiscard]] auto ip() const noexcept -> uint32_t;
        [[nodiscard]] auto port() const noexcept -> uint16_t;
        [[nodiscard]] auto error() const noexcept -> std::error_code;
        [[nodiscard]] auto nonBlocking() const noexcept -> bool;

      private:
        auto write(const std::byte *p_bytes, uint64_t p_size) -> uint64_t;

        std::string m_host_name;

        int32_t m_socket{-1};
        uint32_t m_ip{0};

        std::error_code m_error;

        uint16_t m_port{0};

        bool m_non_blocking{false};
        bool m_bound{false};
        bool m_listening{false};
    };
    auto UdpSocket::write(std::indirectly_readable auto begin, std::indirectly_readable auto end) -> uint64_t
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
                    bytes_written += write(reinterpret_cast< std::byte* >(&*begin), value_size);
                    ++begin;
                }
                return bytes_written;
            }
        }
    }

    auto UdpSocket::write(std::ranges::input_range auto&& range) -> uint64_t {
        return write(range.begin(), range.end());
    }
}  // namespace mt::sockets

#endif //INCLUDE_UDP_SOCKET_HPP
