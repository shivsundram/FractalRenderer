/*
LodePNG Examples

Copyright (c) 2005-2012 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#include "lodepng.h"
#include <iostream>
#include <math.h>
#include <OpenCL/opencl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <OpenCL/opencl.h>
#include <sys/time.h>
using namespace std;


#define MAX_SOURCE_SIZE (0x100000)
#define DATA_SIZE (10000)





//convert rgb values to PNG image
void generateImage(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height)
{
  //Generate the .png image 
  unsigned error = lodepng::encode(filename, image, width, height);

  //if there's an error, display it
  if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
}

void setPixel(std::vector<unsigned char>& image, int xres, int y, int x, int red, int blue, int green){
        image.at(4 * xres * y + 4 * x + 0) = red;
        image.at(4 * xres * y + 4 * x + 1) = blue;
        image.at(4 * xres * y + 4 * x + 2) = green; 
        image.at(4 * xres * y + 4 * x + 3) = 255;

}

//
//convert escape times to rgb values to create a white-black striped image of mandelbrot set
void encodeStripedImage(std::vector<unsigned char>& image, int * escapeData, unsigned xRes, unsigned yRes, int maxiter){

  for (int y = 0; y<yRes; y++){
    for(int x = 0; x <xRes; x++){
    //  printf("escape data: %i  %i\n",y*xRes + x, escapeData[y*xRes + x]);
      if (escapeData[y*xRes + x] > 500){
        setPixel(image, xRes, y, x, 150, 150, 150);
      }
      else{
        if (escapeData[y*xRes + x] % 2 ==0){
          setPixel(image,  xRes, y,  x, 0,0,0);
        }
        else{
          setPixel(image, xRes, y,  x, 150, 150, 150);
        }
      }
    }
  }
}


int main(int argc, char *argv[])
{

cl_uint num = 0;
clGetDeviceIDs(NULL,CL_DEVICE_TYPE_GPU,0,NULL,&num);
cout<<"available compute devices: "<<num<<endl;

//initial version of algorithm adapted from
//http://www.physics.emory.edu/faculty/weeks//software/mand.html
  //image info
  std::vector<unsigned char> image;
  int xRes = DATA_SIZE;    
  int yRes = DATA_SIZE;   

  const char* filename = argc > 1 ? argv[1] : "mandelbrottest.png";
  image.resize(xRes* yRes * 4);
  int * iterationcounts = (int*) malloc(sizeof(int)*xRes*yRes);

  //mandelbrot space
  double x_low=-1.5;
  double x_high=.8; 
  double y_low=-1;
  double y_high=1; 
  double xlen = x_high - x_low;
  double ylen = y_high - y_low;
  int maxiter = 1000; 

  struct timeval startcpu, endcpu;

  gettimeofday(&startcpu, NULL);
/*
  for (int yi =0 ; yi<xRes; yi++){
    for(int xi =0; xi<yRes; xi++){
      iterationcounts[yi*xRes+xi] = 0;
      int iteration = 0; 
      double x = 0 ;
      double y = 0; 
      double cx = x_low + xi*xlen/xRes;
      double cy = y_low + yi*ylen/yRes;
      while(x*x + y*y < 4 && iteration<maxiter){
        double xtemp = x*x - y*y + cx;
        y = 2*x*y + cy;
        x = xtemp; 
        iteration++; 
      }
      iterationcounts[yi*xRes+xi] = iteration;
    //cout<<"pixel: "<<yi*xRes+xi<<" iteration:"<<iteration<<endl;
    }
  }
  */
  gettimeofday(&endcpu, NULL);



    //start gpu computaiton
    int err;                            // error code returned from api calls
    unsigned int count = DATA_SIZE;
    float * data = (float*) malloc(sizeof(float)* count*count);             // original data set given to device
    int * results = (int*) malloc(sizeof(int)* count*count);              // results returned from device
    unsigned int correct;               // number of correct results returned

 
    cl_device_id device_id;             // compute device id 
    cl_context context;                 // compute context
    cl_command_queue commands;          // compute command queue
    cl_program program;                 // compute program
    cl_kernel kernel;                   // compute kernel
    
    cl_mem input;                       // device memory used for the input array
    cl_mem output;                      // device memory used for the output array
    
    // Fill our data set with random float values
    //
    int i = 0;

    cout<<"count: "<<count<<endl;
    for(i = 0; i < count*count; i++)
        data[i] = rand() / (float)RAND_MAX;
    
    // Connect to a compute device
    //
    int gpu = 1;

    err = clGetDeviceIDs(NULL, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to create a device group!\n");
        return EXIT_FAILURE;
    }
  
    // Create a compute context 
    //
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context)
    {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }
 
    // Create a command commands
    //
    commands = clCreateCommandQueue(context, device_id, 0, &err);
    if (!commands)
    {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }

    //extract program
    //
     
    FILE *fp;
    char fileName[] = "../src/kernel.cl";
    char *source_str;
    size_t source_size;
     
    /* Load the source code containing the kernel*/
    fp = fopen(fileName, "r");
      if (!fp) {
      fprintf(stderr, "Failed to load kernel.\n");
      exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

 
    // Create the compute program from the source buffer
    //
    program = clCreateProgramWithSource(context, 1, (const char **) & source_str, NULL, &err);
    if (!program)
    {
        printf("Error: Failed to create compute program!\n");
        return EXIT_FAILURE;
    }
 
    // Build the program executable
    //
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];
 
        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }
 
    // Create the compute kernel in the program we wish to run
    //
    kernel = clCreateKernel(program, "square2", &err);
    if (!kernel || err != CL_SUCCESS)
    {
        printf("Error: Failed to create compute kernel!\n");
        exit(1);
    }
 
    // Create the input and output arrays in device memory for our calculation
    //
    input = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(float) * count * count, NULL, NULL);
    output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * count * count, NULL, NULL);
    if (!input || !output)
    {
        printf("Error: Failed to allocate device memory!\n");
        exit(1);
    }    
    
    // Write our data set into the input array in device memory 
    //
    err = clEnqueueWriteBuffer(commands, input, CL_TRUE, 0, sizeof(float) * count*count, data, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to write to source array!\n");
        exit(1);
    }
 
    // Set the arguments to our compute kernel
    //
    err = 0;
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &input);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &output);
    err |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &count);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to set kernel arguments! %d\n", err);
        exit(1);
    }



    int localsize = 16; 
    size_t global []  = {count+(localsize-count%localsize), count+(localsize-count%localsize)}; // global domain size for our calculation
    size_t local [] = {localsize,localsize};                      // local domain size for our calculation
    struct timeval startgpu, endgpu;
    
    // Execute the kernel over the entire range of our 1d input data set
    // using the maximum number of work group items for this device
    //
    gettimeofday(&startgpu, NULL);
    err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global, local, 0, NULL, NULL);
    if (err)
    {
        printf("Error: Failed to execute kernel!\n");
        return EXIT_FAILURE;
    }
    gettimeofday(&endgpu, NULL);
    // Wait for the command commands to get serviced before reading back results
    //
    clFinish(commands);
 
    // Read back the results from the device to verify the output
    //
    err = clEnqueueReadBuffer( commands, output, CL_TRUE, 0, sizeof(int) * count *count, results, 0, NULL, NULL );  
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to read output array! %d\n", err);
        exit(1);
    }
    
    // Validate our results
    //
    correct = 0;
    for(i = 0; i < count*count; i++)
    {
        //printf("num: %i num: %f\n",i, results[i] );
        if(results[i] == data[i] * data[i])
            correct++;
    }
    
    // Print a brief summary detailing the results
    //
    printf("Computed '%d/%d' correct values!\n", correct, count);


  encodeStripedImage(image, results, xRes, yRes, maxiter);
  generateImage(filename, image, xRes, yRes);
    
    // Shutdown and cleanup
    //
    clReleaseMemObject(input);
    clReleaseMemObject(output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    free(data);
    free(results);
 
   printf("cpu seconds%ld\n", ((endcpu.tv_sec * 1000000 + endcpu.tv_usec)
      - (startcpu.tv_sec * 1000000 + startcpu.tv_usec)));

   printf("gpu seconds%ld\n", ((endgpu.tv_sec * 1000000 + endgpu.tv_usec)
      - (startgpu.tv_sec * 1000000 + startgpu.tv_usec)));
    return 0;



}


