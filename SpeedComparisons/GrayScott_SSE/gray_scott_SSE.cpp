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

// SSE:
#include <xmmintrin.h>

// OpenCV:
#include <cv.h>
#include <highgui.h>

// local:
#include "defs.h"
//#include "display.h"


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
    
    clock_t start,end;

    const int N_FRAMES_PER_DISPLAY = 10000;
    int iteration = 0;
    while(true) 
    {
        start = clock();

        // compute:
        for(int it=0;it<N_FRAMES_PER_DISPLAY;it++)
        {
            compute(a,b,da,db,r_a,r_b,f,k,speed);
            iteration++;
        }

        end = clock();

        char msg[1000];
        sprintf(msg,"GrayScott - %0.2f fps",N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC));

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
    srand((unsigned int)time(NULL));
    
    // figure the values
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
            if(abs(j-100)<10 && abs(i-80)<12)
            {
                da[at(i,j)]=0.00001f;
                db[at(i,j)]=0.0f;
            }
            else
            {
                da[at(i,j)]=0.0f;
                db[at(i,j)]=0.0f;
            }
        }
    }
}

void compute(float *a,float *b,float *da,float *db,
             float r_a,float r_b,float f,float k,
             float speed)
{
    __m128 *a_SSE = (__m128*) a;
    __m128 *b_SSE = (__m128*) b;
    __m128 *da_SSE = (__m128*) da;
    __m128 *db_SSE = (__m128*) db;
    __m128 x_SSE;
    __m128 dda_SSE,ddb_SSE;
    __m128 minus_four_SSE = _mm_set1_ps(-4.0f); // set four to (-4,-4,-4,-4)

    // compute the rates of change
    for(int i=1;i<X_BLOCKS-1;i++) // we skip the left- and right-most blocks for now (for simplicity)
    {
        a_SSE += X_BLOCKS+1;
        b_SSE += X_BLOCKS+1;
        da_SSE += X_BLOCKS+1;
        db_SSE += X_BLOCKS+1;
        for(int j=1;j<Y_BLOCKS-1;j++) // we skip the top- and bottom-most blocks for now
        {
            // retrieve a block containing the 4 cells to the left of our 4 cells
            // find the Laplacian of a (using the von Neumann neighborhood)
            dda_SSE = _mm_mul_ps(*a_SSE,minus_four_SSE);
            //dda_SSE = _mm_add_ps(dda_SSE,);

            // find the Laplacian of b
            // ddb = 
            // compute the new rates of changes
            // da_SSE = 
            // db_SSE = 
        }
    }

    // apply the rate of change
    __m128 speed_SSE = _mm_set1_ps(speed); // set speed_SSE to (speed,speed,speed,speed)
    a_SSE = (__m128*) a;
    b_SSE = (__m128*) b;
    da_SSE = (__m128*) da;
    db_SSE = (__m128*) db;
    for(int i=0;i<TOTAL_BLOCKS;i++)
    {
        // a[i] += speed * da[i];
        x_SSE = _mm_mul_ps(*da_SSE,speed_SSE);
        *a_SSE = _mm_add_ps(*a_SSE,x_SSE);
        a_SSE++;
        da_SSE++;
        // b[i] += speed * db[i];
        x_SSE = _mm_mul_ps(*db_SSE,speed_SSE);
        *b_SSE = _mm_add_ps(*b_SSE,x_SSE);
        b_SSE++;
        db_SSE++;
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
