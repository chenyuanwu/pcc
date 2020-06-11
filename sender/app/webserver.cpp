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
#include <unistd.h>
#include "cc.h"
using namespace std;

#ifndef WIN32
void* monitor(void*);
#else
DWORD WINAPI monitor(LPVOID);
#endif

int main(int argc, char* argv[])
{

    if ((argc < 2) || ((argc > 2) && (0 == atoi(argv[2]))))
    {
        cout << "usage: webserver file_to_send [port] [bindip]" << endl;
        return -1;
    }
    UDT::startup();

    addrinfo hints;
    addrinfo* res;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    string service("9000");
    if (3 == argc)
        service = argv[2];

    char * node = NULL;
    if (4 == argc)
        node = argv[3];

    if (0 != getaddrinfo(node, service.c_str(), &hints, &res))
    {
        cout << "illegal port number or port is busy.\n" << endl;
        return 0;
    }

    UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // Windows UDP issue
    // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
    int mss = 1052;
    UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));
#endif

    if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
    {
        cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }

    freeaddrinfo(res);

    cout << "server is ready at port: " << service << endl;

    UDT::listen(serv, 10);

    sockaddr_storage clientaddr;
    int addrlen = sizeof(clientaddr);

    UDTSOCKET fhandle;

    if (UDT::INVALID_SOCK == (fhandle = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen)))
    {
        cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }

    char clienthost[NI_MAXHOST];
    char clientservice[NI_MAXSERV];
    getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
    cout << "new connection: " << clienthost << ":" << clientservice << endl;

    UDT::setsockopt(fhandle, 0, UDT_CC, new CCCFactory<BBCC>, sizeof(CCCFactory<BBCC>));

#ifndef WIN32
    pthread_create(new pthread_t, NULL, monitor, &fhandle);
#else
    CreateThread(NULL, 0, monitor, &client, 0, NULL);
#endif

    // using CC method
    BBCC* cchandle = NULL;
    int temp;
    UDT::getsockopt(fhandle, 0, UDT_CC, &cchandle, &temp);
//    if (NULL != cchandle)
//        cchandle->setRate(1);

    char file[1024];
    strcpy(file,argv[1]);
    fstream ifs(file, ios::in | ios::binary);

    ifs.seekg(0, ios::end);
    int64_t size = ifs.tellg();
    ifs.seekg(0, ios::beg);
    cout<<"size is "<<size<<endl;

    int64_t sent_size;
    // send file size information
    if (UDT::ERROR == (sent_size = UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0)))
    {
        cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
    cout<<"sentsize is"<<sent_size<<endl;

    UDT::TRACEINFO trace;
    UDT::perfmon(fhandle, &trace);

    // send the file
    int64_t offset = 0;

    if (UDT::ERROR == (sent_size = UDT::sendfile(fhandle, ifs, offset, size)))
    {
        cout << "sendfile: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
    UDT::perfmon(fhandle, &trace);
    cout<<"sentsize is "<<sent_size<<"; measured is "<<trace.pktTotalBytes<<endl;
    UDT::close(fhandle);

    // use this function to release the UDT library
    UDT::cleanup();

    ifs.close();

    return 0;
}

#ifndef WIN32
void* monitor(void* s)
#else
DWORD WINAPI monitor(LPVOID s)
#endif
{
    UDTSOCKET u = *(UDTSOCKET*)s;

    UDT::TRACEINFO perf;

    cout << "SendRate(Mb/s)\tRTT(ms)\tCWnd\tPktSndPeriod(us)\tRecvACK\tRecvNAK" << endl;
    int i=0;
    while (true)
    {
#ifndef WIN32
        usleep(1000000);
#else
        Sleep(1000);
#endif
        i++;
        if(i>10000)
        {
            exit(1);
        }
        if (UDT::ERROR == UDT::perfmon(u, &perf))
        {
            cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
            break;
        }
        cout << perf.mbpsSendRate << "\t\t"
             << perf.msRTT << "\t"
             <<  perf.pktSentTotal << "\t"
             << perf.pktSndLossTotal << "\t\t\t"
             << perf.pktRecvACKTotal << "\t"
             << perf.pktRecvNAKTotal << endl;

    }

#ifndef WIN32
    return NULL;
#else
    return 0;
#endif
}



