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
    //assert(len % 8 == 0 && "Ҫ��NΪ8�ı���");
    if (len % 8)
        std::cout << "Ҫ��NΪ8�ı���" << std::endl;

    float result = 0;
    int per_iter = len / 8;

    __m256 sum = _mm256_setzero_ps();
    __m256 c = _mm256_setzero_ps();
    __m256* ptr = (__m256*)data;


#pragma omp parallel for
    for (int i = 0; i < per_iter; ++i) {
        // ���� y����ȥ���в�����
        __m256 right = _mm256_log_ps(_mm256_sqrt_ps(ptr[i]));
        __m256 y = _mm256_sub_ps(right, c);

        // �����м�͡�
        __m256 t = _mm256_add_ps(sum, y);

        // �������в�����
        c = _mm256_sub_ps(_mm256_sub_ps(t, sum), y);
#pragma omp critical
        // �����ۼ�����
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
    // ��ʼ�� Winsock
    WSAData wsaData;
    WORD DllVersion = MAKEWORD(2, 1);
    if (WSAStartup(DllVersion, &wsaData) != 0) {
        std::cerr << "Winsock startup error" << std::endl;
        return -1;
    }

    // ���� socket
    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (Connection == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
        WSACleanup();
        return -1;
    }

    // ���÷�������ַ��Ϣ
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.43.250");
    addr.sin_port = htons(8083);

    // ���ӵ�������
    if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) {
        std::cerr << "Connection error" << std::endl;
        closesocket(Connection);
        WSACleanup();
        return -1;
    }

    QueryPerformanceCounter(&start);
    sum1 = sumSpeedUp1(rawFloatData, DATANUM);

    while (true) {
        // �Ӽ��̶�ȡ double ֵ
        std::cout << "Enter a double value to send (0 to exit): ";


        // ת��Ϊ�ַ���������
        std::string dataStr = std::to_string(sum1);
        send(Connection, dataStr.c_str(), dataStr.size(), 0);

        if (sum1 == -1) {
            // �������Ϊ0���˳�ѭ��
            continue;
        }

        // ���շ��������ص�״̬
        char status;
        recv(Connection, &status, sizeof(status), 0);
        std::cout << "Received status: " << status << std::endl;

        if (status == '1') {
            // ���״̬��Ϊ1���˳�ѭ��
            break;
        }
    }
    QueryPerformanceCounter(&end);
    //������������
    closesocket(Connection);
    WSACleanup();

    //for (int i = 0; i < 10; i++)
    //{
    //	std::cout << result[i] << '\t';
    //}
    //std::cout << std::endl;

    std::cout << "Time Consumed 1 : " << (end.QuadPart - start.QuadPart) << std::endl;
    time_consumed1 = end.QuadPart - start.QuadPart;
    std::cout << "�����ͽ�� : " << sum1 << std::endl;

    // ����
    closesocket(Connection);
    WSACleanup();
    delete[] rawFloatData, result, floatSorts;
    return 0;
}
