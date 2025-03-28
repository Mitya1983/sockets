#ifndef SOCKETS_INCLUDE_SOCKET_ERROR_HPP
#define SOCKETS_INCLUDE_SOCKET_ERROR_HPP

#include <cstdint>
#include <system_error>

namespace mt::sockets {

    enum class Error : uint8_t {
        /**
         * \brief Success
         */
        SUCCESS,
        /**
         * \brief The protocol type or the specified protocol is not supported within this communication domain
         */
        SOCKET_PROTOCOL_NOT_SUPPORTED,
        /**
         * \brief The per-process descriptor table is full
         */
        SOCKET_PROCESS_TABLE_IS_FULL,
        /**
         * \brief The system file table is full
         */
        SOCKET_SYSTEM_TABLE_IS_FULL,
        /**
         * \brief Permission to create a socket of the specified type and/or protocol is denied
         */
        SOCKET_NOT_ENOUGH_PERMISSIONS,
        /**
         * \brief Insufficient buffer space is available. The socket cannot be created until sufficient resources are freed
         */
        SOCKET_NOT_ENOUGH_MEMORY,
        /**
         * \brief The protocol is the wrong type for the socket
         */
        SOCKET_WRONG_PROTOCOL,
        /**
         * \brief Wrong ip format
         */
        SOCKET_WRONG_IP_FORMAT,
        /**
         * \brief Socket is not initialised
         */
        SOCKET_NOT_INITIALISED,
        /**
         * \brief Fcntl failed to change O_NONBLOCKING flag
         */
        SOCKET_FCNTL_ERROR,
        /**
         * \brief Failed to set SO_RCVTIMEO and SO_SNDTIMEO options
         */
        SOCKET_SET_TIMEOUT_ERROR,
        /**
         * \brief Socket is not connected
         */
        SOCKET_NOT_CONNECTED,
        /**
         * \brief Socket operation timed out
         */
        SOCKET_TIMED_OUT,
        /**
         * \brief The address is protected, and the user is not the superuser
         */
        BIND_NOT_ENOUGH_PERMISSIONS,
        /**
         * \brief The given address is already in use
         */
        BIND_ADDRESS_IN_USE,
        /**
         * \brief Socket is not a valid file descriptor
         */
        BIND_BAD_FILE_DESCRIPTOR,
        /**
         * \brief The socket is already bound to an address
         */
        BIND_ALREADY_BOUND,
        /**
         * \brief The file descriptor does not refer to a socket
         */
        BIND_FILE_DESCRIPTOR_IS_NOT_SOCKET,
        /**
         * \brief A nonexistent interface was requested or the requested address was not local
         */
        BIND_ADDRESS_NOT_AVAILABLE,
        /**
         * \brief Address points outside the user's accessible address space
         */
        BIND_ADDRESS_OUTSIDE_USER_SPACE,
        /**
         * \brief Too many symbolic links were encountered in resolving address
         */
        BIND_TO_MANY_SYMBOLIC_LINKS,
        /**
         * \brief Address is to long
         */
        BIND_NAME_TO_LONG,
        /**
         * \brief A component in the directory prefix of the socket pathname does not exists
         */
        BIND_NO_ENTRY,
        /**
         * \brief Insufficient kernel memory was available
         */
        BIND_NO_MEMORY,
        /**
         * \brief A component of the path prefix is not a directory
         */
        BIND_NOT_DIRECTORY,
        /**
         * \brief The socket inode would reside on a read-only filesystem
         */
        BIND_READ_ONLY_FS,
        /**
         * \brief Another socket is already listening on the same port
         */
        LISTEN_ADDRESS_IN_USE,
        /**
         * \brief Socket is not a valid file descriptor
         */
        LISTEN_BAD_FILE_DESCRIPTOR,
        /**
         * \brief The file descriptor does not refer to a socket
         */
        LISTEN_FILE_DESCRIPTOR_IS_NOT_SOCKET,
        /**
         * \brief The socket is not of a type that supports the listen() operation
         */
        LISTEN_PROTOCOL_NOT_SUPPORTED,
        /**
         * \brief The socket is is already connected
         */
        LISTEN_ALREADY_CONNECTED,
        /**
         * \brief The socket is not bind to any address
         */
        LISTEN_NOT_BOUND,
        /**
         * \brief The socket is not in listen mode
         */
        ACCEPT_SOCKET_IS_NOT_IN_LISTEN_MODE,
        /**
         * \brief The socket is already connected
         */
        ACCEPT_ALREADY_CONNECTED,
        /**
         * \brief The socket is not bind to any address
         */
        ACCEPT_NOT_BOUND,
        /**
         * \brief The socket is marked nonblocking and no connections are present to be accepted
         */
        ACCEPT_TRY_AGAIN,
        /**
         * \brief The socket is not an open file descriptor
         */
        ACCEPT_BAD_FILE_DESCRIPTOR,
        /**
         * \brief A connection has been aborted
         */
        ACCEPT_CONNECTION_ABORTED,
        /**
         * \brief The address argument is not in a writable part of the user address space
         */
        ACCEPT_ADDRESS_OUTSIDE_USER_SPACE,
        /**
         * \brief The system call was interrupted by a signal that was caught before a valid connection arrived
         */
        ACCEPT_INTERRUPTED,
        /**
         * \brief Socket is not listening for connections, or address length is invalid
         */
        ACCEPT_INVALID_VALUE,
        /**
         * \brief The per-process limit on the number of open file descriptors has been reached
         */
        ACCEPT_PER_PROCESS_LIMIT_REACHED,
        /**
         * \brief The system-wide limit on the total number of open files has been reached
         */
        ACCEPT_SYSTEM_WIDE_LIMIT_REACHED,
        /**
         * \brief Not enough free memory. This often means that the memory allocation is limited by the socket buffer limits, not by the system memory
         */
        ACCEPT_NOT_ENOUGH_MEMORY,
        /**
         * \brief The socket file descriptor does not refer to a socket
         */
        ACCEPT_FILE_DESCRIPTOR_IS_NOT_SOCKET,
        /**
         * \brief The referenced socket is not of type SOCK_STREAM
         */
        ACCEPT_OPTION_IS_NOT_SUPPORTED,
        /**
         * \brief Firewall rules forbid connection
         */
        ACCEPT_FIREWALL,
        /**
         * \brief Protocol error
         */
        ACCEPT_PROTOCOL_ERROR,
        /**
         * \brief The socket is is listen mode
         */
        CONNECT_SOCKET_IS_IN_LISTEN_MODE,
        /**
         * \brief Local address is already in use
         */
        CONNECT_NOT_ENOUGH_PERMISSIONS,
        /**
         * \brief For UNIX domain sockets, which are identified by pathname: Write permission is denied on the socket file, or search permission is denied for one of the directories in the path prefix. The user tried to connect to a broadcast address without having the socket broadcast flag enabled or the connection request failed because of a local firewall rule
         */
        CONNECT_ADDRESS_IN_USE,
        /**
         * \brief The socket referred to had not previously been bound to an address and, upon attempting to bind it to an ephemeral port
         */
        CONNECT_ADDRESS_NOT_AVAILABLE,
        /**
         * \brief The passed address didn't have the correct address family in its sa_family field
         */
        CONNECT_AF_NOT_SUPPORTED,
        /**
         * \brief For nonblocking UNIX domain sockets, the socket is nonblocking, and the connection cannot be completed immediately.  For other socket families, there are insufficient entries in the routing cache
         */
        CONNECT_TRY_AGAIN,
        /**
         * \brief The socket is nonblocking and a previous connection attempt has not yet been completed
         */
        CONNECT_ALREADY_IN_PROCESS,
        /**
         * \brief Socket is not a valid open file descriptor
         */
        CONNECT_BAD_FILE_DESCRIPTOR,
        /**
         * \brief On a stream socket found no one listening on the remote address
         */
        CONNECT_CONNECTION_REFUSED,
        /**
         * \brief The socket structure address is outside the user's address space
         */
        CONNECT_ADDRESS_OUTSIDE_USER_SPACE,
        /**
         * \brief The socket is nonblocking and the connection cannot be completed immediately
         */
        CONNECT_IN_PROGRESS,
        /**
         * \brief The system call was interrupted by a signal that was caught
         */
        CONNECT_INTERRUPTED,
        /**
         * \brief The socket is already connected
         */
        CONNECT_CONNECTED,
        /**
         * \brief Network is unreachable
         */
        CONNECT_NETWORK_UNREACHABLE,
        /**
         * \brief The file descriptor does not refer to a socket
         */
        CONNECT_FILE_DESCRIPTOR_IS_NOT_SOCKET,
        /**
         * \brief The protocol type or the specified protocol is not supported within this communication domain
         */
        CONNECT_PROTOCOL_NOT_SUPPORTED,
        /**
         * \brief TLS_client_method() returned nullptr
         */
        SSL_METHOD_ERROR,
        /**
         * \brief SSL_CTX_new returned nullptr
         */
        SSL_CONTEXT_ERROR,
        /**
         * \brief SSL_new returned nullptr
         */
        SSL_INIT_ERROR,
        /**
         * \brief Underlying BIO is nonblocking and operation should be performed once more
         */
        SSL_TRY_AGAIN,
        /**
         * \brief SSL_connect returned an error:
         */
        SSL_CONNECT_ERROR,
        /**
         * \brief SSL_get_peer_certificate returned nullptr
         */
        SSL_CERTIFICATE_ERROR,
        /**
         * \brief Host was not verified
         */
        SSL_CERTIFICATE_VERIFICATION_HOST,
        /**
         * \brief Certificate start date is in future
         */
        SSL_CERTIFICATE_VERIFICATION_START_DATE,
        /**
         * \brief Certificate end date is in the past
         */
        SSL_CERTIFICATE_VERIFICATION_END_DATE,
        /**
         * \brief Certificate had not passed the validation
         */
        SSL_CERTIFICATE_VALIDATION_FAILED,
        /**
         * \brief Connection was closed by host
         */
        SSL_CLOSED_BY_PEER,
        /**
         * \brief Some non-recoverable, fatal I/O error occurred
         */
        SSL_IO_ERROR,
        /**
         * \brief A non-recoverable, fatal error in the SSL library occurred, usually a protocol error
         */
        SSL_FATAL_ERROR,
        /**
         * \brief Unknown error
         */
        SSL_UNKNOWN_ERROR,
        /**
         * \brief Write permission is denied
         */
        WRITE_ACCESS,
        /**
         * \brief The socket is marked nonblocking and the requested operation would block
         */
        WRITE_TRY_AGAIN,
        /**
         * \brief Another Fast Open is in progress
         */
        WRITE_ALREADY,
        /**
         * \brief Socket is not a valid open file descriptor
         */
        WRITE_BAD_FILE_DESCRIPTOR,
        /**
         * \brief Connection reset by peer
         */
        WRITE_CONNECTION_RESET,
        /**
         * \brief The socket refers to a datagram socket for which a peer address has not been set using connect
         */
        WRITE_DESTINATION_ADDRESS,
        /**
         * \brief The user's quota of disk blocks on the filesystem containing the file referred to by fd has been exhausted
         */
        WRITE_USER_QUOTA,
        /**
         * \brief Buffer is outside your accessible address space
         */
        WRITE_BUFFER_OUT_OF_RANGE,
        /**
         * \brief A signal occurred before any data was transmitted
         */
        WRITE_INTERRUPTED,
        /**
         * \brief Invalid argument passed
         */
        WRITE_INVALID_ARGUMENT,
        /**
         * \brief The connection-mode socket was connected already but a recipient was specified
         */
        WRITE_IS_CONNECTED,
        /**
         * \brief The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible
         */
        WRITE_MESSAGE_SIZE,
        /**
         * \brief The output queue for a network interface was full
         */
        WRITE_NO_BUFFER,
        /**
         * \brief No memory available
         */
        WRITE_NO_MEMORY,
        /**
         * \brief The socket is not connected, and no target has been given
         */
        WRITE_NOT_CONNECTED,
        /**
         * \brief The file descriptor does not refer to a socket
         */
        WRITE_NOT_SOCKET,
        /**
         * \brief Some bit in the flags argument is inappropriate for the socket type
         */
        WRITE_NOT_SUPPORTED,
        /**
         * \brief The socket is connected to a pipe or socket whose reading end is closed
         */
        WRITE_PIPE,
        /**
         * \brief The file descriptor fd refers to a file other than a socket and has been marked nonblocking, and the read would block
         */
        READ_TRY_AGAIN,
        /**
         * \brief The socket is not a valid file descriptor or is not open for reading
         */
        READ_BAD_FILE_DESCRIPTOR,
        /**
         * \brief A remote host refused to allow the network connection
         */
        READ_CONNECTION_REFUSED,
        /**
         * \brief Buffer is outside your accessible address space
         */
        READ_BUFFER_OUT_OF_RANGE,
        /**
         * \brief A signal occurred before any data was read
         */
        READ_INTERRUPTED,
        /**
         * \brief The socket is attached to an which is unsuitable for reading
         */
        READ_INVALID_FILE_DESCRIPTOR,
        /**
         * \brief Could not allocate memory
         */
        READ_NO_MEMORY,
        /**
         * \brief The socket is associated with a connection-oriented protocol and has not been connected
         */
        READ_NOT_CONNECTED,
        /**
         * \brief The file descriptor does not refer to a socket
         */
        READ_NOT_SOCKET,
        /**
         * \brief EOF received
         */
        READ_EOF,
        /**
         * \brief Connection reset by peer
         */
        READ_CONNECTION_RESET,
        /**
         * \brief Insufficient resources were available in the system to perform the operation
         */
        READ_NO_BUFFER,
        /**
         * \brief Read until finished reading by reaching the delimiter
         */
        READ_DONE,
        /**
         * \brief The socket argument is not a valid file descriptor
         */
        SHUTDOWN_INVALID_SOCKET_ARGUMENT,
        /**
         * \brief The how argument is invalid
         */
        SHUTDOWN_INVALID_SHUTDOWN_OPTION_PROVIDED,
        /**
         * \brief The socket is not connected
         */
        SHUTDOWN_NOT_CONNECTED,
        /**
         * \brief The socket argument does not refer to a socket
         */
        SHUTDOWN_INVALID_FILE_DESCRIPTOR,
        /**
         * \brief Insufficient resources were available in the system to perform the operation
         */
        SHUTDOWN_NOT_ENOUGH_MEMORY
    };

    /**
     * \brief Creates std::error_code object that stores error information.
     * \param error_code SocketErrors
     * \return std::error_code
     */
    [[nodiscard]] auto makeError(Error error_code) -> std::error_code;

}  // namespace mt::sockets

#endif  //SOCKETS_INCLUDE_SOCKET_ERROR_HPP