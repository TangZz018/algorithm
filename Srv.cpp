#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32.lib")
//#include <WinSock2.h>
#include <iostream>
#include <cmath>
#include <omp.h>
#include <windows.h>
#include "immintrin.h"
#include <cuda_runtime.h>
#include "CPUmethods.h"
#include "CUDAmethods.cuh"


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

LARGE_INTEGER start, end;
float* hostInput, * hostOutput;      // host
float* deviceInput, * deviceOutput;  // GPU
extern const int data_len;

int main() {
	float sum1 = 0, sum2 = 0;
	float max1 = 0, max2 = 0;

	float* result = new float[DATANUM];
	float* rawFloatData = new float[DATANUM];
	float* floatSorts = new float[DATANUM/2];
	float* finalresult= new float[DATANUM];

	long long time_consumed1[5], time_consumed2[5];
	long long time_max_consumed1, time_max_consumed2;

	double time1 = 0, time2 = 0;

	for (size_t i = 0; i < DATANUM/2; i++)
	{
		*(floatSorts + i) = 0;
		*(hostInput + i) = 0.0;
	}
	for (size_t i = 0; i < DATANUM; i++)
	{
		*(rawFloatData + i) = float(i + 1);
		*(finalresult + i) = 0;
		*(result + i) = 0;
	}
	SOCKET Coonection;
	Coonection = web_init();
	float hResult = 0.0f;


	//���
	std::cout << "#########################################" << std::endl;
	std::cout << "  SUM : " << std::endl;
	//CUDA
	for (int i = 0; i < 5; i++)
	{
		cudaMalloc((void**)&deviceInput, DATANUM * sizeof(float));
		cudaMalloc((void**)&deviceOutput, sizeof(float));
		cudaMemcpy(deviceInput, hostInput, DATANUM * sizeof(float), cudaMemcpyHostToDevice);

		QueryPerformanceCounter(&start);
		sum1 = sumSpeedUpCUDA();
		if (Coonection == 0) {
			std::cout << "��������ʧ��" << std::endl;
			return -1;
		}
		if (SOCKET_ERROR == recv(Coonection, (char*)&hResult, sizeof(hResult), NULL))
			return WSAGetLastError();
		float final_result_sum = hResult + sum1;
		QueryPerformanceCounter(&end);

		if (i == 0)
			std::cout << "CUDA parallel : " << final_result_sum << std::endl;
		time_consumed1[i] = end.QuadPart - start.QuadPart;
	}
	time1 = arrayAvg(time_consumed1, 5);
	
	//��������
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		sum2 = sum(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);
		time_consumed2[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
			std::cout << "CPU Sequential : " << sum2 << std::endl;
	}
	time2 = arrayAvg(time_consumed2, 5);
	std::cout << "��ͼ��ٱ�" << time2 / time1 << std::endl;

	//���ֵ
	std::cout << "#########################################" << std::endl;
	std::cout << "  MAX : " << std::endl;
	//CUDA
	float final_result;
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		max1 = maxSpeedUpCUDA();
		if (Coonection == 0) {
			std::cout << "��������ʧ��" << std::endl;
			return -1;
		}
		if (SOCKET_ERROR == recv(Coonection, (char*)&max2, sizeof(max2), NULL))
			return WSAGetLastError();
		final_result = (max1 > max2) ? max1 : max2;
		QueryPerformanceCounter(&end);
		time_consumed1[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
			std::cout << "CUDA parallel : " << final_result << std::endl;
	}
	time1 = arrayAvg(time_consumed1, 5);
	
	//��������
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		max1 = floatMax(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);
		time_consumed2[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
			std::cout << "CPU Sequential : " << max1 << std::endl;
	}
	time2 = arrayAvg(time_consumed2, 5);
	std::cout << "���ֵ���ٱ�" << time2 / time1 << std::endl;

	//����
	std::cout << "#########################################" << std::endl;
	std::cout << "  SORT : " << std::endl;
	//CUDA
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		mergesortCUDA(rawFloatData, result, DATANUM / 2);
		const int CHUNK_SIZE = 1024;  // ÿ����Ĵ�СΪ 1024 �ֽ�
		int totalBytesReceived = 0;
		if (Coonection == 0) {
			std::cout << "��������ʧ��" << std::endl;
			return -1;
		}
		int j = 0;
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
		mergeOfTwo(result, floatSorts, finalresult, DATANUM/2);
		QueryPerformanceCounter(&end);
		time_consumed1[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
		{
			std::cout << "CUDA parallel : ";
			test(result, DATANUM);
		}
	}
	time1 = arrayAvg(time_consumed1, 5);
	
	//��������
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		mergesort_cpu(rawFloatData, result, DATANUM);
		QueryPerformanceCounter(&end);
		time_consumed2[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
		{
			std::cout << "CPU Sequential : ";
			test(result, DATANUM);
		}
	}
	time2 = arrayAvg(time_consumed2, 5);
	std::cout << "������ٱ�" << time2 / time1 << std::endl;
	
	

	/*
	QueryPerformanceCounter(&start);
	sum2 = sum(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);
	time_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "���������ͽ�� : " << final_result_sum << std::endl;
	std::cout << "���������ͽ�� : " << sum2 << std::endl;
	std::cout << "��ͼ��ٱ�" << (float)time_consumed2 / time_consumed1 << std::endl;
	

	std::cout << "/************************************************************" << std::endl;
	QueryPerformanceCounter(&start);
	max1 = floatMax(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);
	time_max_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "����������ֵ��� : " << final_result << std::endl;
	std::cout << "����������ֵ : " << max1 << std::endl;
	std::cout << "���ֵ���ٱ�" << (float)time_max_consumed2 / time_max_consumed1 << std::endl;
	std::cout << "/************************************************************" << std::endl;

	QueryPerformanceCounter(&start);
	floatSort(rawFloatData, result, DATANUM); 
	QueryPerformanceCounter(&end);
	cout << "����������" << endl;
	test(finalresult, DATANUM);
	display(finalresult, 10);
	cout << "������������" << endl;
	test(result, DATANUM);
	display(result, 10);
	//std::cout << "������ : " << final_result << std::endl;
	long long sort_time = end.QuadPart - start.QuadPart;
	std::cout << "������ٱ�" << (float)sort_time / speed_sort_time << std::endl;
	*/


	closesocket(Coonection);;
	WSACleanup();

	delete[] rawFloatData;
	delete[] result;
	delete[] floatSorts;
	delete[] finalresult;
	delete[] hostInput;
	std::cout << "Press Enter to exit...";
	std::cin.get();  // �ȴ��û����� Enter ��
	return 0;
}
