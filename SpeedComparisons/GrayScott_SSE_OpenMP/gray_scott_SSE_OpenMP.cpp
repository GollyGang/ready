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

// SSE:
#include <xmmintrin.h>

// OpenCV:
#include <cv.h>
#include <highgui.h>

// local:
#include "defs.h"
//#include "display.h"

// OpenMP:
#include <omp.h>

inline int at(int x,int y) { return x*Y+y; }
const int FLOATS_PER_BLOCK = 16 / sizeof(float); // 4 single-precision floats fit in a 128-bit (16-byte) SSE block
const int X_BLOCKS = X / FLOATS_PER_BLOCK; // 4 horizontally-neighboring cells form an SSE block
const int Y_BLOCKS = Y;
const int TOTAL_BLOCKS = X_BLOCKS * Y_BLOCKS;
inline int block_at(int x,int y) { return x*Y_BLOCKS+y; }

void init(float *a,float *b,float *da,float *db);
void compute(float *a,float *b,float *da,float *db,
             float r_a,float r_b,float f,float k,
             float speed);
bool display(float *r,float *g,float *b,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message);

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

    const int n_cells = X*Y;
    float *a = (float*)_mm_malloc(n_cells*sizeof(float),16);
    float *b = (float*)_mm_malloc(n_cells*sizeof(float),16);
    float *da = (float*)_mm_malloc(n_cells*sizeof(float),16);
    float *db = (float*)_mm_malloc(n_cells*sizeof(float),16);

    init(a,b,da,db);

    const int N_FRAMES_PER_DISPLAY = 1000;
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
            compute(a,b,da,db,r_a,r_b,f,k,speed);
            iteration++;
        }

        gettimeofday(&tod_record, 0);
        tod_after = ((double) (tod_record.tv_sec))
                                    + ((double) (tod_record.tv_usec)) / 1.0e6;

        tod_elapsed = tod_after - tod_before;
        fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elapsed;
        // We display an exponential moving average of the fps measurement
        fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0);

        char msg[1000];
        sprintf(msg,"GrayScott - %0.2f fps",fps_avg);

        // display:
        if(display(a,a,a,iteration,false,200.0f,2,10,msg)) // did user ask to quit?
            break;
    }

    _mm_free(a);
    _mm_free(b);
    _mm_free(da);
    _mm_free(db);
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

void init(float *a,float *b,float *da,float *db)
{
#if (defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || 	defined(_M_X64) || defined(_M_IX86))
  /* On Intel we disable accurate handling of denorms and zeros. This is an
     important speed optimization. */
  int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
  int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
  _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
#endif

    srand((unsigned int)time(NULL));

    // figure the values
    float val=1.0f;
    for(int i = 0; i < X; i++)
    {
        for(int j = 0; j < Y; j++)
        {
            //if(hypot(i%50-25/*-X/2*/,j%50-25/*-Y/2*/)<=frand(2,5))
            if(hypot(i-X/2,(j-Y/2)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
            {
                a[at(i,j)] = 0.0f;
                b[at(i,j)] = 1.0f;
            }
            else {
                a[at(i,j)] = 1;
                b[at(i,j)] = 0;
            }
            da[at(i,j)]=0.0f;
            db[at(i,j)]=0.0f;
        }
    }
}

void compute(float *a,float *b,float *da,float *db,
             float r_a,float r_b,float f,float k,
             float speed)
{
    const __m128 r_a_SSE = _mm_set1_ps(r_a);
    const __m128 r_b_SSE = _mm_set1_ps(r_b);
    const __m128 f_SSE = _mm_set1_ps(f);
    const __m128 k_SSE = _mm_set1_ps(k);
    const __m128 speed_SSE = _mm_set1_ps(speed);
    const __m128 one_SSE = _mm_set1_ps(1.0f);
    const __m128 minus_four_SSE = _mm_set1_ps(-4.0f);

    // compute the rates of change
    #pragma omp parallel for
    for(int i=1;i<X_BLOCKS-1;i++) // we skip the left- and right-most blocks for now (for simplicity)
    {
        for(int j=1;j<Y_BLOCKS-1;j++) // we skip the top- and bottom-most blocks for now
        {
            __m128 *a_SSE = ((__m128*)a)+block_at(i,j);
            __m128 *b_SSE = ((__m128*)b)+block_at(i,j);
            __m128 *da_SSE = ((__m128*)da)+block_at(i,j);
            __m128 *db_SSE = ((__m128*)db)+block_at(i,j);
            // retrieve the neighboring cells
            __m128 a_left = _mm_loadu_ps(((float*)a_SSE)-1);
            __m128 a_right = _mm_loadu_ps(((float*)a_SSE)+1);
            __m128 *a_above = a_SSE-X_BLOCKS;
            __m128 *a_below = a_SSE+X_BLOCKS;
            __m128 b_left = _mm_loadu_ps(((float*)b_SSE)-1);
            __m128 b_right = _mm_loadu_ps(((float*)b_SSE)+1);
            __m128 *b_above = b_SSE-X_BLOCKS;
            __m128 *b_below = b_SSE+X_BLOCKS;
            // find the Laplacian of a (using the von Neumann neighborhood)
            __m128 dda_SSE = _mm_mul_ps(*a_SSE,minus_four_SSE);
            dda_SSE = _mm_add_ps(dda_SSE,a_left);
            dda_SSE = _mm_add_ps(dda_SSE,a_right);
            dda_SSE = _mm_add_ps(dda_SSE,*a_above);
            dda_SSE = _mm_add_ps(dda_SSE,*a_below);
            // find the Laplacian of b
            __m128 ddb_SSE = _mm_mul_ps(*b_SSE,minus_four_SSE);
            ddb_SSE = _mm_add_ps(ddb_SSE,b_left);
            ddb_SSE = _mm_add_ps(ddb_SSE,b_right);
            ddb_SSE = _mm_add_ps(ddb_SSE,*b_above);
            ddb_SSE = _mm_add_ps(ddb_SSE,*b_below);
            // compute the new rates of changes
            __m128 t1_SSE = _mm_mul_ps(r_a_SSE,dda_SSE); // t1 = r_a * dda
            __m128 t2_SSE = _mm_mul_ps(*a_SSE,*b_SSE); // t2 = aval*bval
            __m128 t3_SSE = _mm_mul_ps(t2_SSE,*b_SSE); // t3 = aval*bval*bval
            __m128 t4_SSE = _mm_sub_ps(one_SSE,*a_SSE); // t4 = 1-aval
            __m128 t5_SSE = _mm_mul_ps(f_SSE,t4_SSE); // t5 = f * (1-aval)
            __m128 t6_SSE = _mm_sub_ps(t1_SSE,t3_SSE); // t6 = r_a *dda - aval*bval*bval
            *da_SSE = _mm_add_ps(t6_SSE,t5_SSE); // da = r_a * dda - aval*bval*bval + f*(1-aval)
            t1_SSE = _mm_mul_ps(r_b_SSE,ddb_SSE); // t1 = r_b * ddb
            t2_SSE = _mm_add_ps(f_SSE,k_SSE); // t2 = f + k
            t3_SSE = _mm_mul_ps(t2_SSE,*b_SSE); // t3 = (f+k) * bval
            t4_SSE = _mm_mul_ps(*a_SSE,*b_SSE); // t4 = aval*bval
            t5_SSE = _mm_mul_ps(t4_SSE,*b_SSE); // t5 = aval*bval*bval
            t6_SSE = _mm_add_ps(t1_SSE,t5_SSE); // t6 = r_b*ddb + aval*bval*bval
            *db_SSE = _mm_sub_ps(t6_SSE,t3_SSE); // db = r_b*ddb + aval*bval*bval - (f+k)*bval
        }
    }

    // apply the rate of change
    #pragma omp parallel for
    for(int i=0;i<TOTAL_BLOCKS;i++)
    {
        __m128 *a_SSE = ((__m128*)a)+i;
        __m128 *b_SSE = ((__m128*)b)+i;
        __m128 *da_SSE = ((__m128*)da)+i;
        __m128 *db_SSE = ((__m128*)db)+i;
        // a[i] += speed * da[i];
        __m128 t1_SSE = _mm_mul_ps(*da_SSE,speed_SSE);
        *a_SSE = _mm_add_ps(*a_SSE,t1_SSE);
        // b[i] += speed * db[i];
        t1_SSE = _mm_mul_ps(*db_SSE,speed_SSE);
        *b_SSE = _mm_add_ps(*b_SSE,t1_SSE);
    }
}

bool display(float *r,float *g,float *b,
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

    // convert float arrays to IplImage for OpenCV to display
    float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<X;i++)
        {
            for(int j=0;j<Y;j++)
            {
                if(r) {
                    val = r[at(i,j)];
                    if(val<minR) minR=val; if(val>maxR) maxR=val;
                }
                if(g) {
                    val = g[at(i,j)];
                    if(val<minG) minG=val; if(val>maxG) maxG=val;
                }
                if(b) {
                    val = b[at(i,j)];
                    if(val<minB) minB=val; if(val>maxB) maxB=val;
                }
            }
        }
    }
    for(int i=0;i<X;i++)
    {
        for(int j=0;j<Y;j++)
        {
            if(r) {
                float val = r[at(i,Y-j-1)];
                if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 2] = (uchar)val;
            }
            if(g) {
                float val = g[at(i,Y-j-1)];
                if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + j*im->widthStep))[i*im->nChannels + 1] = (uchar)val;
            }
            if(b) {
                float val = b[at(i,Y-j-1)];
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
