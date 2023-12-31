#define _WINSOCK_DEPRECATED_NO_WARNINGS


#pragma comment(lib,"ws2_32.lib")
//#include <WinSock2.h>
#include <iostream>
#include <cmath>
#include <omp.h>
#include <windows.h>
#include "immintrin.h"
#include "methods.h"

using namespace std;
SOCKET web_init()
{
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
	addr.sin_port = htons(8083);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//addr.sin_addr.s_addr = inet_addr("192.168.43.250");


	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);//�Ӵ��� 
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));

	listen(sListen, SOMAXCONN);
	SOCKET newConnection;
	std::cout << "�ȴ�����" << std::endl;
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);
	return newConnection;
}

void mergeOfTwo(float arr1[], float arr2[], float merged[], const int len);

int main() {
	float sum1 = 0, sum2 = 0;
	float max1 = 0, max2 = 0;
	LARGE_INTEGER start, end;
	float* result = new float[DATANUM/2];
	float* rawFloatData = new float[DATANUM/2];
	float* floatSorts = new float[DATANUM/2];
	float* finalresult= new float[DATANUM];

	long long time_consumed1, time_consumed2;
	long long time_max_consumed1, time_max_consumed2;
	for (size_t i = 0; i < DATANUM/2; i++)
	{
		*(rawFloatData + i) = float(i + 1);
		*(result + i) = 0;
		*(floatSorts + i) = 0;

	}
	for (size_t i = 0; i < DATANUM; i++)
	{
		
		*(finalresult + i) = 0;
	}
	SOCKET Coonection;
	Coonection = web_init();
	float hResult = 0.0f;
	QueryPerformanceCounter(&start);

	sum1 = sumSpeedUpManual(rawFloatData, DATANUM / 2);

	if (Coonection == 0) {
		std::cout << "��������ʧ��" << std::endl;
		return -1;
	}

		while (true) {

			if (SOCKET_ERROR == recv(Coonection, (char*)&hResult, sizeof(hResult), NULL))
				return WSAGetLastError();
			else
				break;


		}
		
		

	float final_result_sum = hResult + sum1;

	QueryPerformanceCounter(&end);
	//��ʼ����
	std::cout << "Time Consumed 1 : " << (end.QuadPart - start.QuadPart) << std::endl;
	time_consumed1 = end.QuadPart - start.QuadPart;
	std::cout << "�����ͽ�� : " << final_result_sum << std::endl;
	QueryPerformanceCounter(&start);

	max1 = maxSpeedUpManual(rawFloatData, DATANUM / 2);
	if (Coonection == 0) {
		std::cout << "��������ʧ��" << std::endl;
		return -1;
	}

		
		while (true) {

			if (SOCKET_ERROR == recv(Coonection, (char*)&max2, sizeof(max2), NULL))
				return WSAGetLastError();
			else
				break;
		}

	float final_result = (max1 > max2) ? max1 : max2;

	QueryPerformanceCounter(&end);
	//������������
	QueryPerformanceCounter(&start);
	sortSpeedUpManual(rawFloatData, DATANUM / 2, result);
	const int CHUNK_SIZE = 1024;  // ÿ����Ĵ�СΪ 1024 �ֽ�

	int totalBytesReceived = 0;
	if (Coonection == 0) {
		std::cout << "��������ʧ��" << std::endl;
		return -1;
	}

	int i = 0;
	while (totalBytesReceived < DATANUM / 2 * sizeof(float)) {
		int remainingBytes = DATANUM / 2 * sizeof(float) - totalBytesReceived;
		int chunkSize = min(CHUNK_SIZE, remainingBytes);

		// ��������
		int bytesReceived = recv(Coonection, (char*)&result[totalBytesReceived / sizeof(float)], chunkSize, NULL);

		if (bytesReceived <= 0) {
			// ������մ�������ӹرյ����
			break;
		}

		totalBytesReceived += bytesReceived;
	}

	floatSorts = (float*)result;

	mergeOfTwo(result, floatSorts, finalresult, DATANUM);
	QueryPerformanceCounter(&end);
	long long speed_sort_time = end.QuadPart - start.QuadPart;
	//std::cout << "�����ʱ�ļ���ʱ��Ϊ������������������������������" << (end.QuadPart - start.QuadPart) << std::endl;

	closesocket(Coonection);;
	WSACleanup();

	time_max_consumed1 = end.QuadPart - start.QuadPart;




	/**********************************************************************************************************/
	QueryPerformanceCounter(&start);
	sum2 = sum(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);

	std::cout << "Time add Consumed 2 : " << (end.QuadPart - start.QuadPart) << std::endl;
	time_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "�����ͽ�� : " << sum2 << std::endl;
	std::cout << "���ٱ�" << (float)time_consumed2 / time_consumed1 << std::endl;


	std::cout << "/************************************************************" << std::endl;
	QueryPerformanceCounter(&start);
	max1 = floatMax(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);
	std::cout << "Time Consumed 1 : " << time_max_consumed1 << std::endl;
	std::cout << "������ : " << final_result << std::endl;
	std::cout << "Time max Consumed 2 : " << (end.QuadPart - start.QuadPart) << std::endl;
	time_max_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "���ֵ : " << max1 << std::endl;
	std::cout << "���ٱ�" << (float)time_max_consumed2 / time_max_consumed1 << std::endl;
	delete[] rawFloatData, result, floatSorts;
	std::cout << "Press Enter to exit...";
	std::cin.get();  // �ȴ��û����� Enter ��
	return 0;
}
