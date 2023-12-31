#include <iostream>
#include <cmath>
#include <omp.h>
#include <windows.h>
#include <immintrin.h>
#include "methods.h"

//未加速版本
float sum(const float data[], const int len)
{
	float sum = 0.0;   // 准备累加器。
	float c = 0.0;     // 用于追踪低位丢失的比特的运行补偿。

	for (int i = 0; i < len; i++) {
		float y = log(sqrt(data[i])) - c;  // 计算 y，减去运行补偿。
		float t = sum + y;                  // 计算中间和。
		c = (t - sum) - y;                  // 更新运行补偿。
		sum = t;   // 更新累加器。

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

void mergeOfTwo(float arr1[], float arr2[], float merged[], const int len) {
	int i = 0, j = 0, k = 0;

	while (i < len && j < len) {
		if (arr1[i] >= arr2[j]) {  // 修改这里的比较条件
			merged[k] = arr1[i];
			i++;
		}
		else {
			merged[k] = arr2[j];
			j++;
		}
		k++;
	}

	while (i < len) {
		merged[k] = arr1[i];
		i++;
		k++;
	}

	while (j < len) {
		merged[k] = arr2[j];
		j++;
		k++;
	}
}


void merge(float arr[], int left, int mid, int right) {
	int i = left;
	int j = mid + 1;
	int k = 0;
	int tempSize = right - left + 1;
	float* temp = new float[tempSize];

	// 从大到小排序
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

//单机加速版本
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
	HANDLE hThreads[MAX_THREADS] = { nullptr };

	for (int i = 0; i < MAX_THREADS; i++) {
		thread_data[i].data = const_cast<float*>(&result[chunkSize * i]);
		thread_data[i].len = chunkSize;
		thread_data[i].max = 0;

		hThreads[i] = CreateThread(
			NULL,              // 默认安全属性
			0,                 // 使用默认堆栈大小
			sortSpeedUpThread,  // 线程函数
			&thread_data[i],   // 传递给线程函数的参数
			0,                 // 使用默认创建标志。0表示线程立即运行
			NULL               // 不获取线程标识符
		);
		if (hThreads[i] == NULL) {
			DWORD error = GetLastError();
			LPVOID errorMsg;
			FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				0, // Default language
				(LPWSTR)&errorMsg,
				0,
				NULL
			);

			std::cerr << "Failed to create thread " << i << ". Error Code: " << error
				<< "\nError Message: " << errorMsg << std::endl;

			LocalFree(errorMsg);  // 释放 FormatMessage 分配的缓冲区
			// 可以根据需要采取其他处理措施
			continue;  // 跳过失败的线程，继续创建下一个线程
		}

		// 检查 hThreads[i] 是否为 NULL，如果是，就不要调用 ResumeThread
		if (hThreads[i] != NULL) {
			ResumeThread(hThreads[i]);
		}
	}

	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
#pragma omp parallel for
	for (int i = 0; i < MAX_THREADS; i++) {
		if (hThreads[i] != NULL) {
			
			CloseHandle(hThreads[i]);
			}
		else
			std::cout << "线程没有创建" << std::endl;
	}

	// 汇总各线程的排序结果
	mergeResults(result, chunkSize, MAX_THREADS);
}


// 求sse256寄存器中8个数的最大值
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

// 求sse256寄存器中8个数的和
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
	HANDLE hThreads[MAX_THREADS] = { nullptr };
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
		if (hThreads[i] == NULL) {
			DWORD error = GetLastError();
			LPVOID errorMsg;
			FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				0, // Default language
				(LPWSTR)&errorMsg,
				0,
				NULL
			);

			std::cerr << "Failed to create thread " << i << ". Error Code: " << error
				<< "\nError Message: " << errorMsg << std::endl;

			LocalFree(errorMsg);  // 释放 FormatMessage 分配的缓冲区
			// 可以根据需要采取其他处理措施
			continue;  // 跳过失败的线程，继续创建下一个线程
		}

		// 检查 hThreads[i] 是否为 NULL，如果是，就不要调用 ResumeThread
		if (hThreads[i] != NULL) {
			ResumeThread(hThreads[i]);
		}
	}

	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
	float max = 0;
	for (int i = 0; i < MAX_THREADS; i++) {
		if (hThreads[i] != NULL && max < thread_data[i].max) {
			max = thread_data[i].max;
		}
		
		if (hThreads[i] != NULL) {
			// 在检查 hThreads[i] 不为 NULL 的情况下才调用 CloseHandle
			CloseHandle(hThreads[i]);
		}

		// 在检查 hThreads[i] 不为 NULL 的情况下才更新 max
		
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

		// 计算中间和。
		__m256 t = _mm256_add_ps(sum, y);

		// 更新运行补偿。
		c = _mm256_sub_ps(_mm256_sub_ps(t, sum), y);
		sum = t;
	}


	float* resultArray = (float*)&sum;
	thread_data->sum = resultArray[0] + resultArray[1] + resultArray[2] + resultArray[3] +
		resultArray[4] + resultArray[5] + resultArray[6] + resultArray[7];

	return 0;
}

float sumSpeedUpManual(float data[], const int len) {
	HANDLE hThreads[MAX_THREADS] = { nullptr };
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
		if (hThreads[i] == NULL) {
			DWORD error = GetLastError();
			LPVOID errorMsg;
			FormatMessageW(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				0, // Default language
				(LPWSTR)&errorMsg,
				0,
				NULL
			);

			std::cerr << "Failed to create thread " << i << ". Error Code: " << error
				<< "\nError Message: " << errorMsg << std::endl;

			LocalFree(errorMsg);  // 释放 FormatMessage 分配的缓冲区
			// 可以根据需要采取其他处理措施
			continue;  // 跳过失败的线程，继续创建下一个线程
		}

		// 检查 hThreads[i] 是否为 NULL，如果是，就不要调用 ResumeThread
		if (hThreads[i] != NULL) {
			ResumeThread(hThreads[i]);
		}
	}

	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
	float sum = 0;
	for (int i = 0; i < MAX_THREADS; i++) {
		// 在检查 hThreads[i] 不为 NULL 的情况下才调用 CloseHandle
		if (hThreads[i] != NULL) {
			CloseHandle(hThreads[i]);
		}

		sum += thread_data[i].sum;
	}

	return sum;
}

// 测试排序结果是否正确(从小到大)
void test(float result[], const int len)
{
	for (int i = 0; i < len - 1; i++)
	{
		if (result[i] < result[i + 1])
		{
			std::cout << "排序结果错误" << std::endl;
			return;
		}
	}
	std::cout << "排序结果正确" << std::endl;
	return;
}

void display(const float data[], const int len)
{
	for (int i = 0; i < len; i++)
	{
		std::cout << data[i] << '\t';
	}
}