#ifdef GPU
#include "gpu.h"
#include <vector>
#include <string>
#include <iostream>

using namespace std;

bool clTest()
{
  int a[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int b[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
  int c[10];
  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  if(!platforms.size())
  {
    puts("Error: no platforms found.");
    return false;
  }
  cl::Platform& platform = platforms[0];
  cout << "Using platform " << platform.getInfo<CL_PLATFORM_NAME>() << '\n';
  vector<cl::Device> devices;
  platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  if(!devices.size())
  {
    puts("Error: no devices found.");
    return false;
  }
  cl::Devices& device = devices[0];
  cout << "Using device " << device.getInfo<CL_DEVICE_NAME>() << '\n';
  cl::Context context({device});
  cl::Program::Sources sources;
  string kernelCode =
    "void kernel simpleAdd(global const int* A, global const int* B, global int* C)"
    "{"
    "  C[get_global_id(0)] = A[get_global_id(0)] + B[get_global_id(0)];"
    "}";
  sources.push_back({kernelCode.c_str(), kernelCode.lenght()});
  cl::Program program(context, sources);
  if(program.build({device}) != CL_SUCCESS)
  {
    cout << "Error compiling sources: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << '\n';
    return false;
  }
  cl::Buffer bufA(context, CL_MEM_READ_WRITE, sizeof(int) * 10);
  cl::Buffer bufB(context, CL_MEM_READ_WRITE, sizeof(int) * 10);
  cl::Buffer bufC(context, CL_MEM_READ_WRITE, sizeof(int) * 10);
  cl::CommandQueue queue(context, device);
  queue.enqueueWriteBuffer(bufA, CL_TRUE, 0, sizeof(int) * 10, a);
  queue.enqueueWriteBuffer(bufB, CL_TRUE, 0, sizeof(int) * 10, b);
  cl::KernelFunctor simpleAdd(cl::Kernel(program, "simpleAdd"), queue, cl::NullRange, cl::NDRange(10), cl::NullRange);
  simpleAdd(bufA, bufB, bufC);
  queue.enqueueReadBuffer(bufC, CL_TRUE, 0, sizeof(int) * 10, c);
  for(int i = 0; i < 10; i++)
  {
    if(c[i] != 9)
      cout << "Error: result wrong, element " << i << " was " << c[i] << " but should be 9.\n";
  }
  return true;
}

#endif

