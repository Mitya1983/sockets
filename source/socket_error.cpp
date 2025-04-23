#include "include/socket_error.hpp"
#include <map>

struct SocketErrorCategory : std::error_category {
    [[nodiscard]] const char *name() const noexcept override;

    [[nodiscard]] std::string message(int ec) const override;
};

inline const SocketErrorCategory g_socket_error_category;

inline const std::map< mt::sockets::Error, const char * > g_socket_code_descriptions{
    {mt::sockets::Error::SUCCESS,                                   "Success"                                                                                                                },
    {mt::sockets::Error::SOCKET_PROTOCOL_NOT_SUPPORTED,             "The protocol type or the specified protocol is not supported within this communication domain"                          },
    {mt::sockets::Error::SOCKET_PROCESS_TABLE_IS_FULL,              "The per-process descriptor table is full"                                                                               },
    {mt::sockets::Error::SOCKET_SYSTEM_TABLE_IS_FULL,               "The system file table is full"                                                                                          },
    {mt::sockets::Error::SOCKET_NOT_ENOUGH_PERMISSIONS,             "Permission to create a socket of the specified type and/or protocol is denied"                                          },
    {mt::sockets::Error::SOCKET_NOT_ENOUGH_MEMORY,                  "Insufficient buffer space is available. The socket cannot be created until sufficient resources are freed"              },
    {mt::sockets::Error::SOCKET_WRONG_PROTOCOL,                     "The protocol is the wrong type for the socket"                                                                          },
    {mt::sockets::Error::SOCKET_WRONG_IP_FORMAT,                    "Wrong ip format"                                                                                                        },
    {mt::sockets::Error::SOCKET_NOT_INITIALISED,                    "Socket is not initialised"                                                                                              },
    {mt::sockets::Error::SOCKET_FCNTL_ERROR,                        "Fcntl failed to change O_NONBLOCKING flag"                                                                              },
    {mt::sockets::Error::SOCKET_SET_TIMEOUT_ERROR,                  "Failed to set SO_RCVTIMEO and SO_SNDTIMEO options"                                                                      },
    {mt::sockets::Error::SOCKET_NOT_CONNECTED,                      "Socket is not connected"                                                                                                },
    {mt::sockets::Error::SOCKET_TIMED_OUT,                          "Socket operation timed out"                                                                                             },
    {mt::sockets::Error::BIND_NOT_ENOUGH_PERMISSIONS,               "The address is protected, and the user is not the superuser"                                                            },
    {mt::sockets::Error::BIND_ADDRESS_IN_USE,                       "The given address is already in use"                                                                                    },
    {mt::sockets::Error::BIND_BAD_FILE_DESCRIPTOR,                  "Socket is not a valid file descriptor"                                                                                  },
    {mt::sockets::Error::BIND_ALREADY_BOUND,                        "The socket is already bound to an address"                                                                              },
    {mt::sockets::Error::BIND_FILE_DESCRIPTOR_IS_NOT_SOCKET,        "The file descriptor does not refer to a socket"                                                                         },
    {mt::sockets::Error::BIND_ADDRESS_NOT_AVAILABLE,                "A nonexistent interface was requested or the requested address was not local"                                           },
    {mt::sockets::Error::BIND_ADDRESS_OUTSIDE_USER_SPACE,           "Address points outside the user's accessible address space"                                                             },
    {mt::sockets::Error::BIND_TO_MANY_SYMBOLIC_LINKS,               "Too many symbolic links were encountered in resolving address"                                                          },
    {mt::sockets::Error::BIND_NAME_TO_LONG,                         "Address is to long"                                                                                                     },
    {mt::sockets::Error::BIND_NO_ENTRY,                             "A component in the directory prefix of the socket pathname does not exists"                                             },
    {mt::sockets::Error::BIND_NO_MEMORY,                            "Insufficient kernel memory was available"                                                                               },
    {mt::sockets::Error::BIND_NOT_DIRECTORY,                        "A component of the path prefix is not a directory"                                                                      },
    {mt::sockets::Error::BIND_READ_ONLY_FS,                         "The socket inode would reside on a read-only filesystem"                                                                },
    {mt::sockets::Error::LISTEN_ADDRESS_IN_USE,                     "Another socket is already listening on the same port"                                                                   },
    {mt::sockets::Error::LISTEN_BAD_FILE_DESCRIPTOR,                "Socket is not a valid file descriptor"                                                                                  },
    {mt::sockets::Error::LISTEN_FILE_DESCRIPTOR_IS_NOT_SOCKET,      "The file descriptor does not refer to a socket"                                                                         },
    {mt::sockets::Error::LISTEN_PROTOCOL_NOT_SUPPORTED,             "The socket is not of a type that supports the listen() operation"                                                       },
    {mt::sockets::Error::LISTEN_ALREADY_CONNECTED,                  "The socket is is already connected"                                                                                     },
    {mt::sockets::Error::LISTEN_NOT_BOUND,                          "The socket is not bind to any address"                                                                                  },
    {mt::sockets::Error::ACCEPT_SOCKET_IS_NOT_IN_LISTEN_MODE,       "The socket is not in listen mode"                                                                                       },
    {mt::sockets::Error::ACCEPT_ALREADY_CONNECTED,                  "The socket is already connected"                                                                                        },
    {mt::sockets::Error::ACCEPT_NOT_BOUND,                          "The socket is not bind to any address"                                                                                  },
    {mt::sockets::Error::ACCEPT_TRY_AGAIN,                          "The socket is marked nonblocking and no connections are present to be accepted"                                         },
    {mt::sockets::Error::ACCEPT_BAD_FILE_DESCRIPTOR,                "The socket is not an open file descriptor"                                                                              },
    {mt::sockets::Error::ACCEPT_CONNECTION_ABORTED,                 "A connection has been aborted"                                                                                          },
    {mt::sockets::Error::ACCEPT_ADDRESS_OUTSIDE_USER_SPACE,         "The address argument is not in a writable part of the user address space"                                               },
    {mt::sockets::Error::ACCEPT_INTERRUPTED,                        "The system call was interrupted by a signal that was caught before a valid connection arrived"                          },
    {mt::sockets::Error::ACCEPT_INVALID_VALUE,                      "Socket is not listening for connections, or address length is invalid"                                                  },
    {mt::sockets::Error::ACCEPT_PER_PROCESS_LIMIT_REACHED,          "The per-process limit on the number of open file descriptors has been reached"                                          },
    {mt::sockets::Error::ACCEPT_SYSTEM_WIDE_LIMIT_REACHED,          "The system-wide limit on the total number of open files has been reached"                                               },
    {mt::sockets::Error::ACCEPT_NOT_ENOUGH_MEMORY,
     "Not enough free memory. This often means that the memory allocation is limited by the socket buffer limits, not by the system memory"                                                  },
    {mt::sockets::Error::ACCEPT_FILE_DESCRIPTOR_IS_NOT_SOCKET,      "The socket file descriptor does not refer to a socket"                                                                  },
    {mt::sockets::Error::ACCEPT_OPTION_IS_NOT_SUPPORTED,            "The referenced socket is not of type SOCK_STREAM"                                                                       },
    {mt::sockets::Error::ACCEPT_FIREWALL,                           "Firewall rules forbid connection"                                                                                       },
    {mt::sockets::Error::ACCEPT_PROTOCOL_ERROR,                     "Protocol error"                                                                                                         },
    {mt::sockets::Error::CONNECT_SOCKET_IS_IN_LISTEN_MODE,          "The socket is is listen mode"                                                                                           },
    {mt::sockets::Error::CONNECT_NOT_ENOUGH_PERMISSIONS,            "Local address is already in use"                                                                                        },
    {mt::sockets::Error::CONNECT_ADDRESS_IN_USE,
     "For UNIX domain sockets, which are identified by pathname: Write permission is denied on the socket file, or search permission "
     "is denied for one of the directories in the path prefix. The user tried to connect to a broadcast address without having the "
     "socket broadcast flag enabled or the connection request failed because of a local firewall rule"                                                                                       },
    {mt::sockets::Error::CONNECT_ADDRESS_NOT_AVAILABLE,             "The socket referred to had not previously been bound to an address and, upon attempting to bind it to an ephemeral port"},
    {mt::sockets::Error::CONNECT_AF_NOT_SUPPORTED,                  "The passed address didn't have the correct address family in its sa_family field"                                       },
    {mt::sockets::Error::CONNECT_TRY_AGAIN,
     "For nonblocking UNIX domain sockets, the socket is nonblocking, and the connection cannot be completed immediately.  For other "
     "socket families, there are insufficient entries in the routing cache"                                                                                                                  },
    {mt::sockets::Error::CONNECT_ALREADY_IN_PROCESS,                "The socket is nonblocking and a previous connection attempt has not yet been completed"                                 },
    {mt::sockets::Error::CONNECT_BAD_FILE_DESCRIPTOR,               "Socket is not a valid open file descriptor"                                                                             },
    {mt::sockets::Error::CONNECT_CONNECTION_REFUSED,                "On a stream socket found no one listening on the remote address"                                                        },
    {mt::sockets::Error::CONNECT_ADDRESS_OUTSIDE_USER_SPACE,        "The socket structure address is outside the user's address space"                                                       },
    {mt::sockets::Error::CONNECT_IN_PROGRESS,                       "The socket is nonblocking and the connection cannot be completed immediately"                                           },
    {mt::sockets::Error::CONNECT_INTERRUPTED,                       "The system call was interrupted by a signal that was caught"                                                            },
    {mt::sockets::Error::CONNECT_CONNECTED,                         "The socket is already connected"                                                                                        },
    {mt::sockets::Error::CONNECT_NETWORK_UNREACHABLE,               "Network is unreachable"                                                                                                 },
    {mt::sockets::Error::CONNECT_FILE_DESCRIPTOR_IS_NOT_SOCKET,     "The file descriptor does not refer to a socket"                                                                         },
    {mt::sockets::Error::CONNECT_PROTOCOL_NOT_SUPPORTED,            "The protocol type or the specified protocol is not supported within this communication domain"                          },
    {mt::sockets::Error::SSL_METHOD_ERROR,                          "TLS_client_method() returned nullptr"                                                                                   },
    {mt::sockets::Error::SSL_CONTEXT_ERROR,                         "SSL_CTX_new returned nullptr"                                                                                           },
    {mt::sockets::Error::SSL_INIT_ERROR,                            "SSL_new returned nullptr"                                                                                               },
    {mt::sockets::Error::SSL_TRY_AGAIN,                             "Underlying BIO is nonblocking and operation should be performed once more"                                              },
    {mt::sockets::Error::SSL_CONNECT_ERROR,                         "SSL_connect returned an error: "                                                                                        },
    {mt::sockets::Error::SSL_CERTIFICATE_ERROR,                     "SSL_get_peer_certificate returned nullptr"                                                                              },
    {mt::sockets::Error::SSL_CERTIFICATE_VERIFICATION_HOST,         "Host was not verified"                                                                                                  },
    {mt::sockets::Error::SSL_CERTIFICATE_VERIFICATION_START_DATE,   "Certificate start date is in future"                                                                                    },
    {mt::sockets::Error::SSL_CERTIFICATE_VERIFICATION_END_DATE,     "Certificate end date is in the past"                                                                                    },
    {mt::sockets::Error::SSL_CERTIFICATE_VALIDATION_FAILED,         "Certificate had not passed the validation"                                                                              },
    {mt::sockets::Error::SSL_CLOSED_BY_PEER,                        "Connection was closed by host"                                                                                          },
    {mt::sockets::Error::SSL_IO_ERROR,                              "Some non-recoverable, fatal I/O error occurred"                                                                         },
    {mt::sockets::Error::SSL_FATAL_ERROR,                           "A non-recoverable, fatal error in the SSL library occurred, usually a protocol error"                                   },
    {mt::sockets::Error::SSL_UNKNOWN_ERROR,                         "Unknown error"                                                                                                          },
    {mt::sockets::Error::WRITE_ACCESS,                              "Write permission is denied"                                                                                             },
    {mt::sockets::Error::WRITE_TRY_AGAIN,                           "The socket is marked nonblocking and the requested operation would block"                                               },
    {mt::sockets::Error::WRITE_ALREADY,                             "Another Fast Open is in progress"                                                                                       },
    {mt::sockets::Error::WRITE_BAD_FILE_DESCRIPTOR,                 "Socket is not a valid open file descriptor"                                                                             },
    {mt::sockets::Error::WRITE_CONNECTION_RESET,                    "Connection reset by peer"                                                                                               },
    {mt::sockets::Error::WRITE_DESTINATION_ADDRESS,                 "The socket refers to a datagram socket for which a peer address has not been set using connect"                         },
    {mt::sockets::Error::WRITE_USER_QUOTA,                          "The user's quota of disk blocks on the filesystem containing the file referred to by fd has been exhausted"             },
    {mt::sockets::Error::WRITE_BUFFER_OUT_OF_RANGE,                 "Buffer is outside your accessible address space"                                                                        },
    {mt::sockets::Error::WRITE_INTERRUPTED,                         "A signal occurred before any data was transmitted"                                                                      },
    {mt::sockets::Error::WRITE_INVALID_ARGUMENT,                    "Invalid argument passed"                                                                                                },
    {mt::sockets::Error::WRITE_IS_CONNECTED,                        "The connection-mode socket was connected already but a recipient was specified"                                         },
    {mt::sockets::Error::WRITE_MESSAGE_SIZE,                        "The socket type requires that message be sent atomically, and the size of the message to be sent made this impossible"  },
    {mt::sockets::Error::WRITE_NO_BUFFER,                           "The output queue for a network interface was full"                                                                      },
    {mt::sockets::Error::WRITE_NO_MEMORY,                           "No memory available"                                                                                                    },
    {mt::sockets::Error::WRITE_NOT_CONNECTED,                       "The socket is not connected, and no target has been given"                                                              },
    {mt::sockets::Error::WRITE_NOT_SOCKET,                          "The file descriptor does not refer to a socket"                                                                         },
    {mt::sockets::Error::WRITE_NOT_SUPPORTED,                       "Some bit in the flags argument is inappropriate for the socket type"                                                    },
    {mt::sockets::Error::WRITE_PIPE,                                "The socket is connected to a pipe or socket whose reading end is closed"                                                },
    {mt::sockets::Error::READ_TRY_AGAIN,                            "The file descriptor fd refers to a file other than a socket and has been marked nonblocking, and the read would block"  },
    {mt::sockets::Error::READ_BAD_FILE_DESCRIPTOR,                  "The socket is not a valid file descriptor or is not open for reading"                                                   },
    {mt::sockets::Error::READ_CONNECTION_REFUSED,                   "A remote host refused to allow the network connection"                                                                  },
    {mt::sockets::Error::READ_BUFFER_OUT_OF_RANGE,                  "Buffer is outside your accessible address space"                                                                        },
    {mt::sockets::Error::READ_INTERRUPTED,                          "A signal occurred before any data was read"                                                                             },
    {mt::sockets::Error::READ_INVALID_FILE_DESCRIPTOR,              "The socket is attached to an which is unsuitable for reading"                                                           },
    {mt::sockets::Error::READ_NO_MEMORY,                            "Could not allocate memory"                                                                                              },
    {mt::sockets::Error::READ_NOT_CONNECTED,                        "The socket is associated with a connection-oriented protocol and has not been connected"                                },
    {mt::sockets::Error::READ_NOT_SOCKET,                           "The file descriptor does not refer to a socket"                                                                         },
    {mt::sockets::Error::READ_EOF,                                  "EOF received"                                                                                                           },
    {mt::sockets::Error::READ_CONNECTION_RESET,                     "Connection reset by peer"                                                                                               },
    {mt::sockets::Error::READ_NO_BUFFER,                            "Insufficient resources were available in the system to perform the operation"                                                                                               },
    {mt::sockets::Error::READ_DONE,                                 "Read until finished reading by reaching the delimiter"                                                                  },
    {mt::sockets::Error::SHUTDOWN_INVALID_SOCKET_ARGUMENT,          "The socket argument is not a valid file descriptor"                                                                     },
    {mt::sockets::Error::SHUTDOWN_INVALID_SHUTDOWN_OPTION_PROVIDED, "The how argument is invalid"                                                                                            },
    {mt::sockets::Error::SHUTDOWN_NOT_CONNECTED,                    "The socket is not connected"                                                                                            },
    {mt::sockets::Error::SHUTDOWN_INVALID_FILE_DESCRIPTOR,          "The socket argument does not refer to a socket"                                                                         },
    {mt::sockets::Error::SHUTDOWN_NOT_ENOUGH_MEMORY,                "Insufficient resources were available in the system to perform the operation"                                           },
};

auto mt::sockets::makeError(mt::sockets::Error error_code) -> std::error_code {
    return {static_cast< int >(error_code), g_socket_error_category};
}

auto SocketErrorCategory::name() const noexcept -> const char * {
    return "SocketCategory";
}

auto SocketErrorCategory::message(int ec) const -> std::string {
    return {g_socket_code_descriptions.at(static_cast< mt::sockets::Error >(ec))};
}

auto operator==(mt::sockets::Error p_left, int32_t p_right) -> bool {
    return static_cast<int32_t>(p_left) == p_right;
}

auto operator==(int32_t p_left, mt::sockets::Error p_right) -> bool {
    return p_left == static_cast<int32_t>(p_right);
}