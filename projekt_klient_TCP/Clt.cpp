#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <string>
#include "immintrin.h" // avx
#include <WinSock2.h>
#include <string>
#include "methods.h"

int main() {
    float sum = -1;
    float max = -1;
    LARGE_INTEGER start, end;
    float* result = new float[DATANUM];
    float* rawFloatData = new float[DATANUM];
    float* floatSorts = new float[DATANUM];

    long long time_consumed1, time_consumed2;

    for (size_t i = 0; i <  DATANUM; i++)
    {
        *(rawFloatData + i) = float(i + DATANUM + 1);


        *(result + i) = 0;
        *(floatSorts + i) = 0;
    }
    // 初始化 Winsock
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
    addr.sin_addr.s_addr = inet_addr("192.168.43.250");
    addr.sin_port = htons(8083);

    // 连接到服务器
    if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) {
        std::cerr << "Connection error" << std::endl;
        closesocket(Connection);
        WSACleanup();
        return -1;
    }

    QueryPerformanceCounter(&start);
    sum = sumSpeedUpManual(rawFloatData, DATANUM);

    while (true) {
        // 从键盘读取 double 值
        // std::cout << "连接成功 ";


        // 转换为字符串并发送
        //std::string dataStr = std::to_string(sum1);
        send(Connection, (char*)&sum, sizeof(sum), NULL);

        // 接收服务器返回的状态
        char status;
        recv(Connection, &status, sizeof(status), 0);
        std::cout << "Received status: " << status << std::endl;

        if (status == '1') {
            // 如果状态不为1，退出循环
            break;
        }
    }
    QueryPerformanceCounter(&end);
    std::cout << "Time Consumed : " << (end.QuadPart - start.QuadPart) << std::endl;
    std::cout << "输出求和结果 : " << sum << std::endl << std::endl;


    QueryPerformanceCounter(&start);
    max = maxSpeedUp(rawFloatData, DATANUM);
    while (true) {
        // 从键盘读取 double 值
        //std::cout << "Enter a double value to send (0 to exit): ";


        //转换为字符串并发送
        //std::string dataStr = std::to_string(sum1);
        //send(Connection, dataStr.c_str(), dataStr.size(), 0);
        send(Connection, (char*)&max, sizeof(max), NULL);

        // 接收服务器返回的状态
        char status;
        recv(Connection, &status, sizeof(status), 0);
        std::cout << "Received status: " << status << std::endl;

        if (status == '1') {
            // 如果状态为1，退出循环
            break;
        }
    }
    QueryPerformanceCounter(&end);
    //销毁网络连接
    closesocket(Connection);
    WSACleanup();

    std::cout << "Time Consumed : " << (end.QuadPart - start.QuadPart) << std::endl;
    std::cout << "输出最大值结果 : " << max << std::endl;

    // 清理
    closesocket(Connection);
    WSACleanup();
    delete[] rawFloatData, result, floatSorts;
    return 0;
}