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

void init(float a[X][Y],float b[X][Y],float A,float B);

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float A,float B,float D1,float D2,
             float speed,
             bool parameter_space);

int main()
{
    // -- parameters --
    float A = 3.0f;
    float B = 10.0f;
    float D1 = 5.0f;
    float D2 = 12.0f;
    float speed = 0.001f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b,A,B);
    
    int iteration = 0;
    while(true) {

        // compute:
        compute(a,b,da,db,A,B,D1,D2,speed,false);

        // display:
        if(iteration%100==0) 
        {
            if(display(a,b,a,iteration,true,20.0f,2,"Brusselator (Esc to quit)",false)) // did user ask to quit?
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

void init(float a[X][Y],float b[X][Y],float A,float B)
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] = A + frand(-0.01f,0.01f);
            b[i][j] = B/A + frand(-0.01f,0.01f);
        }
    }
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float A,float B,float D1,float D2,
             float speed,
             bool parameter_space)
{
    // compute change in each cell
    for(int i = 0; i < X; i++) 
    {
        //int iprev = (i + X - 1) % X;
        //int inext = (i + 1) % X;
        int iprev = max(0,i-1);
        int inext = min(X-1,i+1);

        for(int j = 0; j < Y; j++) 
        {
            //int jprev = (j + Y - 1) % Y;
            //int jnext = (j + 1) % Y;
            int jprev = max(0,j-1);
            int jnext = min(Y-1,j+1);

            float aval = a[i][j];
            float bval = b[i][j];

            if(parameter_space)
            {
                const float A1=0.0f,A2=4.0f,B1=0.0f,B2=15.0f;
                A = A1+(A2-A1)*i/X;
                B = B1+(B2-B1)*j/Y;
            }

            // compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

            // compute the new rate of change of a and b
            da[i][j] = A-(B+1)*aval + aval*aval*bval + D1*dda;
            db[i][j] = B*aval - aval*aval*bval + D2*ddb;
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

