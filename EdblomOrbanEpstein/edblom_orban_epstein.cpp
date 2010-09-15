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
             float A,float B,float C,float D,float F,
             float speed,
             bool parameter_space);

int main()
{
    // http://www.robinengelhardt.info/speciale/

    // -- parameters --
    float A=17.00f;
    float B=1.0f;
    float C=1.0f;
    float D=1.39f;
    float F=7.65f;
    float speed = 0.01f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b);
    
    int iteration = 0;
    while(true) 
    {
        // compute:
        compute(a,b,da,db,A,B,C,D,F,speed,false);
        
        // display:
        if(iteration%1000==0) 
        {
            if(display(b,b,b,iteration,true,200.0f,4.0f,10,"EdblomOrbanEpstein (Esc to quit)")) // did user ask to quit?
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

void init(float a[X][Y],float b[X][Y])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            a[i][j] = frand(0.0f,17.0f);
            b[i][j] = frand(0.0f,17.0f);
            //if(abs(i-X/2)<5)
            if(abs(i-X/2)>10 && j>Y-3)
            {
                a[i][j]=1.8f;
                b[i][j]=2.8f;
            }
            else
            {
                a[i][j]=3.8f;
                b[i][j]=1.8f;
            }
        }
    }
}

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float A,float B,float C,float D,float F,
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
            da[i][j] =  C * ( - aval*bval*bval + A*bval - (1+B)*aval ) + D*dda;
            db[i][j] = (1/C) * ( aval*bval*bval - (1+A)*bval + aval + F ) + ddb;
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

