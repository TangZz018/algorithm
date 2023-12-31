#pragma once

#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM * MAX_THREADS)   /*�����ֵ����������*/
#define VECTOR_SIZE 8

struct ThreadData {
	float max;
	float* data;
	int len;
	float sum;
};

// δ���ٰ汾
float sum(const float data[], const int len);
float floatMax(const float data[], const int len);

void merge(float arr[], int left, int mid, int right);
void mergeOfTwo(float arr1[], float arr2[], float merged[], const int len);
void mergeSort(float arr[], int left, int right);
void floatSort(const float data[], float result[], const int len);

// �������ٰ汾

// ���߳�����
void sortSpeedUpManual(const float data[], const int len, float result[]);

// ���߳������ֵ
float maxSpeedUpManual(float data[], const int len);

// ���߳����
float sumSpeedUpManual(float data[], const int len);

void test(float result[], const int len);   // �����������Ƿ���ȷ
void display(const float data[], const int len); // ��ʾ������