#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <string>
#include "immintrin.h" // avx
#include <WinSock2.h>
#include <cuda_runtime.h>
#include "CPUmethods.h"
#include "CUDAmethods.cuh"

//LARGE_INTEGER start, end;
float* hostInput, * hostOutput;      // host
float* deviceInput, * deviceOutput;  // GPU
extern const int data_len;

//Ethernet
//addr.sin_addr.s_addr = inet_addr("169.254.75.32");

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
    //addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //addr.sin_addr.s_addr = inet_addr("169.254.75.32");
    //addr.sin_addr.s_addr = inet_addr("192.168.43.250");
    addr.sin_port = htons(8082);
    addr.sin_addr.s_addr = inet_addr("169.254.112.212");
    
    // 连接到服务器
    if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) {
        std::cerr << "Connection error" << std::endl;
        closesocket(Connection);
        WSACleanup();
        return -1;
    }
    return Connection;
}

int main()
{
    float sumClt = -1;
    float maxClt = -1;
    
    float* rawFloatData = new float[DATANUM/2];
    hostInput = new float[DATANUM/2];
    

    long long time_consumed1[5] = { 0 }, time_consumed2[5] = { 0 };
    double time1 = 0, time2 = 0;

    for (size_t i = 0; i <  DATANUM/2; i++)
    {
        *(rawFloatData + i) = float(i + DATANUM /2+ 1);
        *(hostInput + i) = float(i + DATANUM / 2 + 1);
    }
   
    SOCKET Connection = web_init();

    // 求和
    std::cout << "#########################################" << std::endl;
    std::cout << "  SUM : " << std::endl;


    for (int i = 0; i < 5; i++)
    {
        cudaMalloc((void**)&deviceInput, DATANUM / 2 * sizeof(float));
        cudaMalloc((void**)&deviceOutput, sizeof(float));
        cudaMemcpy(deviceInput, hostInput, DATANUM / 2 * sizeof(float), cudaMemcpyHostToDevice);
        sumClt = sumSpeedUpCUDA();
        
        if (SOCKET_ERROR == send(Connection, (char*)&sumClt, sizeof(sumClt), NULL))
            return WSAGetLastError();

        cudaFree(deviceInput);
        cudaFree(deviceOutput);
        syncAndCheckCUDAError();
    }

    // 求最大值
    std::cout << "#########################################" << std::endl;
    std::cout << "  MAX : " << std::endl;

    cudaMalloc((void**)&deviceInput, DATANUM / 2 * sizeof(float));
    cudaMalloc((void**)&deviceOutput, sizeof(float));
    cudaMemcpy(deviceInput, hostInput, DATANUM / 2 * sizeof(float), cudaMemcpyHostToDevice);
    for (int i = 0; i < 5; i++)
    {
        maxClt = maxSpeedUpCUDA();
        if (SOCKET_ERROR == send(Connection, (char*)&maxClt, sizeof(maxClt), NULL))
            return WSAGetLastError();
    }
    cudaFree(deviceInput);
    cudaFree(deviceOutput);
    syncAndCheckCUDAError();

    // 排序
    std::cout << "#########################################" << std::endl;
    std::cout << "  SORT : " << std::endl;

    cudaMalloc((void**)&deviceInput, DATANUM/2 * sizeof(float));
    cudaMalloc((void**)&deviceOutput, DATANUM/2 * sizeof(float));
    

    for (int i = 0; i < 5; i++)
    {
        float* result = new float[DATANUM / 2];
        mergesortCUDA(rawFloatData, result, DATANUM/2);

        const int CHUNK_SIZE = 4096;  // 每个块的大小为 2048 字节

        for (int i = 0; i < DATANUM / 2; i += CHUNK_SIZE) {

            int size = CHUNK_SIZE;
            send(Connection, (char*)&result[i], 4 * size, NULL);
        }
        delete[] result;
    }    


    closesocket(Connection);
    WSACleanup();
    delete[] rawFloatData;
    delete[] hostInput;
    return 0;
}