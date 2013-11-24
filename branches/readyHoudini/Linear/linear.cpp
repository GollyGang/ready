/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y],float b[X][Y]);

void compute(float a[X][Y],float b[X][Y],float da[X][Y],float db[X][Y]);

int main()
{
    // http://www.nature.com/ncomms/journal/v1/n6/full/ncomms1071.html

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
            compute(a,b,da,db);
            iteration++;
        }

		end = clock();

		char msg[1000];
		sprintf(msg,"Linear - %0.2f fps",N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC));

        // display:
        if(display(a,a,b,iteration,true,30.0f,2,10,msg)) // did user ask to quit?
            break;
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
    for(int i = 0; i < X; i++)
    {
        for(int j = 0; j < Y; j++) 
        {
            float m = 0.01f;
            a[i][j] = frand(-m,m);
            b[i][j] = frand(-m,m);
        }
    }
}

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y])
{
    // -- parameters --
    float A = 0.08f;
    float B = 0.08f;
    float C = 0.15f;
    float D = 0.03f;
    float E = 0.10f;
    float F = 0.12f;
    float G = 0.06f;
    float D_a = 0.5f;
    float D_b = 10.0f;
    float R = 20.0f;
    float syn_a_max = 0.23f;
    float syn_b_max = 0.50f;
    float speed = 0.001f;
    // ----------------
    
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

            // compute the synthesis rates of a and b
            float syn_a = A*aval + B*bval + C;
            float syn_b = E*aval - F;
            if(syn_a<0.0f) syn_a=0.0f; if(syn_a>syn_a_max) syn_a=syn_a_max;
            if(syn_b<0.0f) syn_b=0.0f; if(syn_b>syn_b_max) syn_b=syn_b_max;

            // compute the new rate of change of a and b
            da[i][j] = R*( syn_a - D*aval ) + D_a * dda;
            db[i][j] = R*( syn_b - G*bval ) + D_b * ddb;
        }
    }

    // effect change
    for(int i = 0; i < X; i++)
    {
        for(int j = 0; j < Y; j++) 
        {
            a[i][j] += (speed * da[i][j]);
            b[i][j] += (speed * db[i][j]);
        }
    }
}
