#include "server.h"

using namespace std;

Server::Server(int port)
{
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
    {
        return;
    }

    SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slisten == INVALID_SOCKET)
    {
        cout << "create socket error !" << endl;
        return;
    }

    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        cout << "bind error !" << endl;
    }

    if (listen(slisten, 5) == SOCKET_ERROR)
    {
        cout << "listen error !" << endl;
        return;
    }

    SOCKET sClient;
    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    while (true)
    {
        cout << "waiting for connection" << endl;
        sClient = accept(slisten, (SOCKADDR *)&remoteAddr, &nAddrlen);

        if (sClient == INVALID_SOCKET)
        {
            cout << "accept error !" << endl;
            continue;
        }
        cout << "recieve a connection from " << inet_ntoa(remoteAddr.sin_addr) << ":" << remoteAddr.sin_port << endl;
        CreateThreadToHandleRequest(sClient);
    }

    closesocket(slisten);
    WSACleanup();
}

ERROR_CODE Server::HandleRequest(const string &args, string &out_ret)
{
    // 这里请自由发挥
    return REQUEST_SUCCEEDED;
}

DWORD WINAPI Server::RequestThread(LPVOID args)
{
    SOCKET socket = *(SOCKET *)args;
    char revData[255];
    int len;
    while ((len = recv(socket, revData, 255, 0)) > 0)
    {
        string sendData = "hello client, this is TCP server.";
        if (len > 0)
        {
            revData[len] = 0x00;
            string msg(revData);
            HandleRequest(msg, sendData);
        }
        send(socket, sendData.c_str(), sendData.size(), 0);
    }
    cout << "request thread exit." << endl;
    closesocket(socket);
}

void Server::CreateThreadToHandleRequest(SOCKET socket)
{
    DWORD threadID;
    HANDLE threadHandle;
    threadHandle = CreateThread(NULL, 0, RequestThread, &socket, 0, &threadID);
}

void Server::CreateChildProcessToHandleRequest(SOCKET socket)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR lpHandle[20];
    cout << "socket: " << socket << endl;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    wsprintf(lpHandle, TEXT("RequestHandler_Network.exe %d"), socket);
    if (!CreateProcess(NULL,
                       lpHandle, //这里是命令行数据
                       NULL,
                       NULL,
                       TRUE, //这里是允许子进程继承父进程的句柄
                       0,
                       NULL,
                       NULL,
                       &si,
                       &pi))
        cout << "failed to creaet a child process." << endl;
    return;
}