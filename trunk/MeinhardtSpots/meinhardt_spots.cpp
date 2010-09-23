/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

NB. Turk's thesis has errors in the formulae for this system, the correct ones are here:
http://www1.cse.wustl.edu/~faanly/materials/Sketching_RD_Texture.pdf

*/

// stdlib:
#include <time.h>
#include <stdlib.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y],float beta[X][Y],
          float p1,float p2);

void compute(float a[X][Y],float b[X][Y],float beta[X][Y],
             float da[X][Y],float db[X][Y],
             float diff1,float diff2,float p1,float p2,float p3,float s,
             float speed);

int main()
{
    // -- parameters --
    float p1 = 0.03f;
    float p2 = 0.04f;
    float p3 = 0.0f;
    float diff1 = 0.01f;
    float diff2 = 0.2f;
    float s = 0.2f;
    float speed = 1.0f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y];
    float beta[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b,beta,p1,p2);
    
    int iteration = 0;
    while(true)
    {
        // compute:
        compute(a,b,beta,da,db,diff1,diff2,p1,p2,p3,s,speed);

        // display:
        if(iteration%10==0)
        {
            if(display(a,a,b,iteration,true,30.0f,4.0f,"MeinhardtSpots (Esc to quit)",false)) // did user ask to quit?
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

void init(float a[X][Y],float b[X][Y],float beta[X][Y],
          float p1,float p2)
{
    srand((unsigned int)time(NULL));
    
    float ainit,binit;
    ainit = p2 / p1;
    binit = 0.01f * ainit * ainit / p2;

    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            a[i][j] = ainit;
            b[i][j] = binit;
            if(i<X/2)
                beta[i][j] = frand(0.9f,1.1f);
            else
                beta[i][j] = 1;
        }
    }
}

void compute(float a[X][Y],float b[X][Y],float beta[X][Y],
             float da[X][Y],float db[X][Y],
             float diff1,float diff2,float p1,float p2,float p3,float s,
             float speed)
{
    // compute change in each cell
    for(int i = 0; i < X; i++) {

        int iprev = (i + X - 1) % X;
        int inext = (i + 1) % X;

        for(int j = 0; j < Y; j++) {

            int jprev = (j + Y - 1) % Y;
            int jnext = (j + 1) % Y;

            float aval = a[i][j];
            float bval = b[i][j];

            // compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4 * aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4 * bval;

            // compute the new rates of change
            da[i][j] = s * (0.01f * aval * aval * beta[i][j] / bval - aval * p1 + p3) + diff1 * dda;
            db[i][j] = s * (0.01f * aval * aval * beta[i][j] - bval * p2 + p3) + diff2 * ddb;
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

