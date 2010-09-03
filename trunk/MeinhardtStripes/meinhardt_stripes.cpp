/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

See also:
http://www1.cse.wustl.edu/~faanly/materials/Sketching_RD_Texture.pdf

*/

// stdlib:
#include <time.h>
#include <stdlib.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y],float c[X][Y],float d[X][Y],float e[X][Y],
          float beta[X][Y],
          float k_ab,float k_c,float arand);

void compute(float a[X][Y],float b[X][Y],float c[X][Y],float d[X][Y],float e[X][Y],
             float beta[X][Y],
             float da[X][Y],float db[X][Y],float dc[X][Y],float dd[X][Y],float de[X][Y],
             float diff1,float diff2,float k_ab,float k_c,float k_de,float speed);

int main()
{
    // -- parameters --
    float k_ab = 0.04f; // p1
    float k_c = 0.06f; // p2
    float k_de = 0.04f; // p3
    float diff1 = 0.009f;
    float diff2 = 0.2f;
    float arand = 0.01f;
    float speed = 1.0f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y], c[X][Y], d[X][Y], e[X][Y];
    float beta[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y], dc[X][Y], dd[X][Y], de[X][Y];

    // put the initial conditions into each cell
    init(a,b,c,d,e,beta,k_ab,k_c,arand);
    
    int iteration = 0;
    while(true)
    {
        // compute:
        compute(a,b,c,d,e,beta,da,db,dc,dd,de,diff1,diff2,k_ab,k_c,k_de,speed);

        // display:
        if(iteration%10==0)
        {
            if(display(a,a,b,iteration,true,30.0f,2,20,"MeinhardtStripes (Esc to quit)")) // did user ask to quit?
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

void init(float a[X][Y],float b[X][Y],float c[X][Y],float d[X][Y],float e[X][Y],
          float beta[X][Y],
          float k_ab,float k_c,float arand)
{
    srand((unsigned int)time(NULL));
    
    float ainit,binit;
    float cinit,dinit,einit;

    ainit = binit = cinit = dinit = einit = 0;

    // figure the values
    for(int i = 0; i < X; i++) 
    {
        ainit = k_c / (2 * k_ab);
        binit = ainit;
        cinit = 0.02f * ainit * ainit * ainit / k_c;
        dinit = ainit;
        einit = ainit;

        for(int j = 0; j < Y; j++) 
        {
            a[i][j] = ainit;
            b[i][j] = binit;
            c[i][j] = cinit;
            d[i][j] = dinit;
            e[i][j] = einit;
            if(i<X/4) // only initialise part of the grid, to allow regular stripes to form elsewhere
                beta[i][j] = 1 + frand (-0.5f * arand, 0.5f * arand);
            else
                beta[i][j] = 1;
        }
    }
}

void compute(float a[X][Y],float b[X][Y],float c[X][Y],float d[X][Y],float e[X][Y],
             float beta[X][Y],
             float da[X][Y],float db[X][Y],float dc[X][Y],float dd[X][Y],float de[X][Y],
             float diff1,float diff2,float k_ab,float k_c,float k_de,float speed)
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
            float cval = c[i][j];
            float dval = d[i][j];
            float eval = e[i][j];

			// compute the Laplacians of a, b, d and e (we don't need c)
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4 * aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4 * bval;
            float ddd = d[i][jprev] + d[i][jnext] + d[iprev][j] + d[inext][j] - 4 * dval;
            float dde = e[i][jprev] + e[i][jnext] + e[iprev][j] + e[inext][j] - 4 * eval;

			// compute the rates of change of each chemical
            da[i][j] = 0.01f * aval * aval * eval * beta[i][j] / cval - aval * k_ab + diff1 * dda;
            db[i][j] = 0.01f * bval * bval * dval / cval - bval * k_ab + diff1 * ddb;
            dc[i][j] = 0.01f * aval * aval * eval * beta[i][j] + 0.01f * bval * bval * dval - cval * k_c;
            dd[i][j] = (aval - dval) * k_de + diff2 * ddd;
            de[i][j] = (bval - eval) * k_de + diff2 * dde;
        }
    }

    // effect change
    for(int i = 0; i < X; i++)
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] += (speed * da[i][j]);
            b[i][j] += (speed * db[i][j]);
            c[i][j] += (speed * dc[i][j]);
            d[i][j] += (speed * dd[i][j]);
            e[i][j] += (speed * de[i][j]);
        }
    }
}

