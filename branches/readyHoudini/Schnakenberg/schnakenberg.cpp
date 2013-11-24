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
             float alpha,float beta,float gamma,float nu,
             float speed,
             bool parameter_space);

int main()
{
    // J. Schnakenberg, Simple chemical reaction systems with limit cycle behaviour, J. Theor. Biol. 81 (1979) 389–400.
    // Following:
    // ftp://ftp.comlab.ox.ac.uk/pub/Documents/techreports/NA-03-16.pdf

    // Not sure this is right though.

    // -- parameters --
    float alpha=1.0f;
    float beta=0.9f;
    float gamma=1.0f;
    float nu=10.0f;

    float speed = 0.001f;
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
            compute(a,b,da,db,alpha,beta,gamma,nu,speed,false);
            iteration++;
        }

		end = clock();

		char msg[1000];
		sprintf(msg,"Schnakenberg - %0.2f fps",N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC));

        // display:
        if(display(a,b,b,iteration,true,200.0f,2,10,msg)) // did user ask to quit?
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

#define PI 3.1415926535

void init(float a[X][Y],float b[X][Y])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    float x,y;
    for(int i = 0; i < X; i++) 
    {
        x=float(i)/X;
        for(int j = 0; j < Y; j++) 
        {
            /*y = float(j)/Y;
            a[i][j] = 0.919145 + 0.0016*cos(2*PI*(x+y));
            b[i][j] = 0.937903 + 0.0016*cos(2*PI*(x+y));
            for(int t=1;t<=8;t++)
            {
                a[i][j] += 0.01 * cos(2*PI*t*x);
                b[i][j] += 0.01 * cos(2*PI*t*x);
            }*/
            a[i][j] = frand(-1.0f,1.0f);
            b[i][j] = frand(-1.0f,1.0f);
        }
    }
}

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float alpha,float beta,float gamma,float nu,
             float speed,
             bool parameter_space)
{
    const bool toroidal = true;

    int iprev,inext,jprev,jnext;

    // compute change in each cell
    for(int i = 0; i < X; i++) {
        if(toroidal) {
            iprev = (i + X - 1) % X;
            inext = (i + 1) % X;
        }
        else {
            iprev = max(0,i-1);
            inext = min(X-1,i+1);
        }

        for(int j = 0; j < Y; j++) {
            if(toroidal) {
                jprev = (j + Y - 1) % Y;
                jnext = (j + 1) % Y;
            }
            else {
                jprev = max(0,j-1);
                jnext = min(Y-1,j+1);
            }

            if(parameter_space)	{
                /*const float kmin=0.045f,kmax=0.07f,fmin=0.00f,fmax=0.14f;
                // set f and k for this location (ignore the provided values of f and k)
                k = kmin + i*(kmax-kmin)/X;
                f = fmin + j*(fmax-fmin)/Y;*/
            }

            float aval = a[i][j];
            float bval = b[i][j];

            // compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

            // compute the new rate of change of a and b
            //da[i][j] = 1000.0f*(0.126779f - aval + aval*aval*bval) + dda / (128.0f*128.0f);
            //db[i][j] = 1000.0f*(0.792366f - aval*aval*bval) + 10.0f*ddb / (128.0f*128.0f);
            da[i][j] = dda + gamma*(alpha - aval + aval*aval*bval);
            db[i][j] = nu * ddb + gamma*(beta - aval*aval*bval);
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

