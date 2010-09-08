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
             float D_a,float D_b,float alpha,float beta,float gamma,float delta,
             float speed);

int main()
{
    // Here we implement the complex Ginzberg-Landau model:
    // http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.31.529&rep=rep1&type=pdf

    // -- parameters --
    float alpha = 1.0f/16.0f;
    float beta = 1.0f;
    float delta = 1.0f; // 2.0f gives unstable spirals
    float gamma = delta/16.0f;
    float D_a = 0.2f;
    float D_b = 0.2f;
    float speed = 0.2f;
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
        compute(a,b,da,db,D_a,D_b,alpha,beta,gamma,delta,speed);

        // display:
        if(iteration%40==0) 
        {
            if(display(a,a,b,iteration,true,200.0f,2.0f,10,"Complex Ginzberg-Landau (Esc to quit)")) // did user ask to quit?
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
            a[i][j] = frand(-1.0f,1.0f);
            b[i][j] = frand(-1.0f,1.0f);
        }
    }
}

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float D_a,float D_b,float alpha,float beta,float gamma,float delta,
             float speed)
{
    // compute change in each cell
    for(int i = 0; i < X; i++) 
    {
        int iprev = (i + X - 1) % X;
        int inext = (i + 1) % X;

        for(int j = 0; j < Y; j++) 
        {
            int jprev = (j + Y - 1) % Y;
            int jnext = (j + 1) % Y;

            float aval = a[i][j];
            float bval = b[i][j];

            // compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

            // compute the new rate of change of a and b
            da[i][j] = D_a * dda + alpha*aval - gamma*bval + (-beta*aval + delta*bval)*(aval*aval+bval*bval);
            db[i][j] = D_b * ddb + alpha*bval + gamma*aval + (-beta*bval - delta*aval)*(aval*aval+bval*bval);
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

