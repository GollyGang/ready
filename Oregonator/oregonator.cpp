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

void init(float a[X][Y],float b[X][Y],float c[X][Y]);

void compute(float a[X][Y],float b[X][Y],float c[X][Y],
             float da[X][Y],float db[X][Y],float dc[X][Y],
             float Da,float Db,float Dc,float q,float epsilon,float f,
             float speed,
             bool parameter_space);

int main()
{
    // Following:
    // http://hopf.chem.brandeis.edu/yanglingfa/pattern/oreg/index.html

    // But I can't get this to work at the moment.

    // -- parameters --
    float q = 0.01f;
    float epsilon = 0.5f;
    float f = 0.80f; // labyrinthine
    float Da = 3.0f; // (Dx,Dz,Dr at http://hopf.chem.brandeis.edu/yanglingfa/pattern/oreg/index.html )
    float Db = 100.0f;
    float Dc = 0.1f;
    float speed = 0.00001f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y], c[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y], dc[X][Y];

    // put the initial conditions into each cell
    init(a,b,c);
    
    int iteration = 0;
    while(true) {

        // compute:
        compute(a,b,c,da,db,dc,Da,Db,Dc,q,epsilon,f,speed,false);

        // display:
        if(iteration%10==0) 
        {
            if(display(a,b,c,iteration,true,20.0f,2,100,"Oregonator (Esc to quit)")) // did user ask to quit?
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

void init(float a[X][Y],float b[X][Y],float c[X][Y])
{
    srand((unsigned int)time(NULL));
    for(int i = 0; i < X; i++) 
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] = frand(-0.01f,0.01f);
            b[i][j] = frand(-0.01f,0.01f);
            c[i][j] = frand(-0.01f,0.01f);
        }
    }
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void compute(float a[X][Y],float b[X][Y],float c[X][Y],
             float da[X][Y],float db[X][Y],float dc[X][Y],
             float Da,float Db,float Dc,float q,float epsilon,float f,
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
            float cval = c[i][j];

            if(parameter_space)
            {
                /*const float A1=0.0f,A2=4.0f,B1=0.0f,B2=15.0f;
                A = A1+(A2-A1)*i/X;
                B = B2+(B1-B2)*j/Y;*/
            }

            // compute the Laplacians of a, b and c
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;
            float ddc = c[i][jprev] + c[i][jnext] + c[iprev][j] + c[inext][j] - 4*cval;

            // compute the new rate of change of a, b and c
            da[i][j] = Da*dda + ( aval - aval*aval - f*bval*(aval-q)/(aval+q) - (aval-cval)/2.0f ) / epsilon;
            db[i][j] = Db*ddb + aval - bval;
            dc[i][j] = Dc*ddc + ( aval - cval ) / ( 2.0f * epsilon );
        }
    }

    // effect change
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            a[i][j] += (speed * da[i][j]);
            b[i][j] += (speed * db[i][j]);
            c[i][j] += (speed * dc[i][j]);
        }
    }
}

