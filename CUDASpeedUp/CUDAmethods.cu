//
// Created by HP on 2023/12/31.
//
#include <cstdio>
#include <cmath>
#include "CUDAmethods.cuh"
#include <cuda_runtime.h>
#include "immintrin.h"
#include <windows.h>
extern float *hostInput, *hostOutput;      // host
extern float *deviceInput, *deviceOutput;  // GPU
extern LARGE_INTEGER start, end;
const int data_len = DATANUM;
////////////////////////////////
//            CPU             //
////////////////////////////////
float sum(const float data[], const int len)
{
    float sum = 0.0;
    float c = 0.0;

    for (int i = 0; i < len; i++) {
        float y = log(sqrt(data[i])) - c;
        float t = sum + y;
        c = (t - sum) - y;
        sum = t;

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

void merge(float *list, float *sorted, int start, int mid, int end)
{
    int ti=start, i=start, j=mid;
    while (i<mid || j<end)
    {
        if (j==end)
            sorted[ti] = list[i++];
        else if (i==mid)
            sorted[ti] = list[j++];
        else if (list[i]<list[j])
            sorted[ti] = list[i++];
        else
            sorted[ti] = list[j++];
        ti++;
    }

    for (ti=start; ti<end; ti++)
        list[ti] = sorted[ti];
}

void mergesort_recur(float *list, float *sorted, int start, int end)
{
    if (end-start<2)
        return;
    mergesort_recur(list, sorted, start, start + (end-start)/2);
    mergesort_recur(list, sorted, start + (end-start)/2, end);
    merge(list, sorted, start, start + (end-start)/2, end);
}

int mergesort_cpu(const float *list, float *sorted, int n)
{
    float* arr = new float[DATANUM];
    for(int i=0;i<n;i++)
        arr[i]= log(sqrt(list[i]));

    mergesort_recur(arr, sorted, 0, n);
    return 1;
}

////////////////////////////////
//            GPU             //
////////////////////////////////

void syncAndCheckCUDAError() {
    cudaError_t cudaStatus = cudaGetLastError();
    if (cudaStatus != cudaSuccess) {
        printf("CUDA error: %s\n", cudaGetErrorString(cudaStatus));
    }
}

__global__ void sum1(const float *src, int size, float *result){
    __shared__ float cache[THREADS_PER_BLOCK];
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    int cachedIndex = threadIdx.x;
    float temp = 0;
    while(tid < size) {
        temp += log(sqrt(src[tid]));
        tid += gridDim.x * blockDim.x;
    }
    cache[cachedIndex] = temp;

    __syncthreads();

    for (int i = blockDim.x / 2; i > 0; i >>= 1) {
        if (cachedIndex < i) {
            cache[cachedIndex] += cache[cachedIndex + i];
        }
        __syncthreads();
    }

    if(cachedIndex == 0)
        atomicAdd(result, cache[0]);
}

__global__ void sum2(float *result) {
    int cachedIndex = threadIdx.x;

    for (int i = blockDim.x / 2; i > 0; i >>= 1) {
        if (cachedIndex < i) {
            result[cachedIndex] += result[cachedIndex + i];
        }
        __syncthreads();
    }
}

float sumSpeedUpCUDA(){
    int block_num = (DATANUM + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    if (block_num > THREADS_PER_BLOCK)
        block_num = THREADS_PER_BLOCK;
    QueryPerformanceCounter(&start);
    sum1<<<block_num, THREADS_PER_BLOCK>>>(deviceInput, DATANUM, deviceOutput);
    sum2<<<1, block_num>>>(deviceOutput);
    cudaDeviceSynchronize();
    QueryPerformanceCounter(&start);
    float result = 0;
    cudaMemcpy(&result, deviceOutput, sizeof(float), cudaMemcpyDeviceToHost);
    syncAndCheckCUDAError();

    cudaFree(deviceInput);
    cudaFree(deviceOutput);
    syncAndCheckCUDAError();

    return result;
}

__device__ void atomicMaxFloat(float* address, float val) {
    int* address_as_int = reinterpret_cast<int*>(address);
    int old_val_as_int = *address_as_int;
    int assumed;
    do {
        assumed = old_val_as_int;
        float old_val = __int_as_float(assumed);
        float new_val = fmaxf(old_val, val);
        old_val_as_int = __float_as_int(new_val);
    } while (assumed != old_val_as_int);
    *address_as_int = old_val_as_int;
}

__global__ void max_gpu(const float* src, int size, float* result) {
    __shared__ float cache[THREADS_PER_BLOCK];
    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    int cachedIndex = threadIdx.x;

    float temp = 0;
    while (tid < size) {
        temp = fmaxf(temp, logf(sqrtf(src[tid])));
        tid += gridDim.x * blockDim.x;
    }
    cache[cachedIndex] = temp;

    __syncthreads();

    for (int i = blockDim.x / 2; i > 0; i >>= 1) {
        if (cachedIndex < i) {
            cache[cachedIndex] = fmaxf(cache[cachedIndex], cache[cachedIndex + i]);
        }
        __syncthreads();
    }

    if (cachedIndex == 0) {
        atomicMaxFloat(result, cache[0]);
    }
}

float maxSpeedUpCUDA() {
    int block_num = (data_len + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    if (block_num > THREADS_PER_BLOCK)
        block_num = THREADS_PER_BLOCK;

    QueryPerformanceCounter(&start);
    max_gpu<<<block_num, THREADS_PER_BLOCK>>>(deviceInput, data_len, deviceOutput);
    QueryPerformanceCounter(&end);

    float result = 0;
    cudaMemcpy(&result, deviceOutput, sizeof(float), cudaMemcpyDeviceToHost);
    syncAndCheckCUDAError();

    cudaFree(deviceInput);
    cudaFree(deviceOutput);
    syncAndCheckCUDAError();

    return result;
}

__device__ void merge_gpu(float *list, float *sorted, int start, int mid, int end)
{
    int k=start, i=start, j=mid;
    while (i<mid || j<end)
    {
        if (j==end) sorted[k] = list[i++];
        else if (i==mid) sorted[k] = list[j++];
        else if (list[i]<list[j]) sorted[k] = list[i++];
        else sorted[k] = list[j++];
        k++;
    }
}

__global__ void mergesort_gpu(float *list, float *sorted, int n, int chunk){

    int tid = threadIdx.x + blockIdx.x * blockDim.x;
    int start = tid * chunk;
    if(start >= n)
        return;

    int mid = min(start + chunk / 2, n);
    int end = min(start + chunk, n);
    merge_gpu(list, sorted, start, mid, end);
}

// Sequential Merge Sort for GPU when Number of Threads Required gets below 1 Warp Size
void mergesort_gpu_seq( float *list, float *sorted, int n, int chunk){
    int chunk_id;
    for(chunk_id=0; chunk_id*chunk<=n; chunk_id++){
        int start = chunk_id * chunk, end, mid;
        if(start >= n)
            return;
        mid = min(start + chunk/2, n);
        end = min(start + chunk, n);
        merge(list, sorted, start, mid, end);
    }
}

int mergesortCUDA(const float list[], float sorted[], int n){
    float* arr = new float[DATANUM];
    float *list_d;
    float *sorted_d;
    int dummy;
    bool flag = false;
    bool sequential = false;

    int size = n * sizeof(int);

    cudaMalloc((void **)&list_d, size);
    cudaMalloc((void **)&sorted_d, size);

    cudaMemcpy(list_d, arr, size, cudaMemcpyHostToDevice);
    cudaError_t err = cudaGetLastError();
    if(err!=cudaSuccess){
        printf("Error_2: %s\n", cudaGetErrorString(err));
        return -1;
    }

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);

    const int max_active_blocks_per_sm = 32;
    const int max_active_warps_per_sm = 64;

    int warp_size = prop.warpSize;
    int max_grid_size = prop.maxGridSize[0];
    int max_threads_per_block = prop.maxThreadsPerBlock;
    int max_procs_count = prop.multiProcessorCount;

    int max_active_blocks = max_active_blocks_per_sm * max_procs_count;
    int max_active_warps = max_active_warps_per_sm * max_procs_count;

    int chunk_size;

    // Time Start
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&start));
    for(int i=0;i<n;i++)
        arr[i]= log(sqrt(list[i]));
    for(chunk_size=2; chunk_size<2*n; chunk_size*=2){
        int blocks_required=0, threads_per_block=0;
        int threads_required = (n%chunk_size==0) ? n/chunk_size : n/chunk_size+1;

        if (threads_required<=warp_size*3 && !sequential){
            sequential = true;
            //Time End
            QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&end));
            if(flag)
                cudaMemcpy(sorted, sorted_d, size, cudaMemcpyDeviceToHost);
            else
                cudaMemcpy(sorted, list_d, size, cudaMemcpyDeviceToHost);
            err = cudaGetLastError();
            if(err!=cudaSuccess){
                printf("ERROR_4: %s\n", cudaGetErrorString(err));
                return -1;
            }
            cudaFree(list_d);
            cudaFree(sorted_d);
        }
        else if (threads_required<max_threads_per_block){
            threads_per_block = warp_size*4;
            dummy = threads_required/threads_per_block;
            blocks_required = (threads_required%threads_per_block==0) ? dummy : dummy+1;
        }
        else if(threads_required<max_active_blocks*warp_size*4){
            threads_per_block = max_threads_per_block/2;
            dummy = threads_required/threads_per_block;
            blocks_required = (threads_required%threads_per_block==0) ? dummy : dummy+1;
        }
        else{
            dummy = threads_required/max_active_blocks;
            int estimated_threads_per_block = (threads_required%max_active_blocks==0) ? dummy : dummy+1;
            if(estimated_threads_per_block > max_threads_per_block){
                threads_per_block = max_threads_per_block;
                dummy = threads_required/max_threads_per_block;
                blocks_required = (threads_required%max_threads_per_block==0) ? dummy : dummy+1;
            }
            else{
                threads_per_block = estimated_threads_per_block;
                blocks_required = max_active_blocks;
            }
        }

        if(blocks_required>=max_grid_size){
            printf("ERROR_2: Too many Blocks Required\n");
            return -1;
        }
        if(sequential){
            mergesort_gpu_seq(arr, sorted, n, chunk_size);
        }else{
            if(flag) mergesort_gpu<<<blocks_required, threads_per_block>>>(sorted_d, list_d, n, chunk_size);
            else mergesort_gpu<<<blocks_required, threads_per_block>>>(list_d, sorted_d, n, chunk_size);
            cudaDeviceSynchronize();
            err = cudaGetLastError();
            if(err!=cudaSuccess){
                printf("ERROR_3: %s\n", cudaGetErrorString(err));
                return -1;
            }
            flag = !flag;
        }
    }
    return 0;
}