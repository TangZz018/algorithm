#include <iostream>
#include <cmath>
#include <omp.h>
#include <windows.h>
#include <immintrin.h>
#include "methods.h"

//δ���ٰ汾
float sum(const float data[], const int len)
{
	float sum = 0.0;   // ׼���ۼ�����
	float c = 0.0;     // ����׷�ٵ�λ��ʧ�ı��ص����в�����

	for (int i = 0; i < len; i++) {
		float y = log(sqrt(data[i])) - c;  // ���� y����ȥ���в�����
		float t = sum + y;                  // �����м�͡�
		c = (t - sum) - y;                  // �������в�����
		sum = t;   // �����ۼ�����

	}

	return sum;
}

float floatMax(const float data[], const int len)
{
	float max = 1;
	int max_id = 0;
	float sqrt_max = sqrt(max);
	float log_sqrt_max = log(sqrt_max);
	for (int i = 0; i < len; i++)
	{
		float sqrt_data = sqrt(data[i]);
		float log_sqrt_data = log(sqrt_data);

		if (log_sqrt_data > log_sqrt_max)
		{
			max_id = i;
			max = data[i];
			sqrt_max = sqrt_data;
			log_sqrt_max = log_sqrt_data;
		}
	}
	return log(sqrt(max));
}

void merge(float arr[], int left, int mid, int right) {
	int i = left;
	int j = mid + 1;
	int k = 0;
	int tempSize = right - left + 1;
	float* temp = new float[tempSize];

	// �Ӵ�С����
	for (int p = 0; p < tempSize; p++) {
		if (i <= mid && (j > right || arr[i] >= arr[j])) {
			temp[p] = arr[i];
			i++;
		}
		else {
			temp[p] = arr[j];
			j++;
		}
	}

	for (int p = 0; p < tempSize; p++) {
		arr[left + p] = temp[p];
	}

	delete[] temp;
}

void mergeSort(float arr[], int left, int right) {
	if (left < right) {
		int mid = left + (right - left) / 2;
		mergeSort(arr, left, mid);
		mergeSort(arr, mid + 1, right);
		merge(arr, left, mid, right);
	}
}

void floatSort(const float data[], float result[], const int len) {
	for (int i = 0; i < len; i++) {
		result[i] = log(sqrt(data[i]));
	}
	mergeSort(result, 0, len - 1);
}

// �������ٰ汾
float sumSpeedUp(const float data[], const int len) {
	float result = 0;
	int per_iter = len / 8;

	__m256 sum = _mm256_setzero_ps();
	__m256 c = _mm256_setzero_ps();
	__m256* ptr = (__m256*)data;

#pragma omp parallel for
	for (int i = 0; i < per_iter; ++i) {
		// ���� y����ȥ���в�����
		__m256 right = _mm256_log_ps(_mm256_sqrt_ps(ptr[i]));
		__m256 y = _mm256_sub_ps(right, c);

		// �����м�͡�
		__m256 t = _mm256_add_ps(sum, y);

		// �������в�����
		c = _mm256_sub_ps(_mm256_sub_ps(t, sum), y);
#pragma omp critical
		// �����ۼ�����
		sum = t;
	}

	float* resultArray = (float*)&sum;
	result = resultArray[0] + resultArray[1] + resultArray[2] + resultArray[3] +
		resultArray[4] + resultArray[5] + resultArray[6] + resultArray[7];
	
	return result;
}

float maxSpeedUp(const float data[], const int len) {
	int per_iter = len / 8;
	float result_max = 0;
	__m256 new_max = _mm256_setzero_ps();
	__m256* ptr = (__m256*)data;
#pragma omp parallel
	{
		__m256 max = _mm256_setzero_ps();
#pragma omp  for
		for (int i = 0; i < per_iter; ++i) {
			__m256 right = _mm256_log_ps(_mm256_sqrt_ps(ptr[i]));
			max = _mm256_max_ps(max, right);
		}
#pragma omp critical
		new_max = _mm256_max_ps(max, new_max);
	}
	float* resultArray = (float*)&new_max;
	for (int i = 1; i < 8; i++) {
		if (result_max < resultArray[i])
		{
			result_max = resultArray[i];
		}
	}
	return result_max;
}

DWORD WINAPI sortSpeedUpThread(LPVOID lpParam) {
	ThreadData* threadData = static_cast<ThreadData*>(lpParam);
	mergeSort(threadData->data, 0, threadData->len - 1);
	return 0;
}

void mergeResults(float result[], int chunkSize, int threadCount) {
	float* temp = new float[chunkSize * threadCount];
	float* merged = new float[chunkSize * threadCount];
	int mergeStep = chunkSize;

	memcpy(temp, result, chunkSize * threadCount * sizeof(float));

	while (mergeStep < chunkSize * threadCount) {
		for (int i = 0; i < chunkSize * threadCount; i += mergeStep * 2) {
			int left = i;
			int mid = i + mergeStep - 1;
			int right = i + mergeStep * 2 - 1;

			if (right >= chunkSize * threadCount) {
				right = chunkSize * threadCount - 1;
			}

			merge(temp, left, mid, right);
		}
		mergeStep *= 2;
	}

	memcpy(merged, temp, chunkSize * threadCount * sizeof(float));
	delete[] temp;

	memcpy(result, merged, chunkSize * threadCount * sizeof(float));
	delete[] merged;
}


void sortSpeedUpManual(const float data[], const int len, float result[]) {
#pragma omp parallel for
	for (int i = 0; i < len; i += 8) {
		// result[i] = log(sqrt(data[i]));
		__m256 temp = _mm256_log_ps(_mm256_sqrt_ps(_mm256_load_ps(&data[i])));
		_mm256_store_ps(&result[i], temp);
	}

	int chunkSize = len / MAX_THREADS;
	ThreadData thread_data[MAX_THREADS];
	HANDLE hThreads[MAX_THREADS];

	for (int i = 0; i < MAX_THREADS; i++) {
		thread_data[i].data = const_cast<float*>(&result[chunkSize * i]);
		thread_data[i].len = chunkSize;
		thread_data[i].max = 0;

		hThreads[i] = CreateThread(
			NULL,              // Ĭ�ϰ�ȫ����
			0,                 // ʹ��Ĭ�϶�ջ��С
			sortSpeedUpThread,  // �̺߳���
			&thread_data[i],   // ���ݸ��̺߳����Ĳ���
			0,                 // ʹ��Ĭ�ϴ�����־��0��ʾ�߳���������
			NULL               // ����ȡ�̱߳�ʶ��
		);
		ResumeThread(hThreads[i]);
	}

	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
#pragma omp parallel for
	for (int i = 0; i < MAX_THREADS; i++) {
		CloseHandle(hThreads[i]);
	}

	// ���ܸ��̵߳�������
	mergeResults(result, chunkSize, MAX_THREADS);
}


// ��sse256�Ĵ�����8���������ֵ
float horizontal_max(__m256 max_sse) {
	float max8[8];
	_mm256_store_ps(max8, max_sse);
	float max1 = max8[0];
	for (int i = 1; i < 8; i++) {
		if (max1 < max8[i]) {
			max1 = max8[i];
		}
	}
	return max1;
}

// ��sse256�Ĵ�����8�����ĺ�
float horizontal_sum(__m256 sum_sse) {
	float sum8[8];
	float sum1 = 0;
	_mm256_store_ps(sum8, sum_sse);
	for (int i = 0; i < 8; i++) sum1 += sum8[i];
	return sum1;
}

DWORD WINAPI maxSpeedUpThread(LPVOID lpParameter) {
	ThreadData* thread_data = static_cast<ThreadData*>(lpParameter);

	int nb_iters = thread_data->len / 8;
	__m256 max = _mm256_setzero_ps();
	const __m256* ptr = reinterpret_cast<const __m256*>(thread_data->data);
	for (int i = 0; i < nb_iters; ++i, ++ptr) {
		__m256 right = _mm256_log_ps(_mm256_sqrt_ps(*ptr));
		max = _mm256_max_ps(max, right);
	}

	thread_data->max = horizontal_max(max);
	return 0;
}

float maxSpeedUpManual(float data[], const int len) {
	HANDLE hThreads[MAX_THREADS];
	ThreadData thread_data[MAX_THREADS];
	for (int i = 0; i < MAX_THREADS; i++) {
		thread_data[i].data = &data[len / MAX_THREADS * i];
		thread_data[i].len = len / MAX_THREADS;
		hThreads[i] =
			CreateThread(NULL,              // default security attributes
				0,                 // use default stack size
				maxSpeedUpThread,  // thread function
				&thread_data[i],   // argument to thread function
				0,  // use default creation flags.0 means the thread will
				// be run at once  CREATE_SUSPENDED
				NULL);
		ResumeThread(hThreads[i]);
	}

	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
	float max = 0;
	for (int i = 0; i < MAX_THREADS; i++) {
		if (max < thread_data[i].max) {
			max = thread_data[i].max;
		}
		CloseHandle(hThreads[i]);
	}
	return max;
}

DWORD WINAPI sumSpeedUpThread(LPVOID lpParameter) {
	ThreadData* thread_data = static_cast<ThreadData*>(lpParameter);

	int nb_iters = thread_data->len / 8;
	__m256 sum = _mm256_setzero_ps();
	__m256 c = _mm256_setzero_ps();
	__m256 y = _mm256_setzero_ps();
	const __m256* ptr = reinterpret_cast<const __m256*>(thread_data->data);
	for (int i = 0; i < nb_iters; ++i, ++ptr) {
		__m256 right = _mm256_log_ps(_mm256_sqrt_ps(*ptr));
		__m256 y = _mm256_sub_ps(right, c);

		// �����м�͡�
		__m256 t = _mm256_add_ps(sum, y);

		// �������в�����
		c = _mm256_sub_ps(_mm256_sub_ps(t, sum), y);
		sum = t;
	}


	float* resultArray = (float*)&sum;
	thread_data->sum = resultArray[0] + resultArray[1] + resultArray[2] + resultArray[3] +
		resultArray[4] + resultArray[5] + resultArray[6] + resultArray[7];

	return 0;
}

float sumSpeedUpManual(float data[], const int len) {
	HANDLE hThreads[MAX_THREADS];
	ThreadData thread_data[MAX_THREADS];

	for (int i = 0; i < MAX_THREADS; i++) {
		thread_data[i].data = &data[len / MAX_THREADS * i];
		thread_data[i].len = len / MAX_THREADS;
		hThreads[i] =
			CreateThread(NULL,              // default security attributes
				0,                 // use default stack size
				sumSpeedUpThread,  // thread function
				&thread_data[i],   // argument to thread function
				0,  // use default creation flags.0 means the thread will
				// be run at once  CREATE_SUSPENDED
				NULL);
		ResumeThread(hThreads[i]);
	}

	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
	float sum = 0;
	for (int i = 0; i < MAX_THREADS; i++) {
		sum += thread_data[i].sum;
		CloseHandle(hThreads[i]);
	}
	return sum;
}

// �����������Ƿ���ȷ(��С����)
void test(float result[], const int len)
{
	for (int i = 0; i < len - 1; i++)
	{
		if (result[i] < result[i + 1])
		{
			std::cout << "����������" << std::endl;
			return;
		}
	}
	std::cout << "��������ȷ" << std::endl;
	return;
}

void display(const float data[], const int len)
{
	for (int i = 0; i < len; i++)
	{
		std::cout << data[i] << '\t';
	}
}



// ������˫�������ʵ�ֺ���

// ��bitonicSort�е�˳���֪�����ﴫ���arr����˫������
void bitonicMerge(float* data, int len, bool asd) {
	if (len > 1) {
		int m = len / 2;
		for (int i = 0; i < m; ++i) {
			if (data[i] > data[i + m])
				std::swap(data[i], data[i + m]); // ����asd�ж��Ƿ񽻻�
		}
		// forѭ����������������2��˫�����У��ֱ�mergeֱ�����г���Ϊ1
		bitonicMerge(data, m, asd); // ���ǰ���asd����merge
		bitonicMerge(data + m, m, asd);
	}
}

void bitonicSort(float* arr, int len, bool asd) { // asd ����
	if (len > 1) {
		int m = len / 2;
		bitonicSort(arr, m, !asd); // ǰ�뽵��
		bitonicSort(arr + m, len - m, asd); // �������
		// ǰ2��sort֮���γ���1��˫�����У�Ȼ����merge�ϲ���asd�涨������
		bitonicMerge(arr, len, asd); // �ϲ�
	}
}


void bitonicMergeOMP(float* arr, int len, bool asd) {
	if (len > 1) {
		int m = len / 2;
#pragma omp parallel for
		for (int i = 0; i < m; ++i) {
			if (arr[i] > arr[i + m])
				std::swap(arr[i], arr[i + m]);
		}
#pragma omp task
		bitonicMergeOMP(arr, m, asd);
#pragma omp task
		bitonicMergeOMP(arr + m, m, asd);
#pragma omp taskwait
	}
}

void bitonicSortOMP(float* arr, int len, bool asd) {
	if (len > 1) {
		int m = len / 2;
#pragma omp parallel
#pragma omp single nowait
		{
#pragma omp task
			bitonicSortOMP(arr, m, !asd);
#pragma omp task
			bitonicSortOMP(arr + m, len - m, asd);
		}
		bitonicMergeOMP(arr, len, asd);
	}
}

/*
void floatSort(const float data[], float result[], const int len) {
	for (int i = 0; i < len; i++) {
		result[i] = log(sqrt(data[i]));
	}
	bitonicSort(result, DATANUM, false);
}
*/

void sortSpeedUp(const float data[], float result[], const int len) {
	for (int i = 0; i < len; i++) {
		result[i] = log(sqrt(data[i]));
	}
	bitonicSortOMP(result, DATANUM, false);
}