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

#ifdef _WIN32
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <winsock.h>
    // http://www.linuxjournal.com/article/5574
    void gettimeofday(struct timeval* t,void* timezone)
    {       struct _timeb timebuffer;
          _ftime( &timebuffer );
          t->tv_sec=timebuffer.time;
          t->tv_usec=1000*timebuffer.millitm;
    }
#else
    #include <sys/time.h>
#endif

// OpenCL:
#define __NO_STD_VECTOR // Use cl::vector instead of STL version
#define __CL_ENABLE_EXCEPTIONS

// cl.hpp is standard but doesn't come with most SDKs, so download it from here:
// http://www.khronos.org/registry/cl/api/1.1/cl.hpp
#ifdef __APPLE__
# include "cl.hpp"
#else
# include <CL/cl.hpp>
#endif

using namespace cl;

// STL:
#include <fstream>
#include <iostream>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

static int g_opt_device = 0;
static int g_color = 0;
static int g_wrap = 0;

int main(int argc, char * * argv)
{
    for (int i = 1; i < argc; i++) {
        if (0) {
        } else if (strcmp(argv[i],"-color")==0) {
            // do output in wonderful technicolor
            g_color = 1;
        } else if ((i+1<argc) && (strcmp(argv[i],"-device")==0)) {
            // select an output device
            i++; g_opt_device = atoi(argv[i]);
        } else if (strcmp(argv[i],"-wrap")==0) {
            // patterns wrap around ("torus", also called "continuous boundary
            // condition")
            g_wrap = 1;
        } else {
            std::cout << "Unrecognized argument: '" << argv[i] << "'\n";
            exit(-1);
        }
    }

    // Here we implement the Gray-Scott model, as described here:
    // http://www.cc.gatech.edu/~turk/bio_sim/hw3.html
    // http://arxiv.org/abs/patt-sol/9304003

    // -- parameters --
    float r_a = 0.082f;
    float r_b = 0.041f;

    float k, f;
     k = 0.064f; f = 0.035f; // solitons with mitosis (spots that multiply)
    // k = 0.06f; f = 0.035f; // stripes
    // k = 0.065f; f = 0.056f; // long stripes
    // k = 0.064f; f = 0.04f; // dots and stripes
    // k = 0.0475f; f = 0.0118f; // spiral waves
    float speed = 2.0f;
    // ----------------
    
    // the first two will be used to send the initial pattern of
    // chemical concentrations into the GPU, and then all three arrays
    // are used to read the colour rendering back out.
    float red[X][Y], green[X][Y], blue[X][Y];
    const int MEM_SIZE = sizeof(float)*X*Y;

    // put the initial conditions into each cell
    init(red,green);

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

        // range-check the user's selection
        int maxdev = devices.size() - 1;
        g_opt_device = (g_opt_device > maxdev) ? maxdev :
                                     ((g_opt_device < 0) ? 0 : g_opt_device);
        std::cout << (maxdev+1) << " device(s) available; using device "
                                                    << g_opt_device << ".\n";

        // Create a command queue and use the selected device
        if (maxdev < 0) {
          std::cerr << "error -- need at least one OpenCL capable device.\n";
          exit(-1);
        }
        CommandQueue queue = CommandQueue(context, devices[g_opt_device]);
        Event event;
 
        // Read source file
        std::string kfn = CL_SOURCE_DIR; // (defined in CMakeLists.txt to be the source folder)
        kfn += "/grayscott_kernel.cl";
        std::ifstream sourceFile(kfn.c_str());
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

        // Same thing, cor colour kernel
        std::string kfn_2 = CL_SOURCE_DIR;
        kfn_2 += "/gs_colorkernel.cl";
        std::ifstream sourceFile_2(kfn_2.c_str());
        std::string sourceCode_2(
            std::istreambuf_iterator<char>(sourceFile_2),
            (std::istreambuf_iterator<char>()));
        Program::Sources source_2(1,
              std::make_pair(sourceCode_2.c_str(), sourceCode_2.length()+1));
        Program program_2 = Program(context, source_2);
        program_2.build(devices);
        Kernel k_col(program_2, "grayscott_colour");
 
        // Create memory buffers
        Buffer bufferU = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferV = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferU2 = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferV2 = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);

        Buffer bufferR = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferG = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferB = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);

        // Copy lists A and B to the memory buffers
        queue.enqueueWriteBuffer(bufferU, CL_TRUE, 0, MEM_SIZE, red);
        queue.enqueueWriteBuffer(bufferV, CL_TRUE, 0, MEM_SIZE, green);
 
        NDRange global(X,Y);
        NDRange local(8,8);

        kernel.setArg(4, k);
        kernel.setArg(5, f);
        kernel.setArg(6, r_a);
        kernel.setArg(7, r_b);
        kernel.setArg(8, speed);
        kernel.setArg(9, g_wrap);

        int iteration = 0;
        float fps_avg = 0.0; // decaying average of fps
        const int N_FRAMES_PER_DISPLAY = 10000;  // an even number, because of our double-buffering implementation
        while(true) 
        {
            struct timeval tod_record;
            double tod_before, tod_after, tod_elap;

            gettimeofday(&tod_record, 0);
            tod_before = ((double) (tod_record.tv_sec))
                                    + ((double) (tod_record.tv_usec)) / 1.0e6;

            // run a few iterations (without copying the data back)
            for(int it=0;it<N_FRAMES_PER_DISPLAY/2;it++)
            {
                // (buffer-switching)

                kernel.setArg(0, bufferU);
                kernel.setArg(1, bufferV);
                kernel.setArg(2, bufferU2);
                kernel.setArg(3, bufferV2); // output in A2,B2
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);
                iteration++;

                kernel.setArg(0, bufferU2);
                kernel.setArg(1, bufferV2);
                kernel.setArg(2, bufferU);
                kernel.setArg(3, bufferV); // output in A,B
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);
                iteration++;
            }

            if (g_color) {
                // Colorize
                k_col.setArg(0, bufferU);
                k_col.setArg(1, bufferV);
                k_col.setArg(2, bufferU2);
                k_col.setArg(3, bufferR);
                k_col.setArg(4, bufferG);
                k_col.setArg(5, bufferB);
                queue.enqueueNDRangeKernel(k_col, NullRange, global, local,
                                           NULL, &event);

                // Wait for this last command to finish before reading buffers
                // back into CPU memory.
                event.wait();

                // retrieve the buffers
                queue.enqueueReadBuffer(bufferR, CL_TRUE, 0, MEM_SIZE, red);
                queue.enqueueReadBuffer(bufferG, CL_TRUE, 0, MEM_SIZE, green);
                queue.enqueueReadBuffer(bufferB, CL_TRUE, 0, MEM_SIZE, blue);
            } else {
                // no colour -- just read the A pattern back into a buffer
                queue.enqueueReadBuffer(bufferU, CL_TRUE, 0, MEM_SIZE, red);
            }

            gettimeofday(&tod_record, 0);
            tod_after = ((double) (tod_record.tv_sec))
                                    + ((double) (tod_record.tv_usec)) / 1.0e6;

            tod_elap = tod_after - tod_before;

            char msg[1000];
            float fps = 0.0;     // frames per second
            if (tod_elap > 0)
                fps = ((float)N_FRAMES_PER_DISPLAY) / tod_elap;
            // We display an exponential moving average of the fps measurement
            fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0);
            sprintf(msg,"GrayScott - %0.2f fps", fps_avg);

            // display:
            {
                int quitnow;
                if (g_color) {
                    quitnow = display(red,green,blue,iteration,false,200.0f,2,10,msg);
                } else {
                    /* Simply show one dimension on all three channels. */
                    quitnow = display(red,red,red,iteration,false,200.0f,2,10,msg);
                }
                if (quitnow)
                    break;
            }
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
                a[i][j] = frand(0.0f,0.1f);
                b[i][j] = frand(0.9f,1.0f);
            }
            else 
            {
                a[i][j] = frand(0.9f,1.0f);
                b[i][j] = frand(0.0f,0.1f);
            }
            /*float v = frand(0.0f,1.0f);
            a[i][j] = v;
            b[i][j] = 1.0f-v;*/
        }
    }
}

