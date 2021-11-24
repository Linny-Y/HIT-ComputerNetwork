#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <string.h>
#include <tchar.h>
#include <string>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#define MAXSIZE 65507 // �������ݱ��ĵ���󳤶�
#define HTTP_PORT 80  // http�������˿�
// Http ��Ҫͷ������
struct HttpHeader
{
    char method[4];         // POST ���� GET��ע����ЩΪ CONNECT����ʵ���ݲ�����
    char url[1024];         // ����� url
    char host[1024];        // Ŀ������
    char cookie[1024 * 10]; // cookie
    HttpHeader()
    {
        ZeroMemory(this, sizeof(HttpHeader));
    }
};
BOOL InitSocket();
void ParseHttpHead(char *buffer, HttpHeader *httpHeader);
BOOL ConnectToServer(SOCKET *serverSocket, char *host);
unsigned int __stdcall ProxyThread(LPVOID lpParameter);
void Phishing(char *buffer, HttpHeader *httpHeader);
boolean ParseDate(char *buffer, char *tempDate);
void makeNewHTTP(char *buffer, char *Date);

//������ز���
SOCKET ProxyServer;
sockaddr_in ProxyServerAddr;
const int ProxyPort = 10240;

//������ز���
boolean haveCache = false;
struct Cache
{
    HttpHeader *htp;
    char buffer[MAXSIZE];
    char date[1024];
    Cache()
    {
        ZeroMemory(this->buffer, MAXSIZE);
        ZeroMemory(this->date, 1024);
        this->htp = new HttpHeader();
    }
};
Cache *cache[1024];
int cache_Pos;
//�����µ����Ӷ�ʹ�����߳̽��д������̵߳�Ƶ���Ĵ����������ر��˷���Դ
//����ʹ���̳߳ؼ�����߷�����Ч��
// const int ProxyThreadMaxNum = 20;
// HANDLE ProxyThreadHandle[ProxyThreadMaxNum] = {0};
// DWORD ProxyThreadDW[ProxyThreadMaxNum] = {0};
struct ProxyParam
{
    SOCKET clientSocket;
    SOCKET serverSocket;
};
#define NUM 100
char *Filter_website[NUM] = {}; // (char *)"today.hit.edu.cn"
char *Filter_users[NUM] = {};   // (char *)"127.0.0.1"
char *Phishing_website[NUM] = {(char *)"jwts.hit.edu.cn"};  // 
char *Target_website[NUM] = {(char *)"today.hit.edu.cn"};//

int _tmain(int argc, _TCHAR *argv[])
{
    printf("�����������������\n");
    printf("��ʼ��...\n");
    if (!InitSocket())
    {
        printf("socket ��ʼ��ʧ��\n");
        return -1;
    }
    printf("����������������У������˿� %d\n", ProxyPort);
    SOCKET acceptSocket = INVALID_SOCKET;
    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    ProxyParam *lpProxyParam;
    HANDLE hThread;
    DWORD dwThreadID;
    //������������ϼ���
    while (true)
    {
        acceptSocket = accept(ProxyServer, (SOCKADDR *)&remoteAddr, &nAddrlen);
        lpProxyParam = new ProxyParam;
        if (lpProxyParam == NULL)
        {
            continue;
        }
        //////////////////////////////// �����û�
        BOOL blocked = false;
        for (int i = 0; i < NUM; i++)
        {
            if (Filter_users[i] == NULL)
            {
                break;
            }
            if (strcmp(inet_ntoa(remoteAddr.sin_addr), Filter_users[i]) == 0)
            {
                blocked = true;
                break;
            }
        }
        if (blocked)
        {
            printf("==============User blocked.================\n\n");
            continue;
        }
        //////////////////////////////////
        lpProxyParam->clientSocket = acceptSocket;
        printf("Recieve a connection from %s:%d\n\n", inet_ntoa(remoteAddr.sin_addr), remoteAddr.sin_port);
        hThread = (HANDLE)_beginthreadex(NULL, 0,
                                         &ProxyThread, (LPVOID)lpProxyParam, 0, 0);

        CloseHandle(hThread);
        Sleep(200);
    }
    closesocket(ProxyServer);
    WSACleanup();
    return 0;
}

//************************************
// Method: InitSocket
// FullName: InitSocket
// Access: public
// Returns: BOOL
// Qualifier: ��ʼ���׽���
//************************************
BOOL InitSocket()
{
    //�����׽��ֿ⣨���룩
    WORD wVersionRequested;
    WSADATA wsaData;
    //�׽��ּ���ʱ������ʾ
    int err;
    //�汾 2.2
    wVersionRequested = MAKEWORD(2, 2);
    //���� dll �ļ� Scoket ��
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        //�Ҳ��� winsock.dll
        printf("���� winsock ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        printf("�����ҵ���ȷ�� winsock �汾\n");
        WSACleanup();
        return FALSE;
    }
    ProxyServer = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == ProxyServer)
    {
        printf("�����׽���ʧ�ܣ��������Ϊ: %d\n", WSAGetLastError());
        return FALSE;
    }
    ProxyServerAddr.sin_family = AF_INET;
    ProxyServerAddr.sin_port = htons(ProxyPort);
    ProxyServerAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(ProxyServer, (SOCKADDR *)&ProxyServerAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        printf("���׽���ʧ��\n");
        return FALSE;
    }
    if (listen(ProxyServer, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("�����˿�%d ʧ��", ProxyPort);
        return FALSE;
    }
    return TRUE;
}

//************************************
// Method: ConnectToServer
// FullName: ConnectToServer
// Access: public
// Returns: BOOL
// Qualifier: ������������Ŀ��������׽��֣�������
// Parameter: SOCKET * serverSocket
// Parameter: char * host
//************************************
BOOL ConnectToServer(SOCKET *serverSocket, char *host)
{
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);
    HOSTENT *hostent = gethostbyname(host);
    ///////////////////////////////////////// ������ҳ
    for (int i = 0; i < NUM; i++)
    {
        if (Filter_website[i] == NULL)
        {
            break;
        }
        if (strcmp(host, Filter_website[i]) == 0)
        {
            printf("====================Website blocked.===================\n");
            return FALSE;
        }
    }
    /////////////////////////////////////////
    if (!hostent)
    {
        return FALSE;
    }
    in_addr Inaddr = *((in_addr *)*hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == INVALID_SOCKET)
    {
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}
//************************************
// Method: ProxyThread
// FullName: ProxyThread
// Access: public
// Returns: unsigned int __stdcall
// Qualifier: �߳�ִ�к���
// Parameter: LPVOID lpParameter
//************************************
unsigned int __stdcall ProxyThread(LPVOID lpParameter)
{
    // GET http://today.hit.edu.cn/ HTTP/1.1
    // User-Agent: PostmanRuntime/7.28.3
    // Accept: */*
    // Postman-Token: fc56543e-1ed4-4a0e-8e6c-a01b278ea132
    // Host: today.hit.edu.cn
    // Accept-Encoding: gzip, deflate, br
    // Connection: keep-alive
    char Buffer[MAXSIZE];
    char *CacheBuffer;
    ZeroMemory(Buffer, MAXSIZE);
    SOCKADDR_IN clientAddr;
    int length = sizeof(SOCKADDR_IN);
    int recvSize;
    int ret;
    recvSize = recv(((ProxyParam *)lpParameter)->clientSocket, Buffer, MAXSIZE, 0); // ���ձ���
    HttpHeader *httpHeader = new HttpHeader();
    char *DateBuffer;
    char *field = (char *)"Date: ";
    char date[40]; // ����date�ֶ�
    char *p, *ptr, num[10], tempBuffer[MAXSIZE + 1];
    const char *delim = "\r\n";
    if (recvSize <= 0)
    {
        goto error;
    }
    CacheBuffer = new char[recvSize + 1];
    ZeroMemory(CacheBuffer, recvSize + 1);
    memcpy(CacheBuffer, Buffer, recvSize);
    // ���� httpͷ��
    ParseHttpHead(CacheBuffer, httpHeader);
    delete CacheBuffer;
    ////////////////////////////////// ������վ
    Phishing(Buffer, httpHeader);
    //////////////////////////////////
    ///////////////////////////////////////////����Cache

    for (int i = 0; i < 1024; i++)
    { // ���һ���
        if (cache[i] == NULL)
        {
            haveCache = false;
            cache_Pos = i; // ��һ�����ŵĻ���λ��
            break;
        }
        if (!strcmp(cache[i]->htp->method, httpHeader->method) && !strcmp(cache[i]->htp->url, httpHeader->url) && !strcmp(cache[i]->htp->host, httpHeader->host))
        {
            printf("=======================�������======================\n");
            haveCache = true;
            memcpy(date, cache[i]->date, 40);
            cache_Pos = i;
            makeNewHTTP(Buffer, date); // �޸�������
            printf("******************%s***************",Buffer);
            break;
        }
    }
    if (!ConnectToServer(&((ProxyParam
                                *)lpParameter)
                              ->serverSocket,
                         httpHeader->host))
    {
        goto error;
    }
    printf("������������ %s �ɹ�\n\n", httpHeader->host);
    //���ͻ��˷��͵� HTTP ���ݱ���ֱ��ת����Ŀ�������
    ret = send(((ProxyParam *)lpParameter)->serverSocket, Buffer, strlen(Buffer) + 1, 0);
    //�ȴ�Ŀ���������������
    recvSize = recv(((ProxyParam
                          *)lpParameter)
                        ->serverSocket,
                    Buffer, MAXSIZE, 0);

    if (recvSize <= 0)
    {
        goto error;
    }
    // �鿴�Ƿ����
    ZeroMemory(num, 10);
    ZeroMemory(tempBuffer, MAXSIZE + 1);
    memcpy(tempBuffer, Buffer, strlen(Buffer));
    p = strtok_s(tempBuffer, delim, &ptr); //ȡ���ĵ�һ�� ps. "HTTP/1.1 200 OK"
    memcpy(num, &p[9], 3);                 // ��ȡ״̬��
    if (strcmp(num, "304") == 0)
    { //״̬��Ϊ304 ʹ�ñ��ػ���
        printf("=====================ʹ�ñ��ػ���====================\n");
        memcpy(Buffer, cache[cache_Pos]->buffer, MAXSIZE);
    }
    else if (strcmp(num, "200") == 0)
    { //״̬��Ϊ200 ���»���
        if (haveCache == false)
        {
            cache[cache_Pos] = new Cache();
            memcpy(cache[cache_Pos]->htp->method, httpHeader->method, strlen(httpHeader->method));
            memcpy(cache[cache_Pos]->htp->url, httpHeader->url, strlen(httpHeader->url));
            memcpy(cache[cache_Pos]->htp->host, httpHeader->host, strlen(httpHeader->host));
            memcpy(cache[cache_Pos]->htp->cookie, httpHeader->cookie, strlen(httpHeader->cookie));
        }
        ParseDate(Buffer, date); // ��ȡʱ��
        memcpy(cache[cache_Pos]->date, date, strlen(date));
        memcpy(cache[cache_Pos]->buffer, Buffer, strlen(Buffer));
    }

    //��Ŀ����������ص�����ֱ��ת�����ͻ���
    ret = send(((ProxyParam
                     *)lpParameter)
                   ->clientSocket,
               Buffer, sizeof(Buffer), 0);
    printf("%s\n\n", Buffer); // �����վ����
//������
error:
    printf("\n\n�ر��׽���......\n\n");
    Sleep(200);
    closesocket(((ProxyParam *)lpParameter)->clientSocket);
    closesocket(((ProxyParam *)lpParameter)->serverSocket);
    _endthreadex(0);
    return 0;
}
//����HTTPͷ�� "Date: "�ֶ� ��ȡʱ��
boolean ParseDate(char *buffer, char *tempDate)
{
    char *p, *ptr, temp[5];
    char *field = (char *)"Date: ";
    const char *delim = "\r\n";
    ZeroMemory(temp, 5);
    char tempbuffer[MAXSIZE];
    memcpy(tempbuffer, buffer, MAXSIZE);
    p = strtok_s(tempbuffer, delim, &ptr); // ��ȡһ��
    int len = strlen(field);
    while (p)
    { // ѭ������
        if (strstr(p, field) != NULL)
        { // �� buffer�в��� "Date: " �ֶ�  ps.Date: Thu, 28 Oct 2021 14:32:43 GMT
            memcpy(tempDate, &p[len], strlen(p) - len);
            return true;
        }
        p = strtok_s(NULL, delim, &ptr);
    }
    return false;
}

//�޸� HTTP������
void makeNewHTTP(char *buffer, char *Date)
{
    char field[100] = "If-Modified-Since: ";
    strcat(field, Date);
    strcat(field, "\r\n");
    string line = buffer;
    int pos = line.find("Host");
    line.replace(pos, 0, field);
    for (int i = 0; i < line.size();i++){
                *buffer++ = line[i];
            }
}

//************************************
// Method: Phishing
// FullName: Phishing
// Access: public
// Returns: void
// Qualifier: �޸�url,host,����
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void Phishing(char *buffer, HttpHeader *httpHeader)
{
    for (int i = 0; i < NUM; i++)
    {
        if (Phishing_website[i] == NULL || Target_website[i] == NULL)
        {
            break;
        }
        if (strcmp(Phishing_website[i], httpHeader->host) == 0)
        {
            char url[1024] = "http://";
            strcat(url, Target_website[i]);
            strcat(url, "/");
            memcpy(httpHeader->url, url, strlen(Target_website[i]));
            memcpy(httpHeader->host, Target_website[i], strlen(Target_website[i]));
            string line = buffer;
            // GET http://
            int pos = line.find("Get") + 12;
            int len = strlen(Phishing_website[i]);
            line.replace(pos, len, Target_website[i]);
            pos = line.find("Host: ") + 6;
            len = strlen(Phishing_website[i]);
            line.replace(pos, len, Target_website[i]);
            for (int i = 0; i < line.size();i++){
                *buffer++ = line[i];
            }
            break;
        }
    }
}
//************************************
// Method: ParseHttpHead
// FullName: ParseHttpHead
// Access: public
// Returns: void
// Qualifier: ���� TCP �����е� HTTP ͷ��
// Parameter: char * buffer
// Parameter: HttpHeader * httpHeader
//************************************
void ParseHttpHead(char *buffer, HttpHeader *httpHeader)
{
    char *p;
    char *ptr;
    const char *delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr); //��ȡ��һ��
    printf("%s\n", p);
    if (p[0] == 'G')
    { // GET ��ʽ
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    }
    else if (p[0] == 'P')
    { // POST ��ʽ
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    printf("%s\n", httpHeader->url);
    p = strtok_s(NULL, delim, &ptr);
    while (p)
    {
        switch (p[0])
        {
        case 'H': // Host
            memcpy(httpHeader->host, &p[6], strlen(p) - 6);
            break;
        case 'C': // Cookie
            if (strlen(p) > 8)
            {
                char header[8];
                ZeroMemory(header, sizeof(header));
                memcpy(header, p, 6);
                if (!strcmp(header, "Cookie"))
                {
                    memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
                }
            }
            break;
        default:
            break;
        }
        p = strtok_s(NULL, delim, &ptr);
    }
}
