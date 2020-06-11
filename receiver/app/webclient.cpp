#ifndef WIN32
#include <cstdlib>
#include <netdb.h>
#else
#include <winsock2.h>
   #include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstring>
#include <udt.h>

using namespace std;

int main(int argc, char* argv[])
{
    if ((3 != argc) || (0 == atoi(argv[2])))
    {
        cout << "usage: webclient server_ip server_port" << endl;
        return 0;
    }

    // use this function to initialize the UDT library
    UDT::startup();

    struct addrinfo hints, *peer;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    UDTSOCKET fhandle = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

    if (0 != getaddrinfo(argv[1], argv[2], &hints, &peer))
    {
        cout << "incorrect server/peer address. " << argv[1] << ":" << argv[2] << endl;
        return -1;
    }

    // connect to the server, implict bind
    if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))
    {
        cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
        return -1;
    }

    freeaddrinfo(peer);

    // aquiring file name information from client
    char file[1024]="./file_received";

    fstream ofs(file, ios::out | ios::binary | ios::trunc);

    // get size information
    int64_t size;

    if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))
    {
        cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
    }

    if (size < 0)
    {
        cout << "no such file on the server\n";
    }
    cout<<"size is "<<size<<endl;
    int64_t recvsize;
    int64_t offset = 0;

    if (UDT::ERROR == (recvsize = UDT::recvfile(fhandle, ofs, offset, size)))
    {
        cout << "recvfile: " << UDT::getlasterror().getErrorMessage() << endl;
    }
    cout<<"recv size is"<<recvsize<<endl;

    UDT::close(fhandle);

    // use this function to release the UDT library
    UDT::cleanup();

    return 0;
}


