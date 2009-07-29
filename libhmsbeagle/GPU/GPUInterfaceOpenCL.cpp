/*
 * @author Daniel Ayres
 */

#ifdef HAVE_CONFIG_H
#include "libhmsbeagle/config.h"
#endif

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdarg>

#include "libhmsbeagle/GPU/GPUImplDefs.h"
#include "libhmsbeagle/GPU/GPUImplHelper.h"
#include "libhmsbeagle/GPU/GPUInterface.h"

#define SAFE_CL(call)   { \
                            int error = call; \
                            if(error != CL_SUCCESS) { \
                                fprintf(stderr, \
                                    "OpenCL error: \"%s\" from file <%s>, line %i.\n", \
                                    GetCLErrorDescription(error), __FILE__, __LINE__); \
                                exit(-1); \
                            } \
                        }

GPUInterface::GPUInterface() {    
    openClDeviceId = NULL;
    openClContext = NULL;
    openClCommandQueue = NULL;
    openClProgram = NULL;
    openClNumDevices = NULL;
}

GPUInterface::~GPUInterface() {
    
    // TODO: cleanup mem objects, kernels
    
    SAFE_CL(clReleaseProgram(openClProgram));
    SAFE_CL(clReleaseCommandQueue(openClCommandQueue));
    SAFE_CL(clReleaseContext(openClContext));
}

int GPUInterface::GetDeviceCount() {        
    SAFE_CL(clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, NULL, NULL,
                           &openClNumDevices));
    
    return openClNumDevices;
}

void GPUInterface::SetDevice(int deviceNumber) {
    cl_device_id  deviceIds[openClNumDevices];
    
    SAFE_CL(clGetDeviceIDs(NULL, CL_DEVICE_TYPE_GPU, openClNumDevices, deviceIds,
                           NULL));
    
    openClDeviceId = deviceIds[deviceNumber];
    
    int err;
    
    openClContext = clCreateContext(NULL, 1, &openClDeviceId, NULL, NULL, &err);
    SAFE_CL(err);
    
    openClCommandQueue = clCreateCommandQueue(openClContext, openClDeviceId, 0, &err);
    SAFE_CL(err);
    
    const char* kernelsString = KERNELS_STRING;
    
    openClProgram = clCreateProgramWithSource(openClContext, 1,
                                              (const char**) &kernelsString, NULL,
                                              &err);
    SAFE_CL(err);
    if (!openClProgram) {
        fprintf(stderr, "OpenCL error: Failed to create kernels\n");
        exit(-1);
    }
    
    err = clBuildProgram(openClProgram, 0, NULL, OPENCL_BUILD_OPTIONS, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        
        fprintf(stderr, "OpenCL error: Failed to build kernels\n");
        
        clGetProgramBuildInfo(openClProgram, openClDeviceId, CL_PROGRAM_BUILD_LOG,
                              sizeof(buffer), buffer, &len);
        
        fprintf(stderr, "%s\n", buffer);
        
        exit(-1);
    }
    
    SAFE_CL(clUnloadCompiler());
}

void GPUInterface::Synchronize() {
    SAFE_CL(clFinish(openClCommandQueue));
}

GPUFunction GPUInterface::GetFunction(const char* functionName) {
    GPUFunction openClFunction;
    
    int err;
    openClFunction = clCreateKernel(openClProgram, functionName, &err);
    SAFE_CL(err);
    if (!openClFunction) {
        printf("OpenCL error: Failed to create compute kernel %s\n", functionName);
        exit(-1);
    }
    
    return openClFunction;
}

void GPUInterface::LaunchKernelIntParams(GPUFunction deviceFunction,
                                         Dim3Int block,
                                         Dim3Int grid,
                                         int totalParameterCount,
                                         ...) { // unsigned int parameters    
    va_list parameters;
    va_start(parameters, totalParameterCount);  
    for(int i = 0; i < totalParameterCount; i++) {
        unsigned int param = va_arg(parameters, unsigned int);
                
        SAFE_CL(clSetKernelArg(deviceFunction, i, sizeof(unsigned int), &param));
    }
    va_end(parameters);
    
    size_t blockArray[3];
    blockArray[0] = block.x;
    blockArray[1] = block.y;
    blockArray[2] = block.z;
    
    size_t gridArray[3];
    gridArray[0] = grid.x;
    gridArray[1] = grid.y;
    gridArray[2] = grid.z;
    
    SAFE_CL(clEnqueueNDRangeKernel(openClCommandQueue, deviceFunction, 3, NULL,
                                   gridArray, blockArray, 0, NULL, NULL));
}


GPUPtr GPUInterface::AllocateMemory(int memSize) {
    GPUPtr data;
    
    int err;
    data = clCreateBuffer(openClContext, CL_MEM_READ_WRITE, memSize, NULL, &err);
    SAFE_CL(err);
    
    return data;
}

GPUPtr GPUInterface::AllocateRealMemory(int length) {    
    GPUPtr data;

    int err;
    data = clCreateBuffer(openClContext, CL_MEM_READ_WRITE, SIZE_REAL * length, NULL,
                          &err);
    SAFE_CL(err);
    
    return data;
}

GPUPtr GPUInterface::AllocateIntMemory(int length) {    
    GPUPtr data;
    
    int err;
    data = clCreateBuffer(openClContext, CL_MEM_READ_WRITE, SIZE_INT * length, NULL,
                          &err);
    SAFE_CL(err);
        
    return data;
}

void GPUInterface::MemcpyHostToDevice(GPUPtr dest,
                                      const void* src,
                                      int memSize) {    
    SAFE_CL(clEnqueueWriteBuffer(openClCommandQueue, dest, CL_TRUE, 0, memSize, src, 0,
                                 NULL, NULL));
}

void GPUInterface::MemcpyDeviceToHost(void* dest,
                                      const GPUPtr src,
                                      int memSize) {
    SAFE_CL(clEnqueueReadBuffer(openClCommandQueue, src, CL_TRUE, 0, memSize, dest, 0,
                                NULL, NULL));
}

void GPUInterface::FreeMemory(GPUPtr dPtr) {
    SAFE_CL(clReleaseMemObject(dPtr));
}

void GPUInterface::PrintInfo() {        
    fprintf(stderr, "GPU Device Information:");
    
    char name[100];
    cl_ulong totalGlobalMemory = 0;
    cl_uint clockSpeed = 0;
    unsigned int mpCount = 0;
    
    
    SAFE_CL(clGetDeviceInfo(openClDeviceId, CL_DEVICE_NAME, sizeof(char) * 100, name,
                            NULL));
    SAFE_CL(clGetDeviceInfo(openClDeviceId, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong),
                            &totalGlobalMemory, NULL));
    SAFE_CL(clGetDeviceInfo(openClDeviceId, CL_DEVICE_MAX_CLOCK_FREQUENCY,
                            sizeof(cl_uint), &clockSpeed, NULL));
    SAFE_CL(clGetDeviceInfo(openClDeviceId, CL_DEVICE_MAX_COMPUTE_UNITS,
                            sizeof(unsigned int), &mpCount, NULL));
    
    fprintf(stderr, "\nDevice: %s\n", name);
    fprintf(stderr, "\tGlobal Memory (MB): %1.2f\n", totalGlobalMemory / 1024 / 1024.0);
    fprintf(stderr, "\tClock Speed (Ghz) : %1.2f\n", clockSpeed / 1000.0);
    fprintf(stderr, "\tNumber of Cores   : %d\n", mpCount);
    
}

void GPUInterface::PrintfDeviceVector(GPUPtr dPtr,
                                      int length) {
    REAL* hPtr = (REAL*) malloc(SIZE_REAL * length);
    
    MemcpyDeviceToHost(hPtr, dPtr, SIZE_REAL * length);
    
#ifdef DOUBLE_PRECISION
    printfVectorD(hPtr, length);
#else
    printfVectorF(hPtr,length);
#endif
    
    free(hPtr);
}

void GPUInterface::PrintfDeviceInt(GPUPtr dPtr,
                                   int length) {    
    int* hPtr = (int*) malloc(SIZE_INT * length);
    
    MemcpyDeviceToHost(hPtr, dPtr, SIZE_INT * length);
    
    printfInt(hPtr, length);
    
    free(hPtr);
}

const char* GPUInterface::GetCLErrorDescription(int errorCode) {
    const char* errorDesc;
    
    // Error Codes (from cl.h)
    switch(errorCode) {
        case CL_SUCCESS: errorDesc = "CL_SUCCESS"; break;
        case CL_DEVICE_NOT_FOUND: errorDesc = "CL_DEVICE_NOT_FOUND"; break;
        case CL_DEVICE_NOT_AVAILABLE: errorDesc = "CL_DEVICE_NOT_AVAILABLE"; break;
        case CL_COMPILER_NOT_AVAILABLE: errorDesc = "CL_COMPILER_NOT_AVAILABLE"; break;
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: errorDesc = "CL_MEM_OBJECT_ALLOCATION_FAILURE"; break;
        case CL_OUT_OF_RESOURCES: errorDesc = "CL_OUT_OF_RESOURCES"; break;
        case CL_OUT_OF_HOST_MEMORY: errorDesc = "CL_OUT_OF_HOST_MEMORY"; break;
        case CL_PROFILING_INFO_NOT_AVAILABLE: errorDesc = "CL_PROFILING_INFO_NOT_AVAILABLE"; break;
        case CL_MEM_COPY_OVERLAP: errorDesc = "CL_MEM_COPY_OVERLAP"; break;
        case CL_IMAGE_FORMAT_MISMATCH: errorDesc = "CL_IMAGE_FORMAT_MISMATCH"; break;
        case CL_IMAGE_FORMAT_NOT_SUPPORTED: errorDesc = "CL_IMAGE_FORMAT_NOT_SUPPORTED"; break;
        case CL_BUILD_PROGRAM_FAILURE: errorDesc = "CL_BUILD_PROGRAM_FAILURE"; break;
        case CL_MAP_FAILURE: errorDesc = "CL_MAP_FAILURE"; break;
            
        case CL_INVALID_VALUE: errorDesc = "CL_INVALID_VALUE"; break;
        case CL_INVALID_DEVICE_TYPE: errorDesc = "CL_INVALID_DEVICE_TYPE"; break;
        case CL_INVALID_PLATFORM: errorDesc = "CL_INVALID_PLATFORM"; break;
        case CL_INVALID_DEVICE: errorDesc = "CL_INVALID_DEVICE"; break;
        case CL_INVALID_CONTEXT: errorDesc = "CL_INVALID_CONTEXT"; break;
        case CL_INVALID_QUEUE_PROPERTIES: errorDesc = "CL_INVALID_QUEUE_PROPERTIES"; break;
        case CL_INVALID_COMMAND_QUEUE: errorDesc = "CL_INVALID_COMMAND_QUEUE"; break;
        case CL_INVALID_HOST_PTR: errorDesc = "CL_INVALID_HOST_PTR"; break;
        case CL_INVALID_MEM_OBJECT: errorDesc = "CL_INVALID_MEM_OBJECT"; break;
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: errorDesc = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"; break;
        case CL_INVALID_IMAGE_SIZE: errorDesc = "CL_INVALID_IMAGE_SIZE"; break;
        case CL_INVALID_SAMPLER: errorDesc = "CL_INVALID_SAMPLER"; break;
        case CL_INVALID_BINARY: errorDesc = "CL_INVALID_BINARY"; break;
        case CL_INVALID_BUILD_OPTIONS: errorDesc = "CL_INVALID_BUILD_OPTIONS"; break;
        case CL_INVALID_PROGRAM: errorDesc = "CL_INVALID_PROGRAM"; break;
        case CL_INVALID_PROGRAM_EXECUTABLE: errorDesc = "CL_INVALID_PROGRAM_EXECUTABLE"; break;
        case CL_INVALID_KERNEL_NAME: errorDesc = "CL_INVALID_KERNEL_NAME"; break;
        case CL_INVALID_KERNEL_DEFINITION: errorDesc = "CL_INVALID_KERNEL_DEFINITION"; break;
        case CL_INVALID_KERNEL: errorDesc = "CL_INVALID_KERNEL"; break;
        case CL_INVALID_ARG_INDEX: errorDesc = "CL_INVALID_ARG_INDEX"; break;
        case CL_INVALID_ARG_VALUE: errorDesc = "CL_INVALID_ARG_VALUE"; break;
        case CL_INVALID_ARG_SIZE: errorDesc = "CL_INVALID_ARG_SIZE"; break;
        case CL_INVALID_KERNEL_ARGS: errorDesc = "CL_INVALID_KERNEL_ARGS"; break;
        case CL_INVALID_WORK_DIMENSION: errorDesc = "CL_INVALID_WORK_DIMENSION"; break;
        case CL_INVALID_WORK_GROUP_SIZE: errorDesc = "CL_INVALID_WORK_GROUP_SIZE"; break;
        case CL_INVALID_WORK_ITEM_SIZE: errorDesc = "CL_INVALID_WORK_ITEM_SIZE"; break;
        case CL_INVALID_GLOBAL_OFFSET: errorDesc = "CL_INVALID_GLOBAL_OFFSET"; break;
        case CL_INVALID_EVENT_WAIT_LIST: errorDesc = "CL_INVALID_EVENT_WAIT_LIST"; break;
        case CL_INVALID_EVENT: errorDesc = "CL_INVALID_EVENT"; break;
        case CL_INVALID_OPERATION: errorDesc = "CL_INVALID_OPERATION"; break;
        case CL_INVALID_GL_OBJECT: errorDesc = "CL_INVALID_GL_OBJECT"; break;
        case CL_INVALID_BUFFER_SIZE: errorDesc = "CL_INVALID_BUFFER_SIZE"; break;
        case CL_INVALID_MIP_LEVEL: errorDesc = "CL_INVALID_MIP_LEVEL"; break;
            
        default: errorDesc = "Unknown error";
    }
    
    return errorDesc;
}
