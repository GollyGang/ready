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

// stdlib
#include <stdio.h>

// local:
#include "defs.h"

static int g_wrap = 1;
static bool g_paramspace = 0;

bool display_gsd(double r[X][Y],double g[X][Y],double b[X][Y],
             int iteration,bool auto_brighten,double manual_brighten,
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
        im3 = cvCreateImage(cvSize(X*scale+border*2,Y*scale+border),IPL_DEPTH_8U,3);
        
        cvNamedWindow(title,CV_WINDOW_AUTOSIZE);
        
        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);

        if(write_video)
        {
            video = cvCreateVideoWriter(title,CV_FOURCC('D','I','V','X'),25.0,cvGetSize(im3),1);
            border = 20;
        }
    }

    // convert double arrays to IplImage for OpenCV to display
    double val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<X;i++)
        {
            for(int j=0;j<Y;j++)
            {
                if(r) {
                    val = r[i][j];
                    if(val<minR) minR=val; if(val>maxR) maxR=val;
                }
                if(g) {
                    val = g[i][j];
                    if(val<minG) minG=val; if(val>maxG) maxG=val;
                }
                if(b) {
                    val = b[i][j];
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
                double val = r[i][Y-j-1];
                if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 2] = (uchar)val;
            }
            if(g) {
                double val = g[i][Y-j-1];
                if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 1] = (uchar)val;
            }
            if(b) {
                double val = b[i][Y-j-1];
                if(auto_brighten) val = 255.0f * (val-minB) / (maxB-minB);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 0] = (uchar)val;
            }
        }
    }

    cvResize(im,im2);
    cvCopyMakeBorder(im2,im3,cvPoint(border*2,0),IPL_BORDER_CONSTANT);

    char txt[100];
    if(!write_video)
    {
        sprintf(txt,"%d",iteration);
        cvPutText(im3,txt,cvPoint(20,20),&font,white);

        // DEBUG:
        sprintf(txt,"%.4f,%.4f,%.4f",r[0][0],g[0][0],b[0][0]);
        //cvPutText(im3,txt,cvPoint(20,40),&font,white);
    }

    // DEBUG:
    if(write_video)
    {
        cvPutText(im3,"0.06",cvPoint(5,15),&font,white);
        cvPutText(im3,"F",cvPoint(5,im2->height/2),&font,white);
        cvPutText(im3,"0.00",cvPoint(5,im2->height),&font,white);
        cvPutText(im3,"0.03",cvPoint(border*2-10,im2->height+15),&font,white);
        cvPutText(im3,"K",cvPoint(border*2+im2->width/2,im2->height+15),&font,white);
        cvPutText(im3,"0.07",cvPoint(im3->width-35,im2->height+15),&font,white);
    }

    cvPutText(im3,message,cvPoint(20,40),&font,white);

    if(write_video)
        cvWriteFrame(video,im3);

    cvShowImage(title,im3);
    
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

void init(double a[X][Y],double b[X][Y]);

void compute(double a[X][Y],double b[X][Y],
             double da[X][Y],double db[X][Y],
             double r_a,double r_b,double f,double k,
             double speed,
             bool parameter_space);

int main(int argc, char * * argv)
{
    for (int i = 1; i < argc; i++) {
        if (0) {
        } else if (strcmp(argv[i],"-paramspace")==0) {
            // do a parameter space plot, like in the Pearson paper
            g_paramspace = true;
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
    double r_a = 0.082f;
    double r_b = 0.041f;

    // for spots:
    double k = 0.064f;
    double f = 0.035f;
    // for stripes:
    //double k = 0.06f;
    //double f = 0.035f;
    // for long stripes
    //double k = 0.065f;
    //double f = 0.056f;
    // for dots and stripes
    //double k = 0.064f;
    //double f = 0.04f;
    // for spiral waves:
    //double k = 0.0475f;
    //double f = 0.0118f;
    double speed = 1.0f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    double a[X][Y], b[X][Y];
    // these arrays store the rate of change of those chemicals:
    double da[X][Y], db[X][Y];

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
            compute(a,b,da,db,r_a,r_b,f,k,speed,g_paramspace);
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
        sprintf(msg,"GrayScott - %0.2f fps %0.2f Mcgs", fps_avg, Mcgs);

        // display:
        if(display_gsd(a,a,a,iteration,false,200.0f,2,10,msg)) // did user ask to quit?
            break;
    }
}

// return a random value between lower and upper
double frand(double lower,double upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void init(double a[X][Y],double b[X][Y])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            
            //if(hypot(i%10-5/*-X/2*/,j%10-5/*-Y/2*/)<=2.0f)//frand(2,3))
            if(hypot(i-X/2,(j-Y/2)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
            {
                a[i][j] = 0.0f;
                b[i][j] = 1.0f;
            }
            else {
                a[i][j] = 1;
                b[i][j] = 0;
            }
            //double v = frand(0.0f,1.0f);
            //a[i][j] = v;
            //b[i][j] = 1.0f-v;
            //a[i][j] += frand(-0.01f,0.01f);
            //b[i][j] += frand(-0.01f,0.01f);
        }
    }
}

void compute(double a[X][Y],double b[X][Y],
             double da[X][Y],double db[X][Y],
             double r_a,double r_b,double f,double k,double speed,
             bool parameter_space)
{
    // compute change in each cell
    for(int i = 0; i < X; i++) {
        int iprev,inext;
        if (g_wrap) {
            iprev = (i + X - 1) % X;
            inext = (i + 1) % X;
        } else {
            iprev = max(0,i-1);
            inext = min(X-1,i+1);
        }

        for(int j = 0; j < Y; j++) {
            int jprev,jnext;
            if (g_wrap) {
                jprev = (j + Y - 1) % Y;
                jnext = (j + 1) % Y;
            } else {
                jprev = max(0,j-1);
                jnext = min(Y-1,j+1);
            }

            double aval = a[i][j];
            double bval = b[i][j];

            if (parameter_space) {
                const double kmin=0.045f,kmax=0.07f,fmin=0.01f,fmax=0.09f;
                // set f and k for this location (ignore the provided values of f and k)
                k = kmin + i*(kmax-kmin)/X;
                f = fmin + j*(fmax-fmin)/Y;
            }

            // compute the Laplacians of a and b
            double dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            double ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

            // compute the new rate of change of a and b
            da[i][j] = r_a * dda - aval*bval*bval + f*(1-aval);
            db[i][j] = r_b * ddb + aval*bval*bval - (f+k)*bval;
        }
    }

    // effect change
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] += speed * da[i][j];
            b[i][j] += speed * db[i][j];
            // kill denormals by adding a teeny tiny something (http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.20.1348&rank=4)
            a[i][j] += 1e-20;
            b[i][j] += 1e-20;
        }
    }
}

