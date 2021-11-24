#pragma once
#include <iostream>
#include <winsock2.h>

#define REQUEST_SUCCEEDED 0
#define INVALID_ARGUMENT 2
#define METHOD_NOT_DEFINED 1

typedef uint8_t ERROR_CODE;

using namespace std;

class Server
{
public:
    Server(int);

private:
    void CreateChildProcessToHandleRequest(SOCKET);
    void CreateThreadToHandleRequest(SOCKET);
    static ERROR_CODE HandleRequest(const string &, string &);

    static DWORD WINAPI RequestThread(LPVOID);
};