/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

// OpenCV:
#include <cv.h>
#include <highgui.h>

// local:
#include "defs.h"
#include "display.h"

void init(IplImage *a,IplImage *b);

void compute(IplImage *a,IplImage *b,
             IplImage *t1,IplImage *t2,
             IplImage *abb,
             float r_a,float r_b,float f,float k,
             float speed);

bool display(IplImage *r,IplImage *g,IplImage *b,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message);

static int g_wrap = 0;
static bool g_paramspace = 0;

int main(int argc, char * * argv)
{
    for (int i = 1; i < argc; i++) {
        if (0) {
        } else if (strcmp(argv[i],"-wrap")==0) {
            // patterns wrap around ("torus", also called "continuous boundary
            // condition")
            g_wrap = 1;
        } else {
            fprintf(stderr, "Unrecognized argument: '%s'\n", argv[i]);
            exit(-1);
        }
    }

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
    IplImage *a,*b;

    // temp images:
    IplImage *t1,*t2,*abb;

    a = cvCreateImage(cvSize(X,Y),IPL_DEPTH_32F,1);
    b = cvCreateImage(cvSize(X,Y),IPL_DEPTH_32F,1);
    t1 = cvCreateImage(cvSize(X,Y),IPL_DEPTH_32F,1);
    t2 = cvCreateImage(cvSize(X,Y),IPL_DEPTH_32F,1);
    abb = cvCreateImage(cvSize(X,Y),IPL_DEPTH_32F,1);

    // put the initial conditions into each cell
    init(a,b);

    const int N_FRAMES_PER_DISPLAY = 200;
    int iteration = 0;
    double fps_avg = 0.0; // decaying average of fps
    while(true) 
    {
        struct timeval tod_record;
        double tod_before, tod_after, tod_elapsed;
        double fps = 0.0;     // frames per second

        gettimeofday(&tod_record, 0);
        tod_before = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

        // compute:
        for(int it=0;it<N_FRAMES_PER_DISPLAY;it++)
        {
            compute(a,b,t1,t2,abb,r_a,r_b,f,k,speed);
            iteration++;
        }

        gettimeofday(&tod_record, 0);
        tod_after = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

        tod_elapsed = tod_after - tod_before;
        fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elapsed;
        // We display an exponential moving average of the fps measurement
        fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0);
		double Mcgs = fps_avg * ((double) X) * ((double) Y) / 1.0e6;

        char msg[1000];
        sprintf(msg, "GrayScott - %0.2f fps %0.2f Mcgs", fps_avg, Mcgs);

        // display:
        if(display(a,a,a,iteration,false,200.0f,2,10,msg)) // did user ask to quit?
            break;
    }

    cvReleaseImage(&a);
    cvReleaseImage(&b);
    cvReleaseImage(&t1);
    cvReleaseImage(&t2);
    cvReleaseImage(&abb);
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

void init(IplImage *a,IplImage *b)
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            
            if(hypot(i%50-25/*-X/2*/,j%50-25/*-Y/2*/)<=frand(2,5))
            //if(hypot(i-X/2,(j-Y/2)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
            {
                cvSet2D(a,i,j,cvScalar(0.0f));
                cvSet2D(b,i,j,cvScalar(1.0f));
            }
            else 
            {
                cvSet2D(a,i,j,cvScalar(1.0f));
                cvSet2D(b,i,j,cvScalar(0.0f));
            }
        }
    }
}

// a += speed * ( r_a * dda - aval*bval*bval + f*(1-aval) );
// b += speed * ( r_b * ddb + aval*bval*bval - (f+k)*bval );
void compute(IplImage *a,IplImage *b,
             IplImage *t1,IplImage *t2,
             IplImage *abb,
             float r_a,float r_b,float f,float k,float speed)
{
    cvMul(a,b,abb);
    cvMul(abb,b,abb);
    cvLaplace(a,t1,1);
    cvLaplace(b,t2,1);
    cvAddWeighted(t1,r_a,abb,-1.0,0.0,t1);
    cvAddWeighted(t2,r_b,abb,1.0,0.0,t2);
    cvAddWeighted(t1,1.0,a,-f,f,t1);
    cvAddWeighted(t2,1.0,b,-(f+k),0.0,t2);
    cvAddWeighted(a,1.0,t1,speed,1e-10f,a); // adding a teeny-tiny bit to avoid denormals
    cvAddWeighted(b,1.0,t2,speed,1e-10f,b);
}

bool display(IplImage *r,IplImage *g,IplImage *b,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message)
{
    static bool need_init = true;
    static bool write_video = false;

    static IplImage *im,*im2,*float_im;
    static CvFont font;
    static CvVideoWriter *video;
    static const CvScalar white = cvScalar(255,255,255);

    const char *title = "Press ESC to quit";

    if(need_init)
    {
        need_init = false;

        im = cvCreateImage(cvSize(X,Y),IPL_DEPTH_8U,3);
        float_im = cvCreateImage(cvSize(X,Y),IPL_DEPTH_32F,3);
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

    cvMerge(r,g,b,NULL,float_im);
    cvConvertScale(float_im,im,manual_brighten);
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

