#ifndef INET_SOCKET_HPP
#define INET_SOCKET_HPP

#include "socket_common.hpp"

#include <chrono>

namespace mt::sockets {

    class Ssl;

    /**
     * \brief CLass which is used to connect to remote hosts
     */
    class InetSocket {
    public:
        /**
         * \brief Constructor
         * \param p_socket_type SocketType. Default is set to SocketType::STREAM
         */
        explicit InetSocket(SocketType p_socket_type = SocketType::STREAM);
        /**
         * \brief Deleted copy constructor
         */
        InetSocket(const InetSocket&) = delete;
        /**
         * \brief Deleted move constructor
         */
        InetSocket(InetSocket&&) = delete;
        /**
         * \brief Deleted copy assignment operator
         */
        InetSocket& operator=(const InetSocket&) = delete;
        /**
         * \brief Deleted move assignment operator
         */
        InetSocket& operator=(InetSocket&&) = delete;
        /**
         * \brief Destructor
         */
        ~InetSocket();

        /**
         * \brief Sets IP
         * If socket is dedicated to be a server - ip will be considered as local, if socket is dedicated to be a client - ip will be considered as remote
         * \param p_ip uint32_t - IP in network byte order
         * \param p_host_name const std::string& - optional parameter to store human readable host name
         */
        void setHost(uint32_t p_ip, const std::string& p_host_name = "");
        /**
         * \brief Sets port
         * If socket is dedicated to be a server - port will be considered as local, if socket is dedicated to be a client - port will be considered as remote
         * \param p_port uint16_t - port in network byte order
         */
        void setPort(uint16_t p_port);
        /**
         * \brief Sets socket as non blocking
         * \param p_non_blocking bool. Default is true
         */
        void setNonBlocking(bool p_non_blocking = true);
        /**
         * \brief Sets send and receive timeout.
         * If socket is in non blocking state does nothing
         * \param p_seconds std::chrono::seconds
         */
        void setTimeOut(std::chrono::seconds p_seconds);
        /**
         * \brief Resets error to tristan::socket::Error::SUCCESS
         */
        void resetError();
        /**
         * \brief Binds socket with ip and port.
         * Mandatory function for server side socket
         */
        void bind();
        /**
         * \brief Sets socket to be in listen mode
         * \param p_connection_count_limit uint32_t
         */
        void listen(uint32_t p_connection_count_limit);
        /**
         * \brief Connects socket to remote ip and port
         * \param p_ssl bool
         */
        void connect(bool p_ssl = true);
        /**
         * \brief Closes the socket
         */
        void close();
        /**
         * \brief Shutdowns the socket
         */
        void shutdown();
        /**
         * \brief Starts accepting incoming connections
         * \return std::optional<std::unique_ptr<InetSocket>>
         * If error occurred the std::nullopt is returned and error is set respectively
         */
        [[nodiscard]] auto accept() -> std::optional<std::unique_ptr<InetSocket>>;
        /**
         * \brief Write one byte of data
         * \param p_byte uint8_t
         * \return uint8_t. On error 0 is returned and error is set respectively.
         */
        auto write(uint8_t p_byte) -> uint8_t;
        /**
         * \overload
         * \brief Write data to socket
         * \param p_data const std::vector< uint8_t >&
         * \param p_size uint16_t size of data to send
         * \param p_offset position of first byte
         * \return uint64_t indicating number of data sent or 0 if data is empty or error occurred
         */
        auto write(const std::vector< uint8_t >& p_data, uint16_t p_size = 0, uint64_t p_offset = 0) -> uint64_t;
        /**
         * \overload
         * \brief Writes object casted to std::vector< uint8_t > to socket
         * \tparam ObjectClassToSend Class which should meat requirement of std::is_standard_layout_v
         * \param p_object ObjectClassToSend
         * \return uint64_t indicating number of data sent or 0 if data is empty or error occurred
         */
        template < class ObjectClassToSend >
        auto write(ObjectClassToSend p_object) -> uint64_t
            requires std::is_standard_layout_v< ObjectClassToSend >
        {
            std::vector< uint8_t > l_temp_data(reinterpret_cast< uint8_t* >(&p_object), reinterpret_cast< uint8_t* >(&p_object) + sizeof(p_object));
            return InetSocket::write(l_temp_data);
        }
        /**
         * \brief Reads one byte from socket
         * \return uint8_t.
         * This function may return 0 on error or on EOF
         */
        [[nodiscard]] auto read() -> uint8_t;
        /**
         * \overload
         * \brief Reads provided size of data from socket
         * \param p_size
         * \return std::vector< uint8_t >
         */
        [[nodiscard]] auto read(uint16_t p_size) -> std::vector< uint8_t >;
        /**
         * \brief reads from socket until the delimiter is reached
         * \param p_delimiter uint8_t
         * \return std::vector< uint8_t >
         */
        [[nodiscard]] auto readUntil(uint8_t p_delimiter) -> std::vector< uint8_t >;
        /**
         * \overload
         * \brief reads from socket until the delimiter is reached
         * \param p_delimiter const std::vector< uint8_t >&
         * \return std::vector< uint8_t >
         */
        [[nodiscard]] auto readUntil(const std::vector< uint8_t >& p_delimiter) -> std::vector< uint8_t >;

        /**
         * \brief Returns ip
         * \return uint32_t
         */
        [[nodiscard]] auto ip() const noexcept -> uint32_t;
        /**
         * \brief Returns port
         * \return uint16_t
         */
        [[nodiscard]] auto port() const noexcept -> uint16_t;
        /**
         * \brief Returns error
         * \return std::error_code
         */
        [[nodiscard]] auto error() const noexcept -> std::error_code;
        /**
         * \brief Returns socket non blocking state
         * \return bool
         */
        [[nodiscard]] auto nonBlocking() const noexcept -> bool;
        /**
         * \brief Returns socket connection state
         * \return bool
         */
        [[nodiscard]] auto connected() const noexcept -> bool;

    private:

        explicit InetSocket(bool);

        std::string m_host_name;

        int32_t m_socket;
        uint32_t m_ip;

        std::error_code m_error;

        uint16_t m_port;

        std::unique_ptr<Ssl> m_ssl;
        SocketType m_type;


        bool m_non_blocking;
        bool m_bound;
        bool m_listening;
        bool m_not_ssl_connected;
        bool m_connected;
    };

}  // namespace tristan::sockets

#endif  //INET_SOCKET_HPP
