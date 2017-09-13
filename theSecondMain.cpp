#include <stdlib.h>
#include <sys/types.h>
#include <CL/cl.h>
#include <iostream>
#include <time.h>

int main(){

cl_platform_id platform;
cl_context context;
cl_command_queue queue;
cl_device_id device;
cl_int error;

//platform
if (clGetPlatformIDs(1,&platform, NULL) != CL_SUCCESS) {
   std::cout << "Error getting platform id\n";
   exit(error);
}
std::cout << "PLatform Id :" << platform <<"\n";

// Device
if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, NULL)!= CL_SUCCESS) {
   std::cout << "Error getting device ids\n";
   exit(error);
}
std::cout << "Device Id :" << device <<"\n";

// Context
context = clCreateContext(0, 1, &device, NULL, NULL, &error);
if (error != CL_SUCCESS) {
   std::cout << "Error creating context\n";
   exit(error);
}
//std::cout << "Context :"<< context <<"\n";

//FOR PROFILING

cl_event timing_event;
cl_ulong time_start, time_end;
float read_time;

// Command-queue
queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &error);
if (error != CL_SUCCESS) {
   std::cout << "Error creating command queue\n";
   exit(error);
}


 // Initialize of memory
	const int size = 10000;
	float* src_a_h = new float[size];
	float* src_b_h = new float[size];
	float* res_h = new float[size];
	// Initialize both vectors
	for (int i = 0; i < size; i++) {
	  src_a_h[i] = src_b_h[i] = (float) i;
	}
	

// Creating buffers for the arrays
const int mem_size = sizeof(float)*size;
// Allocates a buffer of size mem_size and copies mem_size bytes from src_a_h and others
cl_mem src_a_d = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, mem_size, src_a_h, &error);
cl_mem src_b_d = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, mem_size, src_b_h, &error);
cl_mem res_d = clCreateBuffer(context, CL_MEM_WRITE_ONLY, mem_size, NULL, &error);


cl_program program;
FILE *program_handle;
char* program_buffer;
size_t program_size;

program_handle = fopen("vector_add_gpu.cl", "r");
fseek(program_handle, 0, SEEK_END);
program_size = ftell(program_handle);
rewind(program_handle);
//std::cout<<program_size<<"\n";
program_buffer = (char*)malloc(program_size + 1);
program_buffer[program_size] = '\0';
fread(program_buffer, sizeof(char), program_size,program_handle);
fclose(program_handle);

program = clCreateProgramWithSource(context, 1,(const char**)&program_buffer, &program_size, &error);

// Builds the program
error = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

// Shows the log
char* build_log;
size_t log_size;
// First call to know the proper size
clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
build_log = new char[log_size+1];
// Second call to get the log
clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
build_log[log_size] = '\0';
std::cout << build_log <<"\n";
delete[] build_log;

// Extracting the kernel
cl_kernel vector_add_kernel = clCreateKernel(program, "vector_add_gpu", &error);
//assert(error == CL_SUCCESS);

error = clSetKernelArg(vector_add_kernel, 0, sizeof(cl_mem), &src_a_d);
error |= clSetKernelArg(vector_add_kernel, 1, sizeof(cl_mem), &src_b_d);
error |= clSetKernelArg(vector_add_kernel, 2, sizeof(cl_mem), &res_d);
error |= clSetKernelArg(vector_add_kernel, 3, sizeof(size_t), &size);
//assert(error == CL_SUCCESS);

// Launching kernel


const size_t work_units_per_kernel = (size_t)size;
//const size_t global_units = ((size/work_units_per_kernel) + 1)*(work_units_per_kernel); 

clEnqueueNDRangeKernel(queue, vector_add_kernel, 1, NULL,  &work_units_per_kernel, NULL, 0, NULL, NULL);


//getting global work size


clEnqueueReadBuffer(queue, res_d, CL_TRUE, 0, mem_size, res_h, 0, NULL, &timing_event);

clGetEventProfilingInfo(timing_event, CL_PROFILING_COMMAND_START,
sizeof(time_start), &time_start, NULL);
clGetEventProfilingInfo(timing_event, CL_PROFILING_COMMAND_END,
sizeof(time_end), &time_end, NULL);
read_time = time_end - time_start;
printf("..............%f(miliseconds).................\n\n",(read_time/1000000));

for(int i = 0; i < size; i++)
{
	std::cout << res_h[i]<<"\t";
}


// Cleaning up
delete[] src_a_h;
delete[] src_b_h;
delete[] res_h;
//delete[] check;
clReleaseKernel(vector_add_kernel);
clReleaseCommandQueue(queue);
clReleaseContext(context);
clReleaseMemObject(src_a_d);
clReleaseMemObject(src_b_d);
clReleaseMemObject(res_d);
return 0;
} // Main Ends Here