/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <math.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float D_a,float D_b,float A,float B,
             float speed,
             bool parameter_space);

int main()
{
    // Here we implement the Brandeisator; the Lengyel-Epstein model for the 
    // chlorite-iodide-malonic acid (CIMA) reaction 
    // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.34.9043&rep=rep1&type=pdf

    // But I can't get this to work at the moment. Perhaps we need a more sophisticated solver?
    // Or the initial conditions need to be right to get interesting behaviour?

    // -- parameters --
    float A = 12.371f;
    float B = 16.0f;
    float D_a = 0.1f;
    float D_b = 1.0f;
    float speed = 0.01f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b);
    
    int iteration = 0;
    while(true) {

        // compute:
        compute(a,b,da,db,D_a,D_b,A,B,speed,false);

        // display:
        if(iteration%100==0) 
        {
            if(display(a,a,b,iteration,true,200.0f,2,10,"Brandeisator (Esc to quit)")) // did user ask to quit?
                break;
        }

        iteration++;
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
            a[i][j] = frand(-2.0f,2.0f);
            b[i][j] = frand(-2.0f,2.0f);
        }
    }
}

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float D_a,float D_b,float A,float B,float speed,
             bool parameter_space)
{
    const bool toroidal = false;

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

            if(parameter_space) {
                /*const float kmin=0.03f,kmax=0.07f,fmin=0.00f,fmax=0.06f;
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
            da[i][j] = D_a * dda + A - aval - (aval*bval) / (1+aval*aval);
            db[i][j] = D_b * ddb + 4*B*aval - B * (aval*bval) / (1+aval*aval);
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

