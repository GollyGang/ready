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

// OpenMP:
#include <omp.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float r_a,float r_b,float f,float k,
             float speed, bool parameter_space);

static int g_nthreads;
static int g_wrap = 0;
static bool g_paramspace = 0;

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
    double fps_avg = 0.0; // decaying average of fps
    while(true) 
    {
        struct timeval tod_record;
        double tod_before, tod_after, tod_elap;
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

        tod_elap = tod_after - tod_before;

        char msg[1000];
        fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elap;
        // We display an exponential moving average of the fps measurement
        fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0); 
        sprintf(msg,"GrayScott %d threads %0.2f fps", g_nthreads, fps_avg);

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
             float r_a,float r_b,float par_f,float par_k,float speed,
             bool parameter_space)
{
    // compute change in each cell
    #pragma omp parallel for
    for(int i = 0; i < X; i++) 
    {
        int iprev,inext;
        float f = par_f;
        float k = par_k;
        if (g_wrap) {
            iprev = (i + X - 1) % X;
            inext = (i + 1) % X;
        } else {
            iprev = max(0,i-1);
            inext = min(X-1,i+1);
        }

        for(int j = 0; j < Y; j++) 
        {
            int jprev,jnext;
            if (g_wrap) {
                // toroidal
                jprev = (j + Y - 1) % Y;
                jnext = (j + 1) % Y;
            } else {
                jprev = max(0,j-1);
                jnext = min(Y-1,j+1);
            }

            float aval = a[i][j];
            float bval = b[i][j];

            if (parameter_space) {
                const float kmin=0.045f,kmax=0.07f,fmin=0.01f,fmax=0.09f;
                // set f and k for this location (ignore the provided values of f and k)
                k = kmin + i*(kmax-kmin)/X;
                f = fmin + j*(fmax-fmin)/Y;
            }

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

