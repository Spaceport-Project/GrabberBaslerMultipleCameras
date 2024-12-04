#include <iostream>
#include <cuda_runtime.h>

__global__ void computeIntensiveKernel(int* data, int size, int iterations) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    for (int i = tid; i < size; i += stride) {
        int value = data[i];
        for (int j = 0; j < iterations; ++j) {
            value += j;
        }
        data[i] = value;
    }
}

int main() {
    int size = 1024 * 1024;  // Size of the data array
    int iterations = 10000000;   // Number of iterations per element

    int* data;
    cudaMallocManaged(&data, size * sizeof(int));

    for (int i = 0; i < size; ++i) {
        data[i] = i;
    }

    int blockSize = 256;
    int numBlocks = (size + blockSize - 1) / blockSize;

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    // Launch the kernel on the default stream
    computeIntensiveKernel<<<numBlocks, blockSize>>>(data, size, iterations);

    // Launch the kernel on the custom stream
    computeIntensiveKernel<<<numBlocks, blockSize, 0, stream>>>(data, size, iterations);

    // Synchronize the custom stream
    cudaStreamSynchronize(stream);

    // Free memory and destroy the stream
    cudaFree(data);
    cudaStreamDestroy(stream);

    return 0;
}
