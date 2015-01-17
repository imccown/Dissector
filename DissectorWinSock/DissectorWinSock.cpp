#include "DissectorWinSock.h"
#include "..\Dissector\Dissector.h"
#include <assert.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const unsigned int skBufferSize = 4096;
struct DissectorWinsockData
{
    WSADATA         mWSAData;
    unsigned int    mPort;
    SOCKET          mListenSocket;
    SOCKET          mConnectionSocket;
    char            mMessageBuffer[skBufferSize];
    unsigned int    mMessageBufferIter;
};

DissectorWinsockData sDissectorWinsockData;

namespace DissectorWinsock
{
    bool CreateListenSocket()
    {
        struct addrinfo *result = NULL, *ptr = NULL, hints;
        char portNumber[32];
        _snprintf_s( portNumber, sizeof(portNumber), "%u", sDissectorWinsockData.mPort );
        portNumber[31] = 0;

        ZeroMemory(&hints, sizeof (hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        // Resolve the local address and port to be used by the server
        int res = getaddrinfo(NULL, portNumber, &hints, &result);
        assert( res == 0 );
        if (res != 0)
        {
            WSACleanup();
            return false;
        }

        // Create a SOCKET for the server to listen for client connections
        sDissectorWinsockData.mListenSocket = INVALID_SOCKET;
        sDissectorWinsockData.mConnectionSocket = INVALID_SOCKET;

        sDissectorWinsockData.mListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sDissectorWinsockData.mListenSocket == INVALID_SOCKET)
        {
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }

        unsigned long mode = 1; //1 for non-blocking
        res = ioctlsocket(sDissectorWinsockData.mListenSocket, FIONBIO, &mode);
        if( res == SOCKET_ERROR )
        {
            freeaddrinfo(result);
            closesocket(sDissectorWinsockData.mListenSocket);
            WSACleanup();
            return false;
        }

        // Setup the TCP listening socket
        res = bind( sDissectorWinsockData.mListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (res == SOCKET_ERROR)
        {
            freeaddrinfo(result);
            closesocket(sDissectorWinsockData.mListenSocket);
            WSACleanup();
            return false;
        }

        freeaddrinfo(result);

        if ( listen( sDissectorWinsockData.mListenSocket, SOMAXCONN ) == SOCKET_ERROR )
        {
            closesocket(sDissectorWinsockData.mListenSocket);
            WSACleanup();
            return false;
        }

        return true;
    }

    void CheckForConnection()
    {
        if (sDissectorWinsockData.mConnectionSocket == INVALID_SOCKET)
        {
            // Accept a client socket
            sDissectorWinsockData.mConnectionSocket = accept(sDissectorWinsockData.mListenSocket, NULL, NULL);
            if (sDissectorWinsockData.mConnectionSocket == INVALID_SOCKET)
            {
                int error = WSAGetLastError();
                if( error != WSAEWOULDBLOCK )
                {
                    closesocket(sDissectorWinsockData.mListenSocket);
                }
            }

            unsigned long mode = 1; // 1 for non-blocking
            int res = ioctlsocket(sDissectorWinsockData.mConnectionSocket, FIONBIO, &mode);
        }
    }

    char* FindValue( unsigned int iValue, char* iIter, char* iIterEnd )
    {
        char* valueIter = (char*)&iValue;
        char* valueIterEnd = valueIter + sizeof(unsigned int);

        while( iIter != iIterEnd )
        {
            if( *iIter == *valueIter )
            {
                ++valueIter;
                if( valueIter >= valueIterEnd )
                {
                    ++iIter;
                    return iIter;
                }
            }
            else
            {
                valueIter = (char*)&iValue;
            }
            ++iIter;
        }

        return NULL;
    }

    void SendMessage( const char* iBuffer, unsigned int iDataSize )
    {
        if ( sDissectorWinsockData.mConnectionSocket != INVALID_SOCKET )
        {
            const char* dataIter = iBuffer;
            int dataSize = iDataSize;
            while( dataSize )
            {
                int sent = send( sDissectorWinsockData.mConnectionSocket, dataIter, dataSize, 0 );
                if( sent != -1 )
                {
                    dataIter += sent;
                    dataSize -= sent;
                }
                else
                {
                    int a = 1;
                    a;
                }
            }
        }
    }

    void CheckForMessages()
    {
        if ( sDissectorWinsockData.mConnectionSocket != INVALID_SOCKET )
        {
            int rvalue = recv( sDissectorWinsockData.mConnectionSocket,
                sDissectorWinsockData.mMessageBuffer + sDissectorWinsockData.mMessageBufferIter,
                skBufferSize - sDissectorWinsockData.mMessageBufferIter,
                0 );
             
            if( rvalue == 0 )
            {
                // Connection has been closed
                closesocket( sDissectorWinsockData.mConnectionSocket );
                sDissectorWinsockData.mConnectionSocket = INVALID_SOCKET;
            }
            else if( rvalue == SOCKET_ERROR )
            {
                int error = WSAGetLastError();
                if( error != WSAEWOULDBLOCK )
                {
                    closesocket( sDissectorWinsockData.mConnectionSocket );
                    sDissectorWinsockData.mConnectionSocket = INVALID_SOCKET;
                }
            }
            else
            {
                sDissectorWinsockData.mMessageBufferIter += rvalue;
            }

            if( sDissectorWinsockData.mMessageBufferIter != 0 )
            {
                char* msgBegin = FindValue( Dissector::PACKET_BEGIN, sDissectorWinsockData.mMessageBuffer,
                    sDissectorWinsockData.mMessageBuffer + sDissectorWinsockData.mMessageBufferIter );

                if( msgBegin )
                {   
                    unsigned int messageSize = *(int*)msgBegin;
                    char* msgIter = msgBegin+sizeof(unsigned int);
                    assert( (messageSize + msgIter) < (sDissectorWinsockData.mMessageBuffer + skBufferSize) );

                    if( messageSize + msgIter <= (sDissectorWinsockData.mMessageBuffer + 
                        sDissectorWinsockData.mMessageBufferIter) )
                    {
                        // Got a real message! Dispatch it to the Dissector.
                        Dissector::ReceiveMessage( msgIter, messageSize );

                        // Remove the message from the buffer while saving the rest
                        sDissectorWinsockData.mMessageBufferIter -= messageSize + sizeof(unsigned int)*2;
                        sDissectorWinsockData.mMessageBufferIter = max( sDissectorWinsockData.mMessageBufferIter, 0 );
                        memmove( sDissectorWinsockData.mMessageBuffer, messageSize + msgIter,
                            sDissectorWinsockData.mMessageBufferIter );
                    }
                }
                else if( sDissectorWinsockData.mMessageBufferIter == skBufferSize )
                {
                    // no header and the buffer is full. Erase everything but the last 3 bytes
                    // which might be part of the header
                    sDissectorWinsockData.mMessageBuffer[0] = sDissectorWinsockData.mMessageBuffer[skBufferSize-3];
                    sDissectorWinsockData.mMessageBuffer[1] = sDissectorWinsockData.mMessageBuffer[skBufferSize-2];
                    sDissectorWinsockData.mMessageBuffer[2] = sDissectorWinsockData.mMessageBuffer[skBufferSize-1];
                    sDissectorWinsockData.mMessageBufferIter = 3;
                }
            }
        }
    }

    void FrameUpdate()
    {
        CheckForConnection();
        CheckForMessages();
    }

    void Initialize( unsigned int iListenPort )
    {
        int res;
        sDissectorWinsockData.mMessageBufferIter = 0;

        // Initialize Winsock
        res = WSAStartup(MAKEWORD(2,2), &sDissectorWinsockData.mWSAData);
        assert( res == 0 );

        sDissectorWinsockData.mPort = iListenPort;
        CreateListenSocket();
        Dissector::SetNetworkCallback( FrameUpdate );
        Dissector::SetSendMessageCallback( SendMessage );
    }

    void Shutdown()
    {
        if (sDissectorWinsockData.mListenSocket != INVALID_SOCKET)
        {
            closesocket(sDissectorWinsockData.mListenSocket);
            sDissectorWinsockData.mListenSocket = INVALID_SOCKET;
        }

        if (sDissectorWinsockData.mConnectionSocket != INVALID_SOCKET)
        {
            closesocket(sDissectorWinsockData.mConnectionSocket);
            sDissectorWinsockData.mConnectionSocket = INVALID_SOCKET;
        }
        WSACleanup();
    }
};