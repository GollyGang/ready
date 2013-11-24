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
#include <stdio.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y],float beta[X][Y],
          float a_steady,float b_steady,float beta_init,float beta_rand);

void compute(float a[X][Y],float b[X][Y],float beta[X][Y],
             float da[X][Y],float db[X][Y],
             float diff1,float diff2,float p1,float speed);

int main()
{
    // -- parameters --
    float diff1 = 0.125f;
    float diff2 = 0.03125f;
    float s = 0.0125f;
    float speed = 1.0f;
    float a_steady = 4.0f;
    float b_steady = 4.0f;
    float beta_init = 12.0f;
    float beta_rand = 0.1f;
    // ----------------
    
    // these arrays store the chemical concentrations:
    float a[X][Y], b[X][Y], beta[X][Y];
    // these arrays store the rate of change of those chemicals:
    float da[X][Y], db[X][Y];

    // put the initial conditions into each cell
    init(a,b,beta,a_steady,b_steady,beta_init,beta_rand);
    
	clock_t start,end;

	const int N_FRAMES_PER_DISPLAY = 100;
    int iteration = 0;
    while(true)
    {
		start = clock();

        // compute:
		for(int it=0;it<N_FRAMES_PER_DISPLAY;it++)
		{
            compute(a,b,beta,da,db,diff1,diff2,s,speed);
            iteration++;
        }

		end = clock();

		char msg[1000];
		sprintf(msg,"Turing spots - %0.2f fps",N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC));

        // display:
        if(display(a,a,b,iteration,false,30.0f,2,10,msg)) // did user ask to quit?
            break;
    }
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

void init(float a[X][Y],float b[X][Y],float beta[X][Y],
          float a_steady,float b_steady,float beta_init,float beta_rand)
{
    srand((unsigned int)time(NULL));
    for(int i = 0; i < X; i++)
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] = a_steady;
            b[i][j] = b_steady;
            //if(i<X/2)
                beta[i][j] = beta_init + frand(-beta_rand, beta_rand);
            /*else
                beta[i][j] = beta_init;*/
        }
    }
}

void compute(float a[X][Y],float b[X][Y],float beta[X][Y],
             float da[X][Y],float db[X][Y],
             float diff1,float diff2,float s,float speed)
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
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4 * aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4 * bval;

            // compute the new rate of change of a and b
            da[i][j] = s * (16 - aval * bval) + diff1 * dda;
            db[i][j] = s * (aval * bval - bval - beta[i][j]) + diff2 * ddb;
        }
    }

    // effect change
    for(int i = 0; i < X; i++)
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] += (speed * da[i][j]);
            b[i][j] += (speed * db[i][j]);
            if (b[i][j] < 0)
                b[i][j] = 0;
        }
    }
}
