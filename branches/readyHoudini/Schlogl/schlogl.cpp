/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// local:
#include "defs.h"
#include "display.h"

void init(float a[X][Y]);

void compute(float a[X][Y],
             float da[X][Y],
             float speed);

int main()
{
    // Schlogl model:
    // F. Schlogl, "Chemical reaction models for non-equilibrium phase transitions", Zeitschrift fur Physik, 253, 147, (1972) 
    // Following:
    // http://wwwnlds.physik.tu-berlin.de/~hizanidis/talks/front_propagation_noise.pdf

    // -- parameters --
    float speed = 0.1f;
    // ----------------
    
    // this array stores the chemical concentrations:
    float a[X][Y];
    // this array store the rate of change of those chemicals:
    float da[X][Y];

    // put the initial conditions into each cell
    init(a);
    
	clock_t start,end;

	const int N_FRAMES_PER_DISPLAY = 100;
    int iteration = 0;
    while(true) 
    {
    	start = clock();

        // compute:
		for(int it=0;it<N_FRAMES_PER_DISPLAY;it++)
		{
            compute(a,da,speed);
            iteration++;
        }

		end = clock();

		char msg[1000];
		sprintf(msg,"Schlogl - %0.2f fps",N_FRAMES_PER_DISPLAY / ((end-start)/(float)CLOCKS_PER_SEC));

        // display:
        if(display(a,a,a,iteration,true,200.0f,2,10,msg)) // did user ask to quit?
            break;
    }
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
    return lower + rand()*(upper-lower)/RAND_MAX;
}

void init(float a[X][Y])
{
    srand((unsigned int)time(NULL));
    
    // figure the values
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            a[i][j] = frand(-1.0f,1.0f);
        }
    }
}

void compute(float a[X][Y],
             float da[X][Y],
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

            // compute the Laplacian of a
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;

            // compute the new rate of change of a
            da[i][j] = dda + aval*(1-aval)*(1+aval); // a-a^3
        }
    }

    // effect change
    for(int i = 0; i < X; i++) {
        for(int j = 0; j < Y; j++) {
            a[i][j] += (speed * da[i][j]);
        }
    }
}
