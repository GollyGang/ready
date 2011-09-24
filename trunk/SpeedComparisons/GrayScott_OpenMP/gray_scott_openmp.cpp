/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

// OpenMP:
#include <omp.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float r_a,float r_b,float f,float k,
             float speed);

static int g_nthreads;

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
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b);

    const int N_FRAMES_PER_DISPLAY = 1000;
    int iteration = 0;
    while(true) 
    {
        struct timeval tod_record;
        double tod_before, tod_after, tod_elap;

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

        tod_elap = tod_after - tod_before;

        char msg[1000];
        double fps;
        fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elap;
        sprintf(msg,"GrayScott %d threads %0.2f fps", g_nthreads, fps);

        // display:
        if(display(a,a,a,iteration,false,200.0f,2,10,msg)) // did user ask to quit?
            break;
    }
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void init(float a[X][Y],float b[X][Y])
{
  #pragma omp parallel default(shared)
  {
    g_nthreads = omp_get_num_threads();
  }

    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            // start with a uniform field with an approximate circle in the middle
            //if(hypot(i%20-10/*-X/2*/,j%20-10/*-Y/2*/)<=frand(2,5)) {
            if(hypot(i-X/2,(j-Y/2)/1.5)<=frand(2,5))
            {
                a[i][j] = 0.0f;
                b[i][j] = 1.0f;
            }
            else {
                a[i][j] = 1;
                b[i][j] = 0;
            }
            /*float v = frand(0.0f,1.0f);
            a[i][j] = v;
            b[i][j] = 1.0f-v;*/
        }
    }
}

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float r_a,float r_b,float f,float k,float speed)
{

    // compute change in each cell
    #pragma omp parallel for
    for(int i = 0; i < X; i++) 
    {
        // toroidal
        int iprev = (i + X - 1) % X;
        int inext = (i + 1) % X;

        for(int j = 0; j < Y; j++) 
        {
            // toroidal
            int jprev = (j + Y - 1) % Y;
            int jnext = (j + 1) % Y;

            float aval = a[i][j];
            float bval = b[i][j];

            // compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

            // compute the new rate of change of a and b
            da[i][j] = r_a * dda - aval*bval*bval + f*(1-aval);
            db[i][j] = r_b * ddb + aval*bval*bval - (f+k)*bval;
        }
    }

    // effect change
    #pragma omp parallel for
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] += speed * da[i][j];
            b[i][j] += speed * db[i][j];
            // kill denormals by adding a teeny tiny something (http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.20.1348&rank=4)
            a[i][j] += 1e-10f;
            b[i][j] += 1e-10f;
        }
    }
}

