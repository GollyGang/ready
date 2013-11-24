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

void init(float *chemicals);
bool display(float *chemicals,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message);

static int g_opt_device = 0;

int main(int argc, char * * argv)
{
    for (int i = 1; i < argc; i++) {
        if (0) {
        } else if ((i+1<argc) && (strcmp(argv[i],"-device")==0)) {
            // select an output device
            i++; g_opt_device = atoi(argv[i]);
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

        float *chem1 = new float[X*Y*4]; // we store up to 4 chemicals in an RGBA image
        float *chem2 = new float[X*Y*4];
        init(chem1);

        // Get a list of devices on this platform
        cl::vector<Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        // range-check the user's selection
        int maxdev = devices.size() - 1;
        g_opt_device = (g_opt_device > maxdev) ? maxdev :
                                     ((g_opt_device < 0) ? 0 : g_opt_device);
        std::cout << (maxdev+1) << " device(s) available; using device "
                                                    << g_opt_device << ".\n";

        Device &device = devices[g_opt_device];
        cl::vector<Device> ourdevices = cl::vector<Device>(1, device);


        bool is_ImageSupported = device.getInfo<CL_DEVICE_IMAGE_SUPPORT>();
        if(!is_ImageSupported)
        {
            printf("Images not supported on this device.\n");
            exit(-1);
        }

        // we make two images and swap between them
        cl::Image2D chemicals1(context,CL_MEM_READ_WRITE,cl::ImageFormat(CL_RGBA,CL_FLOAT),X,Y);
        cl::Image2D chemicals2(context,CL_MEM_READ_WRITE,cl::ImageFormat(CL_RGBA,CL_FLOAT),X,Y);

        // Create a command queue and use the selected device
        CommandQueue queue = CommandQueue(context, device);
        Event event;
 
        // Copy to the memory buffers
        cl::size_t<3> origin;
        origin.push_back(0);
        origin.push_back(0);
        origin.push_back(0);
        cl::size_t<3> region;
        region.push_back(X);
        region.push_back(Y);
        region.push_back(1);
        queue.enqueueWriteImage(chemicals1,true,origin,region,0,0,chem1);
        queue.enqueueWriteImage(chemicals2,true,origin,region,0,0,chem2);
 
        // Read source file
        std::string kfn = CL_SOURCE_DIR; // (defined in CMakeLists.txt to be the source folder)
        kfn += "/grayscott_kernel_image.cl";
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
           err = clBuildProgram(myprog, (cl_uint)ourdevices.size(), (cl_device_id*)&ourdevices.front(), NULL, NULL, NULL);
           char proglog[1024];
           clGetProgramBuildInfo(myprog, device(), CL_PROGRAM_BUILD_LOG, 1024, proglog, 0);
           printf("err=%d log=%s\n", err, proglog);
           return 0;
        #endif

        // Make program of the source code in the context
        Program program = Program(context, source);
 
        // Build program for the specific device we are using
        // IMPORTANT: If this program is running on a system that has multiple
        // graphics cards, and if ANY of those cards does not support Images,
        // then "program.build(devices);" will fail. Thus, we must build only
        // on the device that is actually being used for the command queue.
        program.build(ourdevices);
 
        // Make kernel
        Kernel kernel(program, "grayscott_compute");

        NDRange global(X,Y);
        NDRange local(16,16);

        kernel.setArg(2, f);
        kernel.setArg(3, f+k);
        kernel.setArg(4, r_a);
        kernel.setArg(5, r_b);
        kernel.setArg(6, speed);

        int iteration = 0;
        float fps_avg = 0.0; // decaying average of fps
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

                kernel.setArg(0, chemicals1);
                kernel.setArg(1, chemicals2);
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);

                kernel.setArg(0, chemicals2);
                kernel.setArg(1, chemicals1);
                queue.enqueueNDRangeKernel(kernel, NullRange, global, local);
            }
            iteration += N_FRAMES_PER_DISPLAY;

            // read back into a buffer
            queue.enqueueReadImage(chemicals1,true,origin,region,0,0,chem1);
            //queue.enqueueReadImage(chemicals2,true,origin,region,0,0,chem2);

            gettimeofday(&tod_record, 0);
            tod_after = ((double) (tod_record.tv_sec))
                                    + ((double) (tod_record.tv_usec)) / 1.0e6;

            tod_elap = tod_after - tod_before;

            char msg[1000];
            float fps = 0.0f;     // frames per second
            if (tod_elap > 0)
                fps = N_FRAMES_PER_DISPLAY / (float)tod_elap;
            // We display an exponential moving average of the fps measurement
            fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0f) + fps) / 11.0f);
            sprintf(msg,"GrayScott - %0.2f fps (%.2f Mcgs)", fps_avg,fps_avg*X*Y/1e6);

            // display:
            {
                int quitnow = display(chem1,iteration,false,200.0f,2,10,msg);
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

void init(float* chemicals)
{
    srand((unsigned int)time(NULL));

    // figure the values
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            int index = (j*X+i)*4;
            // start with a uniform field with an approximate circle in the middle
            if(hypot(i-X/3,(j-Y/4)/1.5)<=frand(2,5))
            {
                chemicals[index+0] = frand(0.0f,0.1f);
                chemicals[index+1] = frand(0.9f,1.0f);
            }
            else 
            {
                chemicals[index+0] = frand(0.9f,1.0f);
                chemicals[index+1] = frand(0.0f,0.1f);
            }

        }
    }
}


bool display(float *chemicals,
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
            float val = chemicals[(row*X+col)*4 + 0];
            val *= manual_brighten;
            if(val<0) val=0; if(val>255) val=255;
            ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 2] = (uchar)val;
            val = chemicals[(row*X+col)*4 + 0];
            val *= manual_brighten;
            if(val<0) val=0; if(val>255) val=255;
            ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 1] = (uchar)val;
            val = chemicals[(row*X+col)*4 + 0];
            val *= manual_brighten;
            if(val<0) val=0; if(val>255) val=255;
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
