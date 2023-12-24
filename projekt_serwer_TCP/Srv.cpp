#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <omp.h>
#include <windows.h>
#include "immintrin.h" // avx
#pragma comment(lib,"ws2_32.lib")
//#include <WinSock2.h>
#include <iostream>
#include <string>

#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM * MAX_THREADS)   

// �����ٰ汾

// ���ٰ汾
float floatSum(const float data[], const int len) //data��ԭʼ���ݣ�lenΪ���ȡ����ͨ����������
{
	float sum = 0.0;   // ׼���ۼ�����
	float c = 0.0;     // ����׷�ٵ�λ��ʧ�ı��ص����в�����

	for (int i = 0; i < len; i++) {
		float y = log(sqrt(data[i])) - c;  // ���� y����ȥ���в�����
		float t = sum + y;                  // �����м�͡�
		c = (t - sum) - y;                  // �������в�����
		sum = t;   // �����ۼ�����

	}
	//for (int i = 0; i < len; i++) // ���ݳ�ʼ��
	//{
	//    sum += 1.556f;
	//}
	return sum;
}

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
	float sum1 = 0, sum2 = 0;
	float max1 = 0, max2 = 0;
	LARGE_INTEGER start, end;
	float* result = new float[DATANUM];
	float* rawFloatData = new float[DATANUM];
	float* floatSorts = new float[DATANUM];

	long long time_consumed1, time_consumed2;

	for (size_t i = 0; i < DATANUM; i++)
	{
		*(rawFloatData + i) = float(i + 1);
		*(result + i) = 0;
		*(floatSorts + i) = 0;
	}
	//��������
	WSAData wsaData;
	WORD DllVersion = MAKEWORD(2, 1);
	if (WSAStartup(DllVersion, &wsaData) != 0) {
		MessageBoxA(NULL, "WinSock startup error", "Error", MB_OK | MB_ICONERROR);
		return 0;
	}

	SOCKADDR_IN addr;
	int addrlen = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8081);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//addr.sin_addr.s_addr = inet_addr("192.168.43.250");


	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);//�Ӵ��� 
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));

	listen(sListen, SOMAXCONN);

	char L1;
	float L3;

	SOCKET newConnection;
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);
	//������������

	//��ʼ����
	QueryPerformanceCounter(&start);
	sum1 = sumSpeedUp1(rawFloatData, DATANUM/2);
	if (newConnection == 0) {
		std::cout << "Bad connection." << std::endl;
	}
	else {
		std::string strData;
		L1 = '1';

		while (L1 != '0') {
			std::cout << "Waiting for string from client..." << std::endl;

			// �����ַ���
			char buffer[50];
			int bytesReceived = recv(newConnection, (char*)buffer, sizeof(buffer), 0);
			std::cout << buffer << std::endl;
			if (bytesReceived > 0) {
				buffer[bytesReceived] = '\0';  // ȷ���ַ����� null ��β
				strData = buffer;

				std::cout << "Received string from client: " << strData << std::endl;

				try {
					// ���Խ��ַ���ת��Ϊ double
					L3 = std::stof(strData);

					// ��ӡת����� double ֵ
					std::cout << "Converted double value: " << L3 << std::endl;
				}
				catch (const std::invalid_argument&) {
					std::cerr << "Invalid double value received from client." << std::endl;
				}
				catch (const std::out_of_range&) {
					std::cerr << "Double value out of range." << std::endl;
				}
			}

			if (L3 > 0) {
				L1 = '1';
				send(newConnection, (char*)&L1, sizeof(L1), NULL);
				break;
			}

		}
    }
	float final_result = L3 + sum1;
	//return 0;

	//max1 = find_max(rawFloatData, DATANUM);
	//floatSort(rawFloatData, result, DATANUM);
	QueryPerformanceCounter(&end);
	//������������
	closesocket(newConnection);
	closesocket(sListen);
	WSACleanup();
	//for (int i = 0; i < 10; i++)
	//{
	//	std::cout << result[i] << '\t';
	//}
	//std::cout << std::endl;

	std::cout << "Time Consumed 1 : " << (end.QuadPart - start.QuadPart) << std::endl;
	time_consumed1 = end.QuadPart - start.QuadPart;
	std::cout << "�����ͽ�� : " << final_result << std::endl;

	QueryPerformanceCounter(&start);
	sum2 = floatSum(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);
	std::cout << "Time Consumed 2 : " << (end.QuadPart - start.QuadPart) << std::endl;
	time_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "�����ͽ�� : " << sum2 << std::endl;
	std::cout << "���ٱ�" << (float)time_consumed2 / time_consumed1 << std::endl;
	delete[] rawFloatData, result, floatSorts;
	std::cout << "Press Enter to exit...";
	std::cin.get();  // �ȴ��û����� Enter ��
    return 0;
}
