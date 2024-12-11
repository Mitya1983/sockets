#ifndef IPC_SOCKET_HPP
#define IPC_SOCKET_HPP

#include "socket_common.hpp"

namespace mt::sockets {
    /**
     * \brief Class which is used to connect to local hosts
     */
    class IpcSocket {
    public:
        /**
         * \brief Constructor
         * \param p_socket_type SocketType. Default is set to SocketType::STREAM
         */
        explicit IpcSocket(SocketType p_socket_type = SocketType::STREAM);
        /**
         * \brief Deleted copy constructor
         */
        IpcSocket(const IpcSocket&) = delete;
        /**
         * \brief Deleted move constructor
         */
        IpcSocket(IpcSocket&&) = delete;
        /**
         * \brief Deleted copy assignment operator
         */
        IpcSocket& operator=(const IpcSocket&) = delete;
        /**
         * \brief Deleted move assignment operator
         */
        IpcSocket& operator=(IpcSocket&&) = delete;
        /**
         * \brief Destructor
         */
        ~IpcSocket();

        /**
         * \brief Sets name of the socket
         * \param p_name const std::string&
         * \param p_global_namespace bool
         */
        void setName(const std::string& p_name, bool p_global_namespace);
        /**
         * \brief Sets peer sockets name
         * \param p_name const std::string&
         * \param p_global_namespace bool
         */
        void setPeerName(const std::string& p_name, bool p_global_namespace);
        /**
         * \brief Sets socket as non blocking
         * \param p_non_blocking bool. Default is true
         */
        void setNonBlocking(bool p_non_blocking = true);
        /**
         * \brief Resets error to tristan::socket::Error::SUCCESS
         */
        void resetError();
        /**
         * \brief Binds socket with name.
         * Mandatory function for server side socket
         */
        void bind();
        /**
         * \brief Sets socket to be in listen mode
         * \param p_connection_count_limit uint32_t
         */
        void listen(uint32_t p_connection_count_limit);
        /**
         * \brief Connects socket to peer
         * \param p_ssl bool
         */
        void connect();
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
        [[nodiscard]] auto accept() -> std::optional< std::unique_ptr< IpcSocket > >;
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
            return IpcSocket::write(l_temp_data);
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
         * \brief Returns name of the socket
         * \return const std::string&
         */
        [[nodiscard]] auto name() const noexcept -> const std::string&;
        /**
         * \brief Returns name of the peer socket
         * \return const std::string&
         */
        [[nodiscard]] auto peerName() const noexcept -> const std::string&;
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

    protected:
    private:
        explicit IpcSocket(bool);

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
}  // namespace tristan::sockets
#endif  //IPC_SOCKET_HPP
