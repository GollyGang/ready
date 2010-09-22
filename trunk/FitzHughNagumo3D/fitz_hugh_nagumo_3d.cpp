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

#define Z X

void init(float a[X][Y][Z],float b[X][Y][Z]);

void compute(float a[X][Y][Z],float b[X][Y][Z],
             float da[X][Y][Z],float db[X][Y][Z],
             float a0,float a1,float epsilon,float delta,float k1,float k2,float k3,
             float speed,
             bool parameter_map);

int main()
{
    // -- parameters --

    // From Hagberg and Meron:
    // http://arxiv.org/pdf/patt-sol/9401002

    // for (2D) tip-splitting:
    /*float a0 = -0.1f;
    float a1 = 2.0f;
    float epsilon = 0.05f;
    float delta = 4.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;
    bool spiral_waves = false;*/

    // looking for 3D tip-splitting
    float a0 = -0.1f;
    float a1 = 2.0f;
    float epsilon = 0.05f;
    float delta = 4.0f;
    float k1 = 1.0f;
    float k2 = 0.0f;
    float k3 = 1.0f;
    bool spiral_waves = false;

    // for (2D) spiral turbulence:
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
    bool spiral_waves = true;
    */

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
 
    float speed = 0.01f;
    bool parameter_map = false;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y][Z], b[X][Y][Z];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y][Z], db[X][Y][Z];

    // put the initial conditions into each cell
    init(a,b);
    
    int iteration = 0;
    while(true) 
    {
        // compute:
        compute(a,b,da,db,a0,a1,epsilon,delta,k1,k2,k3,speed,parameter_map);

        // display:
        if(iteration%50==0) 
        {
            if(display(a[13],b[13],b[13],iteration,true,200.0f,3.0f,10,"FitzHughNagumo (Esc to quit)")) // did user ask to quit?
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

void init(float a[X][Y][Z],float b[X][Y][Z])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            for(int k = 0; k < Z; k++) 
            {
                a[i][j][k] = 0.0f;
                b[i][j][k] = 0.0f;
                if(abs(i-X/2)<X/3 && abs(j-Y/2)<Y/10 && abs(k-Z/2)<Z/20) // start with a static line in the centre
                {
                    a[i][j][k] = 1.0f;
                    b[i][j][k] = 0.0f;
                }
            }
        }
    }
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void compute(float a[X][Y][Z],float b[X][Y][Z],
             float da[X][Y][Z],float db[X][Y][Z],
             float a0,float a1,float epsilon,float delta,float k1,float k2,float k3,
             float speed,
             bool parameter_map)
{
    int iprev,inext,jprev,jnext,kprev,knext;
    bool toroidal = true;

    // compute change in each cell
    for(int i = 0; i < X; i++) 
    {
        if(toroidal) { iprev = (i + X - 1) % X; inext = (i + 1) % X; }
        else { iprev=max(0,i-1); inext=min(X-1,i+1); }

        for(int j = 0; j < Y; j++) 
        {
            if(toroidal) { jprev = (j + Y - 1) % Y; jnext = (j + 1) % Y; }
            else { jprev=max(0,j-1); jnext=min(Y-1,j+1); }

            for(int k = 0; k < Z; k++) 
            {
                if(toroidal) { kprev = (k + Z - 1) % Z; knext = (k + 1) % Z; }
                else { kprev=max(0,k-1); knext=min(Z-1,k+1); }

                float aval = a[i][j][k];
                float bval = b[i][j][k];

                // compute the Laplacians of a and b
                float dda = a[i][jprev][k] + a[i][jnext][k] + a[iprev][j][k] + a[inext][j][k] + a[i][j][kprev] + a[i][j][knext] - 6*aval;
                float ddb = b[i][jprev][k] + b[i][jnext][k] + b[iprev][j][k] + b[inext][j][k] + b[i][j][kprev] + b[i][j][knext] - 6*bval;

                // compute the new rate of change of a and b
                da[i][j][k] = k1*aval - k2*aval*aval - aval*aval*aval - bval + dda;
                db[i][j][k] = epsilon * (k3*aval - a1*bval - a0) + delta*ddb;
            }
        }
    }

    // effect change
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            for(int k = 0; k < Z; k++) {
                a[i][j][k] += (speed * da[i][j][k]);
                b[i][j][k] += (speed * db[i][j][k]);
            }
        }
    }
}

