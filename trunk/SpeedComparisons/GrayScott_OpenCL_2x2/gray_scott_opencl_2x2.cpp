/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// OpenCV:
#include <cv.h>
#include <highgui.h>

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

void init(float a[X][Y],float b[X][Y]);
bool display(float r[X][Y],float g[X][Y],float b[X][Y],
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message);

// we pack the values in 2x2 blocks:   x y
//                                     z w
float& float_at(float* arr,int x,int y) 
{
    return arr[ ( (x/2)*(Y/2) + y/2 ) * 4 + (y%2)*2 + x%2 ]; 
}

static int g_opt_device = 0;
static int g_wrap = 1;

int main(int argc, char * * argv)
{
    for (int i = 1; i < argc; i++) {
        if (0) {
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
    
    float a[X][Y], b[X][Y];
    const int MEM_SIZE = sizeof(float)*X*Y;

    // put the initial conditions into each cell
    init(a,b);

    try { 
        // Get available OpenCL platforms
        cl::vector<Platform> platforms;
        Platform::get(&platforms);
 
        // Select the default platform and create a context using this platform and the GPU
        cl_context_properties cps[3] = { 
            CL_CONTEXT_PLATFORM, 
            (cl_context_properties)(platforms[0])(), 
            0 
        };
        Context context( CL_DEVICE_TYPE_GPU, cps);
 
        // Get a list of devices on this platform
        cl::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        int maxdev = devices.size() - 1;
        g_opt_device = (g_opt_device > maxdev) ? maxdev :
                                     ((g_opt_device < 0) ? 0 : g_opt_device);
        std::cout << (maxdev+1) << " device(s) available; using device "
                                                    << g_opt_device << ".\n";
        Device &device = devices[g_opt_device];

        CommandQueue queue = CommandQueue(context, device);
        Event event;
 
        // Read source file
        // (CL_SOURCE_DIR is defined in CMakeLists.txt to be the folder
        // containing the source files, including this file 'gray_scott_opencl_2x2.cpp'
        // and the kernel source 'gs_wrap_kernel_2x2.cl'.)
        std::string kfn = CL_SOURCE_DIR;
        kfn += "/grayscott_kernel_2x2.cl";
        std::ifstream sourceFile(kfn.c_str());
        std::string sourceCode(
            std::istreambuf_iterator<char>(sourceFile),
            (std::istreambuf_iterator<char>()));
        Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));


        // enable this code to display kernel compilation error if you get clBuildProgram(-11)
        #if 0
           const ::size_t n = (::size_t)source.size();
           ::size_t* lengths = (::size_t*) alloca(n * sizeof(::size_t));
           const char** strings = (const char**) alloca(n * sizeof(const char*));
           for (::size_t i = 0; i < n; ++i) {
               strings[i] = source[(int)i].first;
               lengths[i] = source[(int)i].second;
           }
           cl_int err;
           cl_program myprog = clCreateProgramWithSource(context(), (cl_uint)n, strings, lengths, &err);
           err = clBuildProgram(myprog, (cl_uint)devices.size(), (cl_device_id*)&devices.front(), NULL, NULL, NULL);
           char proglog[1024];
           clGetProgramBuildInfo(myprog, device(), CL_PROGRAM_BUILD_LOG, 1024, proglog, 0);
           printf("err=%d log=%s\n", err, proglog);
           return 0;
        #endif
         
 
        // Make program of the source code in the context
        Program program = Program(context, source);
 
        // Build program for these specific devices
        // If wrap (toroidal) option is selected, we define a preprocessor flag
        // that controls how the xm1, xp1, etc. are computed.
        program.build(devices, g_wrap ? "-D WRAP" : NULL, NULL, NULL);
 
        // Make kernel
        Kernel kernel(program, "grayscott_compute");

        // Create memory buffers
        Buffer bufferU = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferV = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferU2 = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);
        Buffer bufferV2 = Buffer(context, CL_MEM_READ_ONLY, MEM_SIZE);

        // Copy lists A and B to the memory buffers
        queue.enqueueWriteBuffer(bufferU, CL_TRUE, 0, MEM_SIZE, a);
        queue.enqueueWriteBuffer(bufferV, CL_TRUE, 0, MEM_SIZE, b);
 
        NDRange global(X/2,Y/2);
        NDRange local(1,256);

        kernel.setArg(4, k);
        kernel.setArg(5, f);
        kernel.setArg(6, r_a);
        kernel.setArg(7, r_b);
        kernel.setArg(8, speed);

        int iteration = 0;
        float fps_avg = 0.0; // decaying average of fps
        const int N_FRAMES_PER_DISPLAY = 2000;  // an even number, because of our double-buffering implementation
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

            queue.enqueueReadBuffer(bufferU, CL_TRUE, 0, MEM_SIZE, a);

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
                double Mcgs = (fps_avg * ((double)X) * ((double)Y)) / 1.0e6;
            sprintf(msg,"GrayScott - %0.2f fps %0.2f Mcgs", fps_avg, Mcgs);

            // display:
            if(display(a,a,a,iteration,false,200.0f,1,10,msg))
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
                float_at((float*)a,i,j) = frand(0.0f,0.1f);
                float_at((float*)b,i,j) = frand(0.9f,1.0f);
            }
            else 
            {
                float_at((float*)a,i,j) = frand(0.9f,1.0f);
                float_at((float*)b,i,j) = frand(0.0f,0.1f);
            }
        }
    }
}

bool display(float r[X][Y],float g[X][Y],float b[X][Y],
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message)
{
    static bool need_init = true;
    static bool write_video = false;

    static IplImage *im,*im2;
    static CvFont font;
    static CvVideoWriter *video;
    static const CvScalar white = cvScalar(255,255,255);

    const char *title = "Press ESC to quit";

    if(need_init)
    {
        need_init = false;

        im = cvCreateImage(cvSize(X,Y),IPL_DEPTH_8U,3);
        cvSet(im,cvScalar(0,0,0));
        im2 = cvCreateImage(cvSize(X*scale,Y*scale),IPL_DEPTH_8U,3);
        
        cvNamedWindow(title,CV_WINDOW_AUTOSIZE);
        
        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

        if(write_video)
        {
            video = cvCreateVideoWriter(title,CV_FOURCC('D','I','V','X'),25.0,cvGetSize(im2),1);
        }
    }

    // convert float arrays to IplImage for OpenCV to display
    float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<X;i++)
        {
            for(int j=0;j<Y;j++)
            {
                if(r) {
                    val = float_at((float*)r,i,j);
                    if(val<minR) minR=val; if(val>maxR) maxR=val;
                }
                if(g) {
                    val = float_at((float*)g,i,j);
                    if(val<minG) minG=val; if(val>maxG) maxG=val;
                }
                if(b) {
                    val = float_at((float*)b,i,j);
                    if(val<minB) minB=val; if(val>maxB) maxB=val;
                }
            }
        }
    }
    #pragma omp parallel for
    for(int i=0;i<X;i++)
    {
        for(int j=0;j<Y;j++)
        {
            if(r) {
                float val = float_at((float*)r,i,j);//Y-j-1);
                if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 2] = (uchar)val;
            }
            if(g) {
                float val = float_at((float*)g,i,j);//Y-j-1);
                if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 1] = (uchar)val;
            }
            if(b) {
                float val = float_at((float*)b,i,j);//Y-j-1);
                if(auto_brighten) val = 255.0f * (val-minB) / (maxB-minB);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 0] = (uchar)val;
            }
        }
    }

    cvResize(im,im2);

    char txt[100];
    if(!write_video)
    {
        sprintf(txt,"%d",iteration);
        cvPutText(im2,txt,cvPoint(20,20),&font,white);
    }

    cvPutText(im2,message,cvPoint(20,40),&font,white);

    if(write_video)
        cvWriteFrame(video,im2);

    cvShowImage(title,im2);
    
    int key = cvWaitKey(delay_ms); // allow time for the image to be drawn
    if(key==27) // did user ask to quit?
    {
        cvDestroyWindow(title);
        cvReleaseImage(&im);
        cvReleaseImage(&im2);
        if(write_video)
            cvReleaseVideoWriter(&video);
        return true;
    }
    return false;
}
