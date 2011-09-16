/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// OpenCL:
#define __NO_STD_VECTOR // Use cl::vector instead of STL version
#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
using namespace cl;

// STL:
#include <fstream>
#include <iostream>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

int main()
{
    // Here we implement the Gray-Scott model, as described here:
    // http://www.cc.gatech.edu/~turk/bio_sim/hw3.html
    // http://arxiv.org/abs/patt-sol/9304003

    // -- parameters --
    float r_a = 0.082f;
    float r_b = 0.041f;

    // for spots:
    float k = 0.064f;
    float f = 0.035f;
    // for stripes:
    //float k = 0.06f;
    //float f = 0.035f;
    // for long stripes
    //float k = 0.065f;
    //float f = 0.056f;
    // for dots and stripes
    //float k = 0.064f;
    //float f = 0.04f;
    // for spiral waves:
    //float k = 0.0475f;
    //float f = 0.0118f;
    float speed = 1.0f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    const int MEM_SIZE = sizeof(float)*X*Y;

    // put the initial conditions into each cell
    init(a,b);

    clock_t start,end;

    try { 
        // Get available OpenCL platforms
        vector<Platform> platforms;
        Platform::get(&platforms);
 
        // Select the default platform and create a context using this platform and the GPU
        cl_context_properties cps[3] = { 
            CL_CONTEXT_PLATFORM, 
            (cl_context_properties)(platforms[0])(), 
            0 
        };
        Context context( CL_DEVICE_TYPE_GPU, cps);
 
        // Get a list of devices on this platform
        vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
 
        // Create a command queue and use the first device
        CommandQueue queue = CommandQueue(context, devices[0]);
 
        // Read source file
        std::ifstream sourceFile("E:\\CUDA\\reaction-diffusion\\GrayScott_OpenCL\\grayscott_kernel.cl");
        std::string sourceCode(
            std::istreambuf_iterator<char>(sourceFile),
            (std::istreambuf_iterator<char>()));
        Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));
 
        // Make program of the source code in the context
        Program program = Program(context, source);
 
        // Build program for these specific devices
        program.build(devices);
 
        // Make kernel
        Kernel kernel(program, "grayscott_compute");
 
        // Create memory buffers
        Buffer bufferA = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferB = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferA2 = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferB2 = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
 
        // Copy lists A and B to the memory buffers
        queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, MEM_SIZE, a);
        queue.enqueueWriteBuffer(bufferB, CL_TRUE, 0, MEM_SIZE, b);
 
        NDRange global(X,Y);
        NDRange local(8,8);

        kernel.setArg(4, k);
        kernel.setArg(5, f);
        kernel.setArg(6, r_a);
        kernel.setArg(7, r_b);
        kernel.setArg(8, speed);

        int iteration = 0;
        const int N_FRAMES_PER_DISPLAY = 1000;  // an even number, because of our double-buffering implementation
        while(true) 
        {
            start = clock();

            // run a few iterations (without copying the data back)
            for(int it=0;it<N_FRAMES_PER_DISPLAY/2;it++)
            {
                // (buffer-switching)

                kernel.setArg(0, bufferA);
                kernel.setArg(1, bufferB);
                kernel.setArg(2, bufferA2);
                kernel.setArg(3, bufferB2); // output in A2,B2
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);
                iteration++;

                kernel.setArg(0, bufferA2);
                kernel.setArg(1, bufferB2);
                kernel.setArg(2, bufferA);
                kernel.setArg(3, bufferB); // output in A,B
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);
                iteration++;
            }

            // retrieve the buffers
            queue.enqueueReadBuffer(bufferA, CL_TRUE, 0, MEM_SIZE, a);
            queue.enqueueReadBuffer(bufferB, CL_TRUE, 0, MEM_SIZE, b);

            end = clock();

            char msg[1000];
            float fps = 0.0;
            if(end-start>0)
                fps = N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC);
            sprintf(msg,"GrayScott - %dms = %0.2f fps",end-start,fps);

            // display:
            if(display(a,a,a,iteration,false,200.0f,1,10,msg)) // did user ask to quit?
                break;
        }
    } 
    catch(Error error) 
    {
       std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

void init(float a[X][Y],float b[X][Y])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            // start with a uniform field with an approximate circle in the middle
            //if(hypot(i%20-10/*-X/2*/,j%20-10/*-Y/2*/)<=frand(2,5)) {
            if(hypot(i-X/2,(j-Y/2)/1.5)<=frand(2,5))
            {
                a[i][j] = 0.0f;
                b[i][j] = 1.0f;
            }
            else 
            {
                a[i][j] = 1.0f;
                b[i][j] = 0.0f;
            }
            /*float v = frand(0.0f,1.0f);
            a[i][j] = v;
            b[i][j] = 1.0f-v;*/
        }
    }
}

