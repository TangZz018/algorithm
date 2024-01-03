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
	//网络连接
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


	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);//接待处 
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));

	listen(sListen, SOMAXCONN);
	SOCKET newConnection;
	std::cout << "等待连接" << std::endl;
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


	//求和
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
			std::cout << "网络连接失败" << std::endl;
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
	
	//单机串行
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
	std::cout << "求和加速比" << time2 / time1 << std::endl;

	//最大值
	std::cout << "#########################################" << std::endl;
	std::cout << "  MAX : " << std::endl;
	//CUDA
	float final_result;
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		max1 = maxSpeedUpCUDA();
		if (Coonection == 0) {
			std::cout << "网络连接失败" << std::endl;
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
	
	//单机串行
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
	std::cout << "最大值加速比" << time2 / time1 << std::endl;

	//排序
	std::cout << "#########################################" << std::endl;
	std::cout << "  SORT : " << std::endl;
	//CUDA
	for (int i = 0; i < 5; i++)
	{
		QueryPerformanceCounter(&start);
		mergesortCUDA(rawFloatData, result, DATANUM / 2);
		const int CHUNK_SIZE = 1024;  // 每个块的大小为 1024 字节
		int totalBytesReceived = 0;
		if (Coonection == 0) {
			std::cout << "网络连接失败" << std::endl;
			return -1;
		}
		int j = 0;
		while (totalBytesReceived < DATANUM / 2 * sizeof(float)) {
			int remainingBytes = DATANUM / 2 * sizeof(float) - totalBytesReceived;
			int chunkSize = min(CHUNK_SIZE, remainingBytes);

			// 接收数据
			int bytesReceived = recv(Coonection, (char*)&result[totalBytesReceived / sizeof(float)], chunkSize, NULL);

			if (bytesReceived <= 0) {
				// 处理接收错误或连接关闭的情况
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
	
	//单机串行
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
	std::cout << "排序加速比" << time2 / time1 << std::endl;
	
	

	/*
	QueryPerformanceCounter(&start);
	sum2 = sum(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);
	time_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "输出加速求和结果 : " << final_result_sum << std::endl;
	std::cout << "输出串行求和结果 : " << sum2 << std::endl;
	std::cout << "求和加速比" << (float)time_consumed2 / time_consumed1 << std::endl;
	

	std::cout << "/************************************************************" << std::endl;
	QueryPerformanceCounter(&start);
	max1 = floatMax(rawFloatData, DATANUM);
	QueryPerformanceCounter(&end);
	time_max_consumed2 = end.QuadPart - start.QuadPart;
	std::cout << "输出加速最大值结果 : " << final_result << std::endl;
	std::cout << "输出串行最大值 : " << max1 << std::endl;
	std::cout << "最大值加速比" << (float)time_max_consumed2 / time_max_consumed1 << std::endl;
	std::cout << "/************************************************************" << std::endl;

	QueryPerformanceCounter(&start);
	floatSort(rawFloatData, result, DATANUM); 
	QueryPerformanceCounter(&end);
	cout << "加速排序结果" << endl;
	test(finalresult, DATANUM);
	display(finalresult, 10);
	cout << "不加速排序结果" << endl;
	test(result, DATANUM);
	display(result, 10);
	//std::cout << "输出结果 : " << final_result << std::endl;
	long long sort_time = end.QuadPart - start.QuadPart;
	std::cout << "排序加速比" << (float)sort_time / speed_sort_time << std::endl;
	*/


	closesocket(Coonection);;
	WSACleanup();

	delete[] rawFloatData;
	delete[] result;
	delete[] floatSorts;
	delete[] finalresult;
	delete[] hostInput;
	std::cout << "Press Enter to exit...";
	std::cin.get();  // 等待用户按下 Enter 键
	return 0;
}
