// From Winsock documentation with few changes, for testing purposes only, actual client application will be written in python

#define DEFAULT_PORT "8080"
#define DEFAULT_BUFFER_LENGTH 9064
#define LOG_RECEIVED_BYTES true

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "ws2_32")

  using namespace std;

  int main(int argc, char **argv){
      WSADATA wsaData;
      SOCKET ConnectSocket = INVALID_SOCKET;
      struct addrinfo *result = NULL,
              *ptr = NULL,
              hints;
      const char *sendbuf = "GET / HTTP/1.1\r\n"
                            "User-Agent: Mozilla/5.0\r\n"
                            "Host: www.google.com\r\n"
                            "Accept-Language: en-us\r\n"
                            "Accept-Encoding: gzip, deflate\r\n"
                            "Connection: Keep-Alive\r\n\r\n";
      char recvbuf[DEFAULT_BUFFER_LENGTH];
      int iResult;
      int recvbuflen = DEFAULT_BUFFER_LENGTH;

      // Validate the parameters
      if (argc != 2) {
          printf("usage: %s server-name\n", argv[0]);
          return 1;
      }

      // Initialize Winsock
      iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
      if (iResult != 0) {
          printf("WSAStartup failed with error: %d\n", iResult);
          return 1;
      }

      ZeroMemory( &hints, sizeof(hints) );
      hints.ai_family = AF_UNSPEC;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;

      // Resolve the server address and port
      iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
      if ( iResult != 0 ) {
          printf("getaddrinfo failed with error: %d\n", iResult);
          WSACleanup();
          return 1;
      }

      // Attempt to connect to an address until one succeeds
      for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {

          // Create a SOCKET for connecting to server
          ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                                 ptr->ai_protocol);
          if (ConnectSocket == INVALID_SOCKET) {
              printf("socket failed with error: %ld\n", WSAGetLastError());
              WSACleanup();
              return 1;
          }

          // Connect to server.
          iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
          if (iResult == SOCKET_ERROR) {
              closesocket(ConnectSocket);
              ConnectSocket = INVALID_SOCKET;
              continue;
          }
          break;
      }

      freeaddrinfo(result);

      if (ConnectSocket == INVALID_SOCKET) {
          printf("Unable to connect to server!\n");
          WSACleanup();
          return 1;
      }

      // Send an initial buffer
      iResult = send( ConnectSocket, sendbuf, (int)strlen(sendbuf), 0 );
      if (iResult == SOCKET_ERROR) {
          printf("send failed with error: %d\n", WSAGetLastError());
          closesocket(ConnectSocket);
          WSACleanup();
          return 1;
      }

      printf("Bytes Sent: %ld\n", iResult);

      // shutdown the connection since no more data will be sent
      iResult = shutdown(ConnectSocket, SD_SEND);
      if (iResult == SOCKET_ERROR) {
          printf("shutdown failed with error: %d\n", WSAGetLastError());
          closesocket(ConnectSocket);
          WSACleanup();
          return 1;
      }

      // Receive until the peer closes the connection
      do {

          iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
          if ( iResult > 0 )
              cout << recvbuf << endl;
          else if ( iResult == 0 )
              printf("Connection closed\n");
          else
              printf("recv failed with error: %d\n", WSAGetLastError());

      } while( iResult > 0 );

      // cleanup
      closesocket(ConnectSocket);
      WSACleanup();

      return 0;
}
