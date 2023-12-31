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



int main() {
	float sum1 = 0, sum2 = 0;
	float max1 = 0, max2 = 0;
	LARGE_INTEGER start, end;
	float* result = new float[DATANUM];
	float* rawFloatData = new float[DATANUM];
	float* floatSorts = new float[DATANUM/2];
	float* finalresult= new float[DATANUM];

	long long time_consumed1, time_consumed2;
	long long time_max_consumed1, time_max_consumed2;
	for (size_t i = 0; i < DATANUM/2; i++)
	{
		
		
		*(floatSorts + i) = 0;

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
	QueryPerformanceCounter(&start);
	sum1 = sumSpeedUpManual(rawFloatData, DATANUM / 2);
	if (Coonection == 0) {
		std::cout << "网络连接失败" << std::endl;
		return -1;
	}
	if (SOCKET_ERROR == recv(Coonection, (char*)&hResult, sizeof(hResult), NULL))
		return WSAGetLastError();
	float final_result_sum = hResult + sum1;
	QueryPerformanceCounter(&end);
	time_consumed1 = end.QuadPart - start.QuadPart;
	

	//最大
	QueryPerformanceCounter(&start);
	max1 = maxSpeedUpManual(rawFloatData, DATANUM / 2);
	if (Coonection == 0) {
		std::cout << "网络连接失败" << std::endl;
		return -1;
	}
	if (SOCKET_ERROR == recv(Coonection, (char*)&max2, sizeof(max2), NULL))
		return WSAGetLastError();
	float final_result = (max1 > max2) ? max1 : max2;
	QueryPerformanceCounter(&end);
	time_max_consumed1 = end.QuadPart - start.QuadPart;

	//排序
	
	QueryPerformanceCounter(&start);
	sortSpeedUpManual(rawFloatData, DATANUM / 2, result);
	const int CHUNK_SIZE = 1024;  // 每个块的大小为 1024 字节
	int totalBytesReceived = 0;
	if (Coonection == 0) {
		std::cout << "网络连接失败" << std::endl;
		return -1;
	}
	int i = 0;
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
	long long speed_sort_time = end.QuadPart - start.QuadPart;
	closesocket(Coonection);;
	WSACleanup();

	/**********************************************************************************************************/
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

	delete[] rawFloatData, result, floatSorts,finalresult;
	std::cout << "Press Enter to exit...";
	std::cin.get();  // 等待用户按下 Enter 键
	return 0;
}
