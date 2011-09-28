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

inline int at(int x,int y) { return y*X+x; }
const int FLOATS_PER_BLOCK = 16 / sizeof(float); // 4 single-precision floats fit in a 128-bit (16-byte) SSE block
const int X_BLOCKS = X / FLOATS_PER_BLOCK; // 4 horizontally-neighboring cells form an SSE block
const int Y_BLOCKS = Y;
const int TOTAL_BLOCKS = X_BLOCKS * Y_BLOCKS;
inline int block_at(int x,int y) { return y*X_BLOCKS+x; }

void init(float *a,float *b,float *da,float *db);
void compute(float *a,float *b,float *da,float *db,
             const float r_a,const float r_b,const float f,const float k,
             const float speed);
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

// some macros to make the next macros slightly more readable
#define ADD(a,b) _mm_add_ps(a,b)
#define MUL(a,b) _mm_mul_ps(a,b)
#define SUB(a,b) _mm_sub_ps(a,b)
#define SET(a) _mm_set1_ps(a)

// we write the Gray-Scott computation as macros because functions and __m128 don't mix well

#define LAPLACIAN(_a,_a_left,_a_right,_a_above,_a_below) ADD(ADD(ADD(ADD(MUL(_a,SET(-4.0f)),_a_left),_a_right),_a_above),_a_below)

#define GRAYSCOTT_DA(_a,_abb,_a_left,_a_right,_a_above,_a_below,_r_a,_f) ADD(SUB(MUL(_r_a,LAPLACIAN(_a,_a_left,_a_right,_a_above,_a_below)),_abb),MUL(_f,SUB(SET(1.0f),_a)));
// da = r_a*dda - aval*bval*bval + f*(1-aval)

#define GRAYSCOTT_DB(_b,_abb,_b_left,_b_right,_b_above,_b_below,_r_b,_f_plus_k) SUB(ADD(MUL(_r_b,LAPLACIAN(_b,_b_left,_b_right,_b_above,_b_below)),_abb),MUL(_f_plus_k,_b));
// db = r_b*ddb + aval*bval*bval - (f+k)*bval

void compute(float *a,float *b,float *da,float *db,
             const float r_a,const float r_b,const float f,const float k,
             const float speed)
{
    const __m128 r_a_SSE = SET(r_a);
    const __m128 r_b_SSE = SET(r_b);
    const __m128 f_SSE = SET(f);
    const __m128 f_plus_k_SSE = SET(f+k);
    const __m128 speed_SSE = SET(speed);

    // compute the rates of change
    #pragma omp parallel for
    for(int j=1;j<Y_BLOCKS-1;j++) // we skip the top- and bottom-most blocks
    {
        for(int i=1;i<X_BLOCKS-1;i++) // we skip the left- and right-most blocks
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
            __m128 abb = _mm_mul_ps(_mm_mul_ps(*a_SSE,*b_SSE),*b_SSE);
            // compute the new rates of change of each chemical
            *da_SSE = GRAYSCOTT_DA(*a_SSE,abb,a_left,a_right,*a_above,*a_below,r_a_SSE,f_SSE);
            *db_SSE = GRAYSCOTT_DB(*b_SSE,abb,b_left,b_right,*b_above,*b_below,r_b_SSE,f_plus_k_SSE);
        }
    }

    // apply the rate of change
    #pragma omp parallel for
    for(int j=0;j<Y_BLOCKS;j++)
    {
        for(int i=0;i<X_BLOCKS;i++)
        {
            __m128 *a_SSE = ((__m128*)a)+block_at(i,j);
            __m128 *b_SSE = ((__m128*)b)+block_at(i,j);
            __m128 *da_SSE = ((__m128*)da)+block_at(i,j);
            __m128 *db_SSE = ((__m128*)db)+block_at(i,j);
            *a_SSE = ADD(*a_SSE,MUL(*da_SSE,speed_SSE)); // a[i] += speed * da[i];
            *b_SSE = ADD(*b_SSE,MUL(*db_SSE,speed_SSE)); // b[i] += speed * db[i];
        }
    }

    // copy the top and bottom rows of the active area to the other side
    for(int i=0;i<X_BLOCKS;i++) 
    {
        *(((__m128*)a)+block_at(i,Y_BLOCKS-1)) = *(((__m128*)a)+block_at(i,1));
        *(((__m128*)a)+block_at(i,0)) = *(((__m128*)a)+block_at(i,Y_BLOCKS-2));
        *(((__m128*)b)+block_at(i,Y_BLOCKS-1)) = *(((__m128*)b)+block_at(i,1));
        *(((__m128*)b)+block_at(i,0)) = *(((__m128*)b)+block_at(i,Y_BLOCKS-2));
    }
    // likewise for the left- and right-most columns
    #pragma omp parallel for
    for(int j=0;j<Y_BLOCKS;j++) 
    {
        *(((__m128*)a)+block_at(X_BLOCKS-1,j)) = *(((__m128*)a)+block_at(1,j));
        *(((__m128*)a)+block_at(0,j)) = *(((__m128*)a)+block_at(X_BLOCKS-2,j));
        *(((__m128*)b)+block_at(X_BLOCKS-1,j)) = *(((__m128*)b)+block_at(1,j));
        *(((__m128*)b)+block_at(0,j)) = *(((__m128*)b)+block_at(X_BLOCKS-2,j));
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
