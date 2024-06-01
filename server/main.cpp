// Original Author: Hayden Karp
// I'm relatively unfamiliar with Winsock so I'm going to document some things to make sure I retain the info

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
#include <vector>
#include <sstream>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "ws2_32")

using namespace std;

string isolate_host_name (std::string str){
    u_int64 f = str.find('\n');
    while (f != std::string::npos){
        str.replace(f, 1, " ");

        f = str.find('\n');
    }

    f = str.find('\r');
    while (f != std::string::npos){
        str.replace(f, 1, "");

        f = str.find('\r');
    }

    std::string s;

    stringstream inp_stream (str);
    std::vector <string> v;

    while (std::getline (inp_stream, s, ' ')){

        v.push_back (s);
    }


    string host_name;

    for (int i = 0; i < v.size (); i++)
    {
        if (v[i].find("Host") != std::string::npos)
        {
            host_name = v[i + 1];
            break;
        }
    }

    return host_name;
}

int main() {
    //std::cout << "   __    ____   ___  _____  _  _ \n  /__\\  (  _ \\ / __)(  _  )( \\( )\n /(__)\\  )   /( (_-. )(_)(  )  ( \n(__)(__)(_)\\_) \\___/(_____)(_)\\_)\nStarting Server...";

    // Startup Winsock (it'll perform system specific checks during this time)

    WSADATA wsa_data;

    int wsa_res;

    wsa_res = WSAStartup(MAKEWORD(2,2), &wsa_data);
    if (wsa_res != 0) {
        std::cout << "WSA Startup Failed: Code " << wsa_res;
        return 1;
    }

    struct addrinfo *result = NULL, *ptr = NULL, target_address; // object that contains information about the hosted port

    // cant use calloc here because calloc is specifically for array type data structures and not structs (I think)
    ZeroMemory(&target_address, sizeof(target_address));

    target_address.ai_family = AF_INET; // specifies IPV4 family addresses are expected
    target_address.ai_socktype = SOCK_STREAM; // specifies this socket will be streaming information as it receives it
    target_address.ai_protocol = IPPROTO_TCP; // specifies the socket will be utilizing the TCP data transfer protocol
    target_address.ai_flags = AI_PASSIVE; // this one is weird, but how I best understand it is this tells Winsock that we intend on using this target address in a bind call (later on) and thus it knows that if the 'nodename' (first parameter) of the getaddrinfo function is NULL, it will allow all IP Adresses to query the socket (austensibly making it public)

    int addr_info_res;

    // Winsock is essentially double-checking our work and putting together all the info for what we want our socket to do in the result pointer
    addr_info_res = getaddrinfo(NULL, DEFAULT_PORT, &target_address, &result);

    if (addr_info_res != 0){
        std::cout << "'getaddrinfo' failed, check default port or code.";
        return 1;
    }

    // Create the actual sockets

    SOCKET server_receive_socket = INVALID_SOCKET;

    server_receive_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    if (server_receive_socket == INVALID_SOCKET){
        std::cout << "Invalid socket parameters, socket failed to start error: " << WSAGetLastError();

        // Special free and cleanup functions for sockets
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Before this, the socket just exists in memory untied to a specific hardware port, binding connects it directly to an actual port where it can listen for data

    int bind_res;

    bind_res = bind(server_receive_socket, result->ai_addr, (int)result->ai_addrlen);

    if (bind_res != 0){
        std::cout << "Binding socket failed, error: " << WSAGetLastError();

        freeaddrinfo(result);
        closesocket(server_receive_socket); // Socket must also be closed this time
        WSACleanup();

        return 1;
    }

    // because we now have the socket binded to hardware its data/config no longer just exists in the result variable, so we can free it to avoid wasted memory
    freeaddrinfo(result);

    // SOMAXCONN is a Winsock constant that represents the maximum reasonable backlog amount that winsock can support
    int listen_res = listen(server_receive_socket, SOMAXCONN);

    if (listen_res == SOCKET_ERROR){
        std::cout << "General server exception, error: " << WSAGetLastError();

        freeaddrinfo(result);
        closesocket(server_receive_socket); // Socket must also be closed this time
        WSACleanup();

        return 1;
    }

    // TODO Implement multi-threaded server responses

    SOCKET client_socket = INVALID_SOCKET;
    int client_res, server_send_res;
    char receive_buffer[DEFAULT_BUFFER_LENGTH];
    int receive_buffer_len = DEFAULT_BUFFER_LENGTH;

    while (true){
        client_res = 1;
        client_socket = accept(server_receive_socket, NULL, NULL);

        if (client_socket == INVALID_SOCKET){
            std::cout << "Unable to accept client socket, error: " << WSAGetLastError();

            freeaddrinfo(result);
            closesocket(server_receive_socket);
            WSACleanup();

            return 1;
        }

        SOCKET pseudo_client_socket = INVALID_SOCKET;
        char pseudo_client_receive_buffer[DEFAULT_BUFFER_LENGTH];
        int pseudo_client_receive_buffer_len = DEFAULT_BUFFER_LENGTH;

        while (client_res > 0){
            client_res = recv(client_socket, receive_buffer, receive_buffer_len, 0); // populates a buffer with received data

            if (client_res > 0){
                if (LOG_RECEIVED_BYTES){
                    std::cout << "Bytes Received From Client: " << receive_buffer << "\n";
                }

                // Generate pseudo client to make request to host

                struct addrinfo pseudo_client_hints, *pseudo_client_lookup_res;

                ZeroMemory( &pseudo_client_hints, sizeof(pseudo_client_hints) );
                pseudo_client_hints.ai_family = AF_UNSPEC;
                pseudo_client_hints.ai_socktype = SOCK_STREAM;
                pseudo_client_hints.ai_protocol = IPPROTO_TCP;

                int find_target_host_res;
                struct hostent *request_host;

                string host_name = isolate_host_name(receive_buffer);
                char* extracted_host_name = new char[host_name.length() + 1];

                strcpy(extracted_host_name, host_name.c_str());

                request_host = gethostbyname(extracted_host_name);

                cout << inet_ntoa (*((struct in_addr *) request_host->h_addr_list[0])) << endl;

                find_target_host_res = getaddrinfo(inet_ntoa (*((struct in_addr *) request_host->h_addr_list[0])), "80", &pseudo_client_hints, &pseudo_client_lookup_res);
                if ( find_target_host_res != 0 ) {
                    cout << "Unable to find requested endpoint, error: " << WSAGetLastError();
                    WSACleanup();
                    return 1;
                }

                // Attempt to connect to an address until one succeeds
                for(ptr=pseudo_client_lookup_res; ptr != NULL ;ptr=ptr->ai_next) {

                    // Create a SOCKET for connecting to server
                    pseudo_client_socket = socket(ptr->ai_family, ptr->ai_socktype,
                                           ptr->ai_protocol);
                    if (pseudo_client_socket == INVALID_SOCKET) {
                        cout << "Pseudo Client failed to establish itself with error: " << WSAGetLastError();
                        WSACleanup();
                        return 1;
                    }

                    int pseudo_client_connection_res;

                    // Connect to server.
                    pseudo_client_connection_res = connect( pseudo_client_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
                    if (pseudo_client_connection_res == SOCKET_ERROR) {
                        if (LOG_RECEIVED_BYTES){
                            cout << "Failed to connect to address: " << ptr->ai_addr << endl;
                        }

                        closesocket(pseudo_client_socket);
                        pseudo_client_socket = INVALID_SOCKET;
                        continue;
                    }
                    break;
                }

                freeaddrinfo(pseudo_client_lookup_res);

                if (pseudo_client_socket == INVALID_SOCKET) {
                    printf("Cannot connect to target host, provided host may be invalid?\n");
                    WSACleanup();
                    return 1;
                }


                // Pass data from client to intended endpoint
                int proxy_data_to_endpoint_res;
                proxy_data_to_endpoint_res = send( pseudo_client_socket, receive_buffer, (int)strlen(receive_buffer), 0 );

                if (proxy_data_to_endpoint_res == SOCKET_ERROR) {
                    cout << "Failed data proxy to intended endpoint with error: " << WSAGetLastError();
                    closesocket(pseudo_client_socket);
                    WSACleanup();
                    return 1;
                }

                int close_connection_to_endpoint_res;

                close_connection_to_endpoint_res = shutdown(pseudo_client_socket, SD_SEND);
                if (close_connection_to_endpoint_res == SOCKET_ERROR) {
                    cout << "Failed to close connection with intended endpoint with error: " << WSAGetLastError();
                    closesocket(pseudo_client_socket);
                    WSACleanup();
                    return 1;
                }

                int intended_endpoint_res = 1;

                // Receive until the peer closes the connection
                while( intended_endpoint_res > 0 ) {

                    intended_endpoint_res = recv(pseudo_client_socket, pseudo_client_receive_buffer, pseudo_client_receive_buffer_len, 0);
                    if ( intended_endpoint_res > 0 ){
                        if (LOG_RECEIVED_BYTES){
                            cout << "Received bytes from intended endpoint: " << pseudo_client_receive_buffer << endl;
                        }

                        server_send_res = send(client_socket, pseudo_client_receive_buffer, pseudo_client_receive_buffer_len, 0);

                        if (server_send_res == SOCKET_ERROR){
                            std::cout << "Failed to send response to client, error: " << WSAGetLastError();

                            freeaddrinfo(result);
                            closesocket(client_socket);
                            WSACleanup();

                            return 1;
                        }
                    } else if (intended_endpoint_res == 0) {
                        if (LOG_RECEIVED_BYTES){
                            cout << "Closed connection with intended endpoint" << endl;
                        }
                    }else{
                        cout << "Error occured while receiving data from the intended endpoint: " << WSAGetLastError();
                    }


                }
            }else if (client_res == 0){
                if (LOG_RECEIVED_BYTES){
                    std::cout << "Closing Connection With Client" << "\n";
                }
            }else{
                std::cout << "Failed to receive data client, error: " << WSAGetLastError();

                freeaddrinfo(result);
                closesocket(client_socket);
                WSACleanup();

                return 1;
            }
        }
    }

    return 0;
}
