#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <string>
#include "immintrin.h" // avx
#include <WinSock2.h>
#include <string>

#define MAX_THREADS 64
#define SUBDATANUM 1000000
#define DATANUM (SUBDATANUM * MAX_THREADS)   
float sumSpeedUp1(const float data[], const int len) {
    //assert(len % 8 == 0 && "要求N为8的倍数");
    if (len % 8)
        std::cout << "要求N为8的倍数" << std::endl;

    float result = 0;
    int per_iter = len / 8;

    __m256 sum = _mm256_setzero_ps();
    __m256 c = _mm256_setzero_ps();
    __m256* ptr = (__m256*)data;


#pragma omp parallel for
    for (int i = 0; i < per_iter; ++i) {
        // 计算 y，减去运行补偿。
        __m256 right = _mm256_log_ps(_mm256_sqrt_ps(ptr[i]));
        __m256 y = _mm256_sub_ps(right, c);

        // 计算中间和。
        __m256 t = _mm256_add_ps(sum, y);

        // 更新运行补偿。
        c = _mm256_sub_ps(_mm256_sub_ps(t, sum), y);
#pragma omp critical
        // 更新累加器。
        sum = t;
    }

    float* resultArray = (float*)&sum;
    result = resultArray[0] + resultArray[1] + resultArray[2] + resultArray[3] +
        resultArray[4] + resultArray[5] + resultArray[6] + resultArray[7];
    return result;

}


int main() {
    float sum1 = 1, sum2 = 0;
    float max1 = 0, max2 = 0;
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
    sum1 = sumSpeedUp1(rawFloatData, DATANUM);

    while (true) {
        // 从键盘读取 double 值
        std::cout << "Enter a double value to send (0 to exit): ";


        // 转换为字符串并发送
        std::string dataStr = std::to_string(sum1);
        send(Connection, dataStr.c_str(), dataStr.size(), 0);

        if (sum1 == -1) {
            // 如果输入为0，退出循环
            continue;
        }

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
    //销毁网络连接
    closesocket(Connection);
    WSACleanup();

    //for (int i = 0; i < 10; i++)
    //{
    //	std::cout << result[i] << '\t';
    //}
    //std::cout << std::endl;

    std::cout << "Time Consumed 1 : " << (end.QuadPart - start.QuadPart) << std::endl;
    time_consumed1 = end.QuadPart - start.QuadPart;
    std::cout << "输出求和结果 : " << sum1 << std::endl;

    // 清理
    closesocket(Connection);
    WSACleanup();
    delete[] rawFloatData, result, floatSorts;
    return 0;
}
