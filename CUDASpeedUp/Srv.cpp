#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib,"ws2_32.lib")
//#include <WinSock2.h>
#include <iostream>
#include <cmath>
#include <windows.h>
#include <chrono>
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
	addr.sin_port = htons(8082);
	//addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_addr.s_addr = inet_addr("169.254.112.212");
	//addr.sin_addr.s_addr = inet_addr("192.168.43.250");

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);//�Ӵ��� 
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));

	listen(sListen, SOMAXCONN);
	SOCKET newConnection;
	std::cout << "�ȴ�����" << std::endl;
	newConnection = accept(sListen, (SOCKADDR*)&addr, &addrlen);
	return newConnection;
}

float* hostInput, * hostOutput;      // host
float* deviceInput, * deviceOutput;  // GPU
extern const int data_len;

int main() {
	float sum1 = 0, sum2 = 0;
	float max1 = 0, max2 = 0;

	float* result = new float[DATANUM];
	float* rawFloatData = new float[DATANUM];
	hostInput = new float[DATANUM/2];
	
	LARGE_INTEGER start,end;
	long long timeConsumed1[5], timeConsumed2[5];

	double time1 = 0, time2 = 0;

	for (size_t i = 0; i < DATANUM/2; i++)
	{
		//*(floatSorts + i) = 0;
		*(hostInput + i) = float(i + 1);
	}
	for (size_t i = 0; i < DATANUM; i++)
	{
		*(rawFloatData + i) = float(i + 1);
		//*(finalresult + i) = 0;
		*(result + i) = 0;
	}
	SOCKET Connection;
	Connection = web_init();
	if (Connection == 0) {
		std::cout << "��������ʧ��" << std::endl;
		return -1;
	}
	float hResult = 0.0f;


	//���
	std::cout << "#########################################" << std::endl;
	std::cout << "  SUM : " << std::endl;
	for (int i = 0; i < 5; i++)
	{
		cudaMalloc((void**)&deviceInput, DATANUM/2 * sizeof(float));
		cudaMalloc((void**)&deviceOutput, sizeof(float));
		cudaMemcpy(deviceInput, hostInput, DATANUM/2 * sizeof(float), cudaMemcpyHostToDevice);

		QueryPerformanceCounter(&start);
		sum1 = sumSpeedUpCUDA();
		QueryPerformanceCounter(&end);

		if (SOCKET_ERROR == recv(Connection, (char*)&hResult, sizeof(hResult), NULL))
			return WSAGetLastError();
		float final_result_sum = hResult + sum1;

		if (i == 0)
			std::cout << "CUDA parallel : " << final_result_sum << std::endl;
		
		timeConsumed1[i] = end.QuadPart - start.QuadPart;
		cudaFree(deviceInput);
		cudaFree(deviceOutput);
		syncAndCheckCUDAError();
	}
	time1 = arrayAvg(timeConsumed1, 5);
	std::cout << "Time consumed : " << time1 << std::endl;
	
	//��������
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		sum2 = floatSum(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);

		timeConsumed2[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
			std::cout << "CPU Sequential : " << sum2 << std::endl;
	}
	time2 = arrayAvg(timeConsumed2, 5);
	std::cout << "Time consumed : " << time2 << std::endl;

	std::cout << "��ͼ��ٱ�" << time2 / time1 << std::endl;

	//���ֵ
	std::cout << "#########################################" << std::endl;
	std::cout << "  MAX : " << std::endl;
	//CUDA
	float final_result;
	for (int i = 0; i < 5; i++)
	{
		cudaMalloc((void**)&deviceInput, DATANUM / 2 * sizeof(float));
		cudaMalloc((void**)&deviceOutput, sizeof(float));
		cudaMemcpy(deviceInput, hostInput, DATANUM / 2 * sizeof(float), cudaMemcpyHostToDevice);

		QueryPerformanceCounter(&start);
		max1 = maxSpeedUpCUDA();
		if (Connection == 0) {
			std::cout << "��������ʧ��" << std::endl;
			return -1;
		}
		if (SOCKET_ERROR == recv(Connection, (char*)&max2, sizeof(max2), NULL))
			return WSAGetLastError();

		final_result = (max1 > max2) ? max1 : max2;
		QueryPerformanceCounter(&end);

		timeConsumed1[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
			std::cout << "CUDA parallel : " << final_result << std::endl;

		cudaFree(deviceInput);
		cudaFree(deviceOutput);
		syncAndCheckCUDAError();
	}
	time1 = arrayAvg(timeConsumed1, 5);
	std::cout << "Time consumed : " << time1 << std::endl;
	
	//��������
	for (int i = 0; i < 5; i++)    
	{
		QueryPerformanceCounter(&start);
		max1 = floatMax(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);
		timeConsumed2[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
			std::cout << "CPU Sequential : " << max1 << std::endl;
	}
	time2 = arrayAvg(timeConsumed2, 5);
	std::cout << "Time consumed : " << time2 << std::endl;

	std::cout << "���ֵ���ٱ�" << time2 / time1 << std::endl;



	//����
	std::cout << "#########################################" << std::endl;
	std::cout << "  SORT : " << std::endl;
	
	//CUDA
	const int CHUNK_SIZE = 4096;                         // ÿ����Ĵ�СΪ 2048 �ֽ�
	const int DATA_SIZE = DATANUM / 2 * sizeof(float);   // һ�����յ�������
	float* finalResult = new float[DATANUM];
	float* resultSorts = new float[DATANUM / 2];
	char* buffer = new char[DATA_SIZE];
	for (int i = 0; i < 5; i++)
	{
		//float* floatSorts = new float[DATANUM / 2];

		
		cudaMalloc((void**)&deviceInput, DATANUM / 2 * sizeof(float));
		cudaMalloc((void**)&deviceOutput, DATANUM / 2 * sizeof(float));

		QueryPerformanceCounter(&start);
		mergesortCUDA(rawFloatData, resultSorts, DATANUM / 2);

		int totalBytesReceived = 0;

		while (totalBytesReceived < DATA_SIZE) {
			int remainingBytes = DATA_SIZE - totalBytesReceived;
			int chunkSize =min(CHUNK_SIZE, remainingBytes);

			// ��������
			int bytesReceived = recv(Connection, &buffer[totalBytesReceived], chunkSize, 0);

			if (bytesReceived <= 0) {
				// ������մ�������ӹرյ����
				// �������������Ϣ������������������
				break;
			}

			totalBytesReceived += bytesReceived;
		}

		// �����յ�������ת��Ϊ float ����
		float* receivedData = reinterpret_cast<float*>(buffer);

		// ������յ�������
		mergeOfTwo(resultSorts, (float*)receivedData, finalResult, DATANUM / 2);
		QueryPerformanceCounter(&end);
		timeConsumed1[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
		{
			std::cout << "CUDA parallel : " << std::endl;
			test(finalResult, DATANUM);
		}
	}
	delete[] buffer;
	delete[] finalResult;
	delete[] resultSorts;
	time1 = arrayAvg(timeConsumed1, 5);
	std::cout << "Time consumed : " << time1 << std::endl;

	//��������
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		mergesortCpu(rawFloatData, result, DATANUM);
		QueryPerformanceCounter(&end);

		timeConsumed2[i] = end.QuadPart - start.QuadPart;
		if (i == 0)
		{
			std::cout << "\nCPU Sequential : ";
			test(result, DATANUM);
		}
	}
	time2 = arrayAvg(timeConsumed2, 5);
	std::cout << "Time consumed : " << time2 << std::endl;

	std::cout << "������ٱ�" << time2 / time1 << std::endl;
	

	closesocket(Connection);;
	WSACleanup();

	delete[] rawFloatData;
	delete[] result;
	delete[] hostInput;
	std::cout << "Press Enter to exit...";
	std::cin.get();  // �ȴ��û����� Enter ��
	return 0;
}
