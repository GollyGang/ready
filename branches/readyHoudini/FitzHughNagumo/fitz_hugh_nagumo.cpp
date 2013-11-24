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

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float a0,float a1,float epsilon,float delta,float k1,float k2,float k3,
             float speed,
             bool parameter_map);

int main()
{
    // -- parameters --

    // From Hagberg and Meron:
    // http://arxiv.org/pdf/patt-sol/9401002

    // for tip-splitting:
    float a0 = -0.1f;
    float a1 = 2.0f;
    float epsilon = 0.05f;
    float delta = 4.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;
    bool spiral_waves = false;

    // for spiral turbulence:
    /*float a0 = -0.1f;
    float a1 = 2.0f;
    float epsilon = 0.014f;
    float delta = 2.8f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;
    bool spiral_waves = false;*/

    // for spiral waves: http://thevirtualheart.org/java/2dfhn.html
    /*float a0 = 0.0f;
    float a1 = 1.0f;
    float epsilon = 0.01f;
    float delta = 0.0f;
    float k1 = -0.1f;
    float k2 = -1.1f;
    float k3 = 0.5f;
    bool spiral_waves = true;*/

    // from Malevanets and Kapral (can't get these to work)

    // for labyrinth:
    /*float a0 = 0.146f;
    float a1 = 3.05f;
    float epsilon = 0.017f;
    float delta = 4.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;*/
    
    // for bloch fronts:
    /*float a0 = 0.0f;
    float a1 = 4.88f;
    float epsilon = 0.084f;
    float delta = 0.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;*/
 
    float speed = 0.05f;
    bool parameter_map = false;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b);
    
	clock_t start,end;

	const int N_FRAMES_PER_DISPLAY = 100;
    int iteration = 0;
    while(true) 
    {
    	start = clock();

        // compute:
		for(int it=0;it<N_FRAMES_PER_DISPLAY;it++)
		{
            compute(a,b,da,db,a0,a1,epsilon,delta,k1,k2,k3,speed,parameter_map);
            iteration++;
        }

		end = clock();

		char msg[1000];
		sprintf(msg,"FitzHugh-Nagumo - %0.2f fps",N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC));

        // display:
        if(display(a,b,b,iteration,true,200.0f,3,10,msg)) // did user ask to quit?
            break;

        // to make more interesting patterns we periodically reset part of the grid
        if(spiral_waves && (iteration==1000 || (iteration>0 && iteration%2000==0)) )
        {
            int div = (int)(rand()*Y/(float)RAND_MAX);
            for(int i = 0; i < X; i++) 
            {
                for(int j = 0; j < Y; j++) 
                {
                    if(j<div)
                    {
                        a[i][j] = 0.0f;
                        b[i][j] = 0.0f;
                    }
                }
            }
        }
    }
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

void init(float a[X][Y],float b[X][Y])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    float pos;
    for(int i = 0; i < X; i++) 
    {
        if(i%30==0)
            pos = frand(-6.0f,6.0f);
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] = 0.0f;
            b[i][j] = 0.0f;
            /*if(i>10 && i<20) // start with a wave on the left edge
            {
                a[i][j] = 1.0f;
            }
            if(i>1 && i<17)
            {
                b[i][j] = 0.1f;
            }*/
            if(abs(i-X/2)<X/3 && abs(j-Y/2+pos)<10) // start with a wavy static line in the centre
            //if(i<10)
            {
                a[i][j] = 1.0f;
                b[i][j] = 0.0f;
            }
            // you can start from random conditions too, but it's harder to guarantee
            // interesting waves
            if(false)
            {
                a[i][j] = frand(-0.1f,0.1f);
                b[i][j] = frand(-0.1f,0.1f);
            }
        }
    }
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float a0,float a1,float epsilon,float delta,float k1,float k2,float k3,
             float speed,
             bool parameter_map)
{
    int iprev,inext,jprev,jnext;
    bool toroidal = false;

    // compute change in each cell
    for(int i = 0; i < X; i++) 
    {
        if(toroidal) { iprev = (i + X - 1) % X; inext = (i + 1) % X; }
        else { iprev=max(0,i-1); inext=min(X-1,i+1); }

        for(int j = 0; j < Y; j++) 
        {
            if(toroidal) { jprev = (j + Y - 1) % Y; jnext = (j + 1) % Y; }
            else { jprev=max(0,j-1); jnext=min(Y-1,j+1); }

            float aval = a[i][j];
            float bval = b[i][j];

            // compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

            if(parameter_map)
            {
                const float min_a0=-0.6f,max_a0=0.6f;
                const float min_a1=-2.0f,max_a1=10.0f;
                const float min_epsilon=0.0f,max_epsilon=0.2f;
                const float min_delta=0.0f,max_delta=5.0f;
                //a0 = min_a0 + (max_a0-min_a0) * i/X;
                //a1 = min_a1 + (max_a1-min_a1) * j/Y;
                epsilon = min_epsilon + (max_epsilon-min_epsilon) * i/X;
                delta = min_delta + (max_delta-min_delta) * j/Y;
            }

            // compute the new rate of change of a and b
            da[i][j] = k1*aval - k2*aval*aval - aval*aval*aval - bval + dda;
            db[i][j] = epsilon * (k3*aval - a1*bval - a0) + delta*ddb;
        }
    }

    // effect change
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            a[i][j] += (speed * da[i][j]);
            b[i][j] += (speed * db[i][j]);
        }
    }
}

