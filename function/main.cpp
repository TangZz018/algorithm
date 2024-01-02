#include <iostream>
#include <cmath>
#include <omp.h>
#include <windows.h>
#include "immintrin.h"

#include "methods.h"

int main()
{
	float sum1 = 0, sum2 = 0;
	float max1 = 0, max2 = 0;
	LARGE_INTEGER start, end;

	float* result = new float[DATANUM];
	float* rawFloatData = new float[DATANUM];
	//float* temp = new float[DATANUM];
	long long time_consumed1, time_consumed2;
	
	// 浮点数初始化
	for (size_t i = 0; i < DATANUM; i++)
	{
		*(rawFloatData+i) = float(i + 1);
		*(result + i) = 0;
		// *(temp + 1) = 0;
	}
	
	for (int i = 0; i < 1; i++)
	{
		std::cout << "Phase" << i << " : " << std::endl;
		std::cout << "****************************************" << std::endl << std::endl;
		std::cout << "求和" << std::endl;

		QueryPerformanceCounter(&start);
		sum1 = sum(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);

		std::cout << "输出求和结果 : " << sum1 << std::endl;
		time_consumed1 = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 1 : " << time_consumed1 << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		QueryPerformanceCounter(&start);
		sum2 = sumSpeedUpManual(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);

		std::cout << "输出求和结果 : " << sum2 << std::endl;
		time_consumed2 = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 2 : " << time_consumed2 << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		std::cout << "加速比 : " << (double)time_consumed1 / time_consumed2 << std::endl;
		std::cout << "\n****************************************" << std::endl << std::endl;
		std::cout << "求最大值" << std::endl << std::endl;

		QueryPerformanceCounter(&start);
		max1 = floatMax(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);

		std::cout << "输出最大值 : " << max1 << std::endl;
		time_consumed1 = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 1 : " << time_consumed1 << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		QueryPerformanceCounter(&start);
		max2 = maxSpeedUpManual(rawFloatData, DATANUM);
		QueryPerformanceCounter(&end);

		std::cout << "输出最大值 : " << max2 << std::endl;
		time_consumed2 = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 2 : " << time_consumed2 << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		std::cout << "加速比 : " << (double)time_consumed1 / time_consumed2 << std::endl;
		std::cout << "\n****************************************" << std::endl << std::endl;
		std::cout << "排序" << std::endl << std::endl;

		QueryPerformanceCounter(&start);
		floatSort(rawFloatData, result, DATANUM);
		QueryPerformanceCounter(&end);

		test(result, DATANUM);
		display(result, 10);
		time_consumed1 = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 1 : " << time_consumed1 << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		QueryPerformanceCounter(&start);
		sortSpeedUpManual(rawFloatData, DATANUM, result);
		//sortSpeedUp(rawFloatData, DATANUM, result);
		QueryPerformanceCounter(&end);

		test(result, DATANUM);
		display(result, 10);
		time_consumed2 = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 2 : " << time_consumed2 << std::endl;
		std::cout << "----------------------------------------" << std::endl;

		std::cout << "加速比 : " << (double)time_consumed1 / time_consumed2 << std::endl;
		std::cout << "\n****************************************" << std::endl << std::endl;
	}
	delete[] rawFloatData, result;
	return 0;
}