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

void init(float *a,float *b);
bool display(float *a,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message);

// we pack the values in 2x2 blocks:   x y
//                                     z w
float& float_at(float* arr,int x,int y) 
{
    return arr[ ( (y/2)*(X/2) + x/2 ) * 4 + (y%2)*2 + x%2 ]; 
}

int main()
{
    // Here we implement the Gray-Scott model, as described here:
    // http://www.cc.gatech.edu/~turk/bio_sim/hw3.html
    // http://arxiv.org/abs/patt-sol/9304003

    // -- parameters --
    float r_a = 0.082f;
    float r_b = 0.041f;

    float k, f;
    k = 0.064f; f = 0.035f; // solitons with mitosis (spots that multiply)
    // k = 0.06f; f = 0.035f; // stripes
    //k = 0.065f; f = 0.056f; // long stripes
    //k = 0.064f; f = 0.04f; // dots and stripes
    // k = 0.0475f; f = 0.0118f; // spiral waves
    float speed = 2.0f;
    // ----------------
    
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

        float *a = new float[X*Y];
        float *b = new float[X*Y];
        init(a,b);

        // Get a list of devices on this platform
        cl::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        bool is_ImageSupported = devices[0].getInfo<CL_DEVICE_IMAGE_SUPPORT>();
        if(!is_ImageSupported)
        {
            printf("Images not supported on this device.\n");
            throw;
        }

        // we make two images and swap between them
        cl::Image2D a1(context,CL_MEM_READ_WRITE,cl::ImageFormat(CL_RGBA,CL_FLOAT),X/2,Y/2);
        cl::Image2D b1(context,CL_MEM_READ_WRITE,cl::ImageFormat(CL_RGBA,CL_FLOAT),X/2,Y/2);
        cl::Image2D a2(context,CL_MEM_READ_WRITE,cl::ImageFormat(CL_RGBA,CL_FLOAT),X/2,Y/2);
        cl::Image2D b2(context,CL_MEM_READ_WRITE,cl::ImageFormat(CL_RGBA,CL_FLOAT),X/2,Y/2);

        // Create a command queue and use the selected device
        CommandQueue queue = CommandQueue(context, devices[0]);
        Event event;
 
        // Copy to the memory buffers
        cl::size_t<3> origin;
        origin.push_back(0);
        origin.push_back(0);
        origin.push_back(0);
        cl::size_t<3> region;
        region.push_back(X/2);
        region.push_back(Y/2);
        region.push_back(1);
        queue.enqueueWriteImage(a1,true,origin,region,0,0,a);
        queue.enqueueWriteImage(b1,true,origin,region,0,0,b);
 
        // Read source file
        std::string kfn = CL_SOURCE_DIR; // (defined in CMakeLists.txt to be the source folder)
        kfn += "/grayscott_kernel_image_2x2.cl";
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
        Kernel kernel(program, "grayscott_compute_2x2");

        NDRange global(X/2,Y/2);
        NDRange local(16,16);

        kernel.setArg(4, f);
        kernel.setArg(5, f+k);
        kernel.setArg(6, r_a);
        kernel.setArg(7, r_b);
        kernel.setArg(8, speed);

        int iteration = 0;
        const int N_FRAMES_PER_DISPLAY = 5000;  // an even number, because of our double-buffering implementation
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

                kernel.setArg(0, a1);
                kernel.setArg(1, b1);
                kernel.setArg(2, a2);
                kernel.setArg(3, b2);
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);

                kernel.setArg(0, a2);
                kernel.setArg(1, b2);
                kernel.setArg(2, a1);
                kernel.setArg(3, b1);
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);
            }
            iteration += N_FRAMES_PER_DISPLAY;

            // read back into a buffer
            queue.enqueueReadImage(a1,true,origin,region,0,0,a);
            //queue.enqueueReadImage(b1,true,origin,region,0,0,b);

            gettimeofday(&tod_record, 0);
            tod_after = ((double) (tod_record.tv_sec))
                                    + ((double) (tod_record.tv_usec)) / 1.0e6;

            tod_elap = tod_after - tod_before;

            char msg[1000];
            float fps = 0.0f;     // frames per second
            if (tod_elap > 0)
                fps = N_FRAMES_PER_DISPLAY / (float)tod_elap;
            sprintf(msg,"GrayScott - %0.2f fps (%.2f Mcgs)",fps,fps*X*Y/1e6);

            // display:
            {
                int quitnow = display(a,iteration,false,200.0f,2,10,msg);
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

void init(float* a,float* b)
{
    srand((unsigned int)time(NULL));

    // figure the values
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            // start with a uniform field with an approximate circle in the middle
            if(hypot(i-X/3,(j-Y/4)/1.5)<=frand(2,5))
            {
                float_at(a,i,j) = frand(0.0f,0.1f);
                float_at(b,i,j) = frand(0.9f,1.0f);
            }
            else 
            {
                float_at(a,i,j) = frand(0.9f,1.0f);
                float_at(b,i,j) = frand(0.0f,0.1f);
            }

        }
    }
}


bool display(float *a,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message)
{
    static bool need_init = true;
    static bool write_video = false;

    static IplImage *im,*im2,*im3;
    static int border = 0;
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
    }

    // convert float arrays to IplImage for OpenCV to display
    for(int col=0;col<X;col++)
    {
        for(int row=0;row<Y;row++)
        {
            float val = float_at(a,col,row);
            val *= manual_brighten;
            if(val<0) val=0; if(val>255) val=255;
            ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 2] = (uchar)val;
            ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 1] = (uchar)val;
            ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 0] = (uchar)val;
        }
    }

    cvResize(im,im2);

    {
        char txt[100];
        sprintf(txt,"%d",iteration);
        cvPutText(im2,txt,cvPoint(20,20),&font,white);
        cvPutText(im2,message,cvPoint(20,40),&font,white);
    }

    cvShowImage(title,im2);

    int key = cvWaitKey(delay_ms); // allow time for the image to be drawn
    if(key==27) // did user ask to quit?
    {
        cvDestroyWindow(title);
        cvReleaseImage(&im);
        cvReleaseImage(&im2);
        return true;
    }
    return false;
}
