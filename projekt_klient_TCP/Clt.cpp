#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <string>
#include "immintrin.h" // avx
#include <WinSock2.h>
#include <string>
#include "methods.h"

SOCKET web_init()
{
    WSAData wsaData;
    WORD DllVersion = MAKEWORD(2, 1);
    if (WSAStartup(DllVersion, &wsaData) != 0) {
        std::cerr << "Winsock startup error" << std::endl;
        return -1;
    }

    // 创建 socket
    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (Connection == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return -1;
    }

    // 设置服务器地址信息
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = inet_addr("192.168.43.250");
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8083);

    // 连接到服务器
    if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) {
        std::cerr << "Connection error" << std::endl;
        closesocket(Connection);
        WSACleanup();
        return -1;
    }
    return Connection;
}

int main() {
    float sum = -1;
    float max = -1;
    LARGE_INTEGER start, end;
    float* result = new float[DATANUM/2];
    float* rawFloatData = new float[DATANUM/2];

    long long time_consumed1, time_consumed2;

    for (size_t i = 0; i <  DATANUM/2; i++)
    {
        *(rawFloatData + i) = float(i + DATANUM /2+ 1);
        *(result + i) = 5.2f;
    }

    SOCKET Connection = web_init();


    QueryPerformanceCounter(&start);
    sum = sumSpeedUpManual(rawFloatData, DATANUM / 2);

    
    if(SOCKET_ERROR==send(Connection, (char*)&sum, sizeof(sum), NULL))
        return WSAGetLastError();
    
    QueryPerformanceCounter(&end);
    std::cout << "Time Consumed : " << (end.QuadPart - start.QuadPart) << std::endl;
    std::cout << "输出求和结果 : " << sum << std::endl << std::endl;


    QueryPerformanceCounter(&start);
    max = maxSpeedUpManual(rawFloatData, DATANUM / 2);
    if (SOCKET_ERROR == send(Connection, (char*)&max, sizeof(max), NULL))
        return WSAGetLastError();
    QueryPerformanceCounter(&end);
    //销毁网络连接


    std::cout << "Time Consumed : " << (end.QuadPart - start.QuadPart) << std::endl;
    std::cout << "输出最大值结果 : " << max << std::endl;

    //排序：
    QueryPerformanceCounter(&start);
    sortSpeedUpManual(rawFloatData, DATANUM / 2, result);
    const int CHUNK_SIZE = 1024;  // 例如，每个块的大小为 1024 字节
    for (int i = 0; i < DATANUM / 2; i += CHUNK_SIZE) {

        int size = CHUNK_SIZE;
            send(Connection, (char*)&result[i], 4*size, NULL);

    }

 
    
    QueryPerformanceCounter(&end);
    closesocket(Connection);
    WSACleanup();
    delete[] rawFloatData, result;
   
    return 0;
}