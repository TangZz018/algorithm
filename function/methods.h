#pragma once

#define MAX_THREADS 64
#define SUBDATANUM 1000000
#define DATANUM (SUBDATANUM * MAX_THREADS)   /*这个数值是总数据量*/

struct ThreadData {
	float max;
	float* data;
	int len;
	float sum;
};

// 未加速版本
float sum(const float data[], const int len);
float floatMax(const float data[], const int len);

void merge(float arr[], int left, int mid, int right);
void mergeSort(float arr[], int left, int right);
void floatSort(const float data[], float result[], const int len);

// 单机加速版本
float sumSpeedUp(const float data[], const int len);
float maxSpeedUp(const float data[], const int len);

// 多线程排序
DWORD WINAPI sortSpeedUpThread(LPVOID lpParam);
void mergeResults(float result[], int chunkSize, int threadCount);
void sortSpeedUp(const float data[], const int len, float result[]);

// 多线程求最大值
float horizontal_max(__m256 max_sse);
DWORD WINAPI maxSpeedUpThread(LPVOID lpParameter);
float maxSpeedUpManual(float data[], const int len);

// 多线程求和
float horizontal_sum(__m256 sum_sse);
DWORD WINAPI sumSpeedUpThread(LPVOID lpParameter);
float sumSpeedUpManual(float data[], const int len);

void test(float result[], const int len);   // 测试排序结果是否正确