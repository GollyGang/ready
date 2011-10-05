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

// If SIZE_OPTIONS is declared, the user can specify an image size with -width N and -height N options
//#define SIZE_OPTIONS

#ifndef SIZE_OPTIONS
// local:
#include "defs.h"
#endif

// consecutive horizontal SSE blocks lie end to end, to enable easy use of _mm_loadu_ps() for left and right

const int SSE_BITS_PER_BLOCK = 128;
const int FLOATS_PER_BLOCK = (SSE_BITS_PER_BLOCK/8) / sizeof(float);

#ifndef SIZE_OPTIONS
const int PADDED_X = X + 2*FLOATS_PER_BLOCK; // our toroidal wrap-around scheme uses a border that is copied from the other side each time
const int PADDED_Y = Y + 2;
const int X_BLOCKS = PADDED_X / FLOATS_PER_BLOCK;
const int Y_BLOCKS = PADDED_Y;
const int TOTAL_BLOCKS = X_BLOCKS * Y_BLOCKS;
#else
static int X = 256;
static int Y = 256;
static int PADDED_X = X + 2*FLOATS_PER_BLOCK; // our toroidal wrap-around scheme uses a border that is copied from the other side each time
static int PADDED_Y = Y + 2;
static int X_BLOCKS = PADDED_X / FLOATS_PER_BLOCK;
static int Y_BLOCKS = PADDED_Y;
static int TOTAL_BLOCKS = X_BLOCKS * Y_BLOCKS;
#endif

inline int at(int x,int y) { return y*PADDED_X+x; } 
inline int block_at(int x,int y) { return y*X_BLOCKS+x; }

void init(float *a,float *b,float *da,float *db);
void compute(float *a,float *b,float *da,float *db,
             const float r_a,const float r_b,const float f,const float k,
             const float speed);
bool display(float *r,float *g,float *b,
             int iteration,bool auto_brighten,float manual_brighten,
             int scale,int delay_ms,const char* message);

int main(int argc, char * * argv)
{
#ifdef SIZE_OPTIONS
  for (int i = 1; i < argc; i++) {
    if (0) {
    } else if ((i+1<argc) && (strcmp(argv[i],"-height")==0)) {
      // set height
      i++; Y = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-size")==0)) {
      // set both width and height
      i++; Y = X = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-width")==0)) {
      // set width
      i++; X = atoi(argv[i]);
    } else {
      fprintf(stderr, "Unrecognized argument, or parameter needed: '%s'\n", argv[i]);
      exit(-1);
    }
  }
  PADDED_X = X + 2*FLOATS_PER_BLOCK; // our toroidal wrap-around scheme uses a border that is copied from the other side each time
  PADDED_Y = Y + 2;
  X_BLOCKS = PADDED_X / FLOATS_PER_BLOCK;
  Y_BLOCKS = PADDED_Y;
  TOTAL_BLOCKS = X_BLOCKS * Y_BLOCKS;
#endif

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

    const int n_cells = PADDED_X*PADDED_Y;
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
        float Mcgs = (fps_avg * ((float)X) * ((float)Y)) / 1.0e6;

        char msg[1000];
        sprintf(msg,"GrayScott - %0.2f fps %0.3f Mcgs",fps_avg, Mcgs);

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
    for(int i = 0; i < PADDED_X; i++)
    {
        for(int j = 0; j < PADDED_Y; j++)
        {
            if(hypot(i-PADDED_X/3,(j-PADDED_Y/4)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
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


#if 0

// we tried using 'raise' and 'lower' macros to reduce the amount of loading but it ended up slower

// the same as _mm_loadu_ps(((float*)(&_here))-1) when _here follows _left in memory
#define FLOAT_LEFT(_left,_here) _mm_shuffle_ps(_mm_shuffle_ps(_left,_here,_MM_SHUFFLE(0,0,3,3)),_here,_MM_SHUFFLE(2,1,2,0))
// (Robert Munafo's RAISE function)

// the same as _mm_loadu_ps(((float*)(&_here))+1) when _right follows _here in memory
#define FLOAT_RIGHT(_here,_right) _mm_shuffle_ps(_here,_mm_move_ss(_here,_right),_MM_SHUFFLE(0,3,2,1))

void compute(float *a,float *b,float *da,float *db,
             const float r_a,const float r_b,const float f,const float k,
             const float speed)
{
    /* 
    {
        // test our new FLOAT_LEFT and FLOAT_RIGHT macros:
        float arr1[12] = {1,2,3,4,5,6,7,8,9,10,11,12};
        float *arr2 = (float*)_mm_malloc(12*sizeof(float),16);
        memcpy(arr2,arr1,12*sizeof(float));
        __m128 *a = (__m128*)(arr2+0);
        __m128 *b = (__m128*)(arr2+4);
        __m128 *c = (__m128*)(arr2+8);
        __m128 left = FLOAT_LEFT(*a,*b);
        __m128 right = FLOAT_RIGHT(*b,*c);
        printf("done"); // put a breakpoint here and look at left and right
    }
    */

    const __m128 r_a_SSE = SET(r_a);
    const __m128 r_b_SSE = SET(r_b);
    const __m128 f_SSE = SET(f);
    const __m128 f_plus_k_SSE = SET(f+k);
    const __m128 speed_SSE = SET(speed);

    // compute the rate of change
    for(int j=1;j<Y_BLOCKS-1;j++) // we skip the top- and bottom-most blocks
    {
        __m128 *a1,*b1;
        __m128 *a2 = ((__m128*)a)+block_at(0,j);
        __m128 *a3 = ((__m128*)a)+block_at(1,j);
        __m128 *b2 = ((__m128*)b)+block_at(0,j);
        __m128 *b3 = ((__m128*)b)+block_at(1,j);
        // (TODO: wrap-around)
        for(int i=1;i<X_BLOCKS-1;i++) // we skip the left- and right-most blocks
        {
            // load the next blocks
            a1=a2; a2=a3; a3 = ((__m128*)a)+block_at(i+1,j);
            b1=b2; b2=b3; b3 = ((__m128*)b)+block_at(i+1,j);
            __m128 *da_SSE = ((__m128*)da)+block_at(i,j);
            __m128 *db_SSE = ((__m128*)db)+block_at(i,j);
            // retrieve the neighboring cells
            __m128 a_left = FLOAT_LEFT(*a1,*a2);
            __m128 a_right = FLOAT_RIGHT(*a2,*a3);
            __m128 *a_above = a2-X_BLOCKS;
            __m128 *a_below = a2+X_BLOCKS;
            __m128 b_left = FLOAT_LEFT(*b1,*b2);
            __m128 b_right = FLOAT_RIGHT(*b2,*b3);
            __m128 *b_above = b2-X_BLOCKS;
            __m128 *b_below = b2+X_BLOCKS;
            __m128 abb = _mm_mul_ps(_mm_mul_ps(*a2,*b2),*b2);
            // compute the new rates of change of each chemical
            *da_SSE = GRAYSCOTT_DA(*a2,abb,a_left,a_right,*a_above,*a_below,r_a_SSE,f_SSE);
            *db_SSE = GRAYSCOTT_DB(*b2,abb,b_left,b_right,*b_above,*b_below,r_b_SSE,f_plus_k_SSE);
        }
    }

    // apply the rate of change
    for(int j=1;j<Y_BLOCKS-1;j++) // we skip the top- and bottom-most blocks
    {
        for(int i=1;i<X_BLOCKS-1;i++) // we skip the left- and right-most blocks
        {
            __m128 *a_SSE = ((__m128*)a)+block_at(i,j);
            __m128 *b_SSE = ((__m128*)b)+block_at(i,j);
            __m128 *da_SSE = ((__m128*)da)+block_at(i,j);
            __m128 *db_SSE = ((__m128*)db)+block_at(i,j);
            *a_SSE = ADD(*a_SSE,MUL(*da_SSE,speed_SSE)); // a[i] += speed * da[i];
            *b_SSE = ADD(*b_SSE,MUL(*db_SSE,speed_SSE)); // b[i] += speed * db[i];
        }
    }
}

#else

// original code (faster)

void compute(float *a,float *b,float *da,float *db,
             const float r_a,const float r_b,const float f,const float k,
             const float speed)
{
    const __m128 r_a_SSE = SET(r_a);
    const __m128 r_b_SSE = SET(r_b);
    const __m128 f_SSE = SET(f);
    const __m128 f_plus_k_SSE = SET(f+k);
    const __m128 speed_SSE = SET(speed);

    // compute the rate of change
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
    for(int j=1;j<Y_BLOCKS-1;j++) // we skip the top- and bottom-most blocks
    {
        for(int i=1;i<X_BLOCKS-1;i++) // we skip the left- and right-most blocks
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
    for(int i=1;i<X_BLOCKS-1;i++) 
    {
        *(((__m128*)a)+block_at(i,Y_BLOCKS-1)) = *(((__m128*)a)+block_at(i,1));
        *(((__m128*)a)+block_at(i,0)) = *(((__m128*)a)+block_at(i,Y_BLOCKS-2));
        *(((__m128*)b)+block_at(i,Y_BLOCKS-1)) = *(((__m128*)b)+block_at(i,1));
        *(((__m128*)b)+block_at(i,0)) = *(((__m128*)b)+block_at(i,Y_BLOCKS-2));
    }
    // likewise for the left- and right-most columns
    for(int j=1;j<Y_BLOCKS-1;j++) 
    {
        *(((__m128*)a)+block_at(X_BLOCKS-1,j)) = *(((__m128*)a)+block_at(1,j));
        *(((__m128*)a)+block_at(0,j)) = *(((__m128*)a)+block_at(X_BLOCKS-2,j));
        *(((__m128*)b)+block_at(X_BLOCKS-1,j)) = *(((__m128*)b)+block_at(1,j));
        *(((__m128*)b)+block_at(0,j)) = *(((__m128*)b)+block_at(X_BLOCKS-2,j));
    }
}

#endif

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

        im = cvCreateImage(cvSize(PADDED_X,PADDED_Y),IPL_DEPTH_8U,3);
        cvSet(im,cvScalar(0,0,0));
        im2 = cvCreateImage(cvSize(PADDED_X*scale,PADDED_Y*scale),IPL_DEPTH_8U,3);

        cvNamedWindow(title,CV_WINDOW_AUTOSIZE);

        double hScale=0.4;
        double vScale=0.4;
        int lineWidth=1;
        cvInitFont(&font,CV_FONT_HERSHEY_COMPLEX,hScale,vScale,0,lineWidth,CV_AA);
    }

    // convert float arrays to IplImage for OpenCV to display
    float val,minR=FLT_MAX,maxR=-FLT_MAX,minG=FLT_MAX,maxG=-FLT_MAX,minB=FLT_MAX,maxB=-FLT_MAX;
    if(auto_brighten)
    {
        for(int i=0;i<PADDED_X;i++)
        {
            for(int j=0;j<PADDED_Y;j++)
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
    for(int col=0;col<PADDED_X;col++)
    {
        for(int row=0;row<PADDED_Y;row++)
        {
            if(r) {
                float val = r[at(col,row)];
                if(auto_brighten) val = 255.0f * (val-minR) / (maxR-minR);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 2] = (uchar)val;
            }
            if(g) {
                float val = g[at(col,row)];
                if(auto_brighten) val = 255.0f * (val-minG) / (maxG-minG);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 1] = (uchar)val;
            }
            if(b) {
                float val = b[at(col,row)];
                if(auto_brighten) val = 255.0f * (val-minB) / (maxB-minB);
                else val *= manual_brighten;
                if(val<0) val=0; if(val>255) val=255;
                ((uchar *)(im->imageData + row*im->widthStep))[col*im->nChannels + 0] = (uchar)val;
            }
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
