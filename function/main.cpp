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
	long long time_consumed1[5], time_consumed2[5];
	
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

		QueryPerformanceCounter(&start);

		sum1 = sum(rawFloatData, DATANUM);
		max1 = floatMax(rawFloatData, DATANUM);
		floatSort(rawFloatData, result, DATANUM);

		QueryPerformanceCounter(&end);

		time_consumed1[i] = end.QuadPart - start.QuadPart;
		std::cout << "Time Consumed 1 : " << (end.QuadPart - start.QuadPart) << std::endl;

		std::cout << "输出求和结果 : " << sum1 << std::endl;
		std::cout << "输出最大值 : " << max1 << std::endl;
		//test(result, DATANUM);
		std::cout << "----------------------------------------" << std::endl;
		
		//for (int i = 0; i < 10; i++)
		//{
		//	std::cout << result[i] << '\t';
		//}
		//std::cout << std::endl;


		QueryPerformanceCounter(&start);

		//sum2 = sumSpeedUp(rawFloatData, DATANUM);
		//max2 = maxSpeedUp(rawFloatData, DATANUM);
		
		sum2 = sumSpeedUpManual(rawFloatData, DATANUM);
		max2=maxSpeedUpManual(rawFloatData, DATANUM);
		sortSpeedUp(rawFloatData, DATANUM, result);
		QueryPerformanceCounter(&end);

		time_consumed2[i] = end.QuadPart - start.QuadPart;

		std::cout << "Time Consumed 2 : " << (end.QuadPart - start.QuadPart) << std::endl;
		std::cout << "输出求和结果 : " << sum2 << std::endl;
		std::cout << "输出最大值 : " << max2 << std::endl;
		//test(result, DATANUM);
		std::cout << "****************************************" << std::endl;

		//for (int i = 0; i < 10; i++)
		//{
		//		std::cout << result[i] << '\t';
		//}
		//std::cout << std::endl;
	}
	

	// long long total_time1 = 0, total_time2 = 0;
	// for (int i = 0; i < 5; i++)
	// {
	// 	total_time1 += time_consumed1[i];
	// 	total_time2 += time_consumed2[i];
	// }

	std::cout << std::endl << "加速比 : " << (double)time_consumed1[0] / time_consumed2[0] << std::endl;

	delete[] rawFloatData, result;
	return 0;
}