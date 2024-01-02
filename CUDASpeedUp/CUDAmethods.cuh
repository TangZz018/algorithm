//
// Created by HP on 2023/12/31.
//
#ifndef CUDASPEEDUP_CUDAMETHODS_CUH
#define CUDASPEEDUP_CUDAMETHODS_CUH
#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM  (SUBDATANUM * MAX_THREADS)
#define THREADS_PER_BLOCK 1024
float sum(const float data[], const int len);
float floatMax(const float data[], const int len);
int mergesort_cpu(const float *list, float *sorted, int n);
float sumSpeedUpCUDA();
float maxSpeedUpCUDA();
int mergesortCUDA(const float list[], float sorted[], int n);
#endif //CUDASPEEDUP_CUDAMETHODS_CUH
