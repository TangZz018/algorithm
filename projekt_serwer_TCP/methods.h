#pragma once

#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM * MAX_THREADS)   /*这个数值是总数据量*/
#define VECTOR_SIZE 8

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
void mergeOfTwo(float arr1[], float arr2[], float merged[], const int len);
void mergeSort(float arr[], int left, int right);
void floatSort(const float data[], float result[], const int len);

// 单机加速版本

// 多线程排序
void sortSpeedUpManual(const float data[], const int len, float result[]);

// 多线程求最大值
float maxSpeedUpManual(float data[], const int len);

// 多线程求和
float sumSpeedUpManual(float data[], const int len);

void test(float result[], const int len);   // 测试排序结果是否正确
void display(const float data[], const int len); // 显示排序结果