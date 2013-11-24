/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

Specifically following Wikipedia
http://en.wikipedia.org/wiki/File:Reaction_diffusion_spiral.gif

*/

// Original copyright notice:

/*

Make spots and stripes with reaction-diffusion.

The spot-formation system is described in the article:

  "A Model for Generating Aspects of Zebra and Other Mammailian
   Coat Patterns"
  Jonathan B. L. Bard
  Journal of Theoretical Biology, Vol. 93, No. 2, pp. 363-385
  (November 1981)

The stripe-formation system is described in the book:

  Models of Biological Pattern Formation
  Hans Meinhardt
  Academic Press, 1982


Permission is granted to modify and/or distribute this program so long
as the program is distributed free of charge and this header is retained
as part of the program.

Copyright (c) Greg Turk, 1991

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
             float tau,float du2,float dv2,float kappa,float lambda,float sigma,
             float speed,
			 bool parameter_map);

int main()
{
    // -- parameters --

	// hex:
	float tau = 0.1f;
	float du2 = 0.00024f * 10.0f;
	float dv2 = 0.005f * 10.0f;
	float kappa = -0.05f;
	float lambda = 1.0f;
	float sigma = 1.0f;

	// for spiral waves
	/*float tau = 4.0f;
	float du2 = 0.0015f * 10.0f;
	float dv2 = 0.01f * 10.0f;
	float kappa = -1.126f;
	float lambda = 4.67f;
	float sigma = -3.33f;*/

	float speed = 0.001f;
	bool parameter_map = false;
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
        compute(a,b,da,db,tau,du2,dv2,kappa,lambda,sigma,speed,parameter_map);

        // display:
        if(iteration%100==0) 
		{
            if(display(a,b,b,iteration,true,200.0f,1,10,"FitzHughNagumo.avi")) // did user ask to quit?
                break;
        }

		// to make more interesting patterns we periodically reset part of the grid
		/*if(iteration==300 || (iteration>0 && iteration%3000==0))
		{
			int div = rand()*Y/(float)RAND_MAX;
			for(int i = 0; i < X; i++) 
			{
				for(int j = 0; j < Y; j++) 
				{
					if(j<div)
					{
						a[i][j] = 0.0f;
						b[i][j] = 0.0f;
					}
				}
			}
		}*/

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
	float pos;
    for(int j = 0; j < Y; j++) 
	{
		if(j%10==0)
			pos = frand(8.0f,12.0f);
	    for(int i = 0; i < X; i++) 
		{
			a[i][j] = 0.0f;
			b[i][j] = 0.0f;
			//if(abs(i-X/2+pos)<10) // start with a wavy static line in the centre
			if(i>10 && i<20) // start with a wave on the left edge
			{
				a[i][j] = 1.0f;
			}
			if(i>1 && i<17)
			{
				b[i][j] = 0.1f;
			}
			// you can start from random conditions too, but it's harder to guarantee
			// interesting waves
		    a[i][j] = frand(-0.1f,0.1f);
		    b[i][j] = frand(-0.1f,0.1f);
        }
    }
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void compute(float a[X][Y],float b[X][Y],
             float da[X][Y],float db[X][Y],
             float tau,float du2,float dv2,float kappa,float lambda,float sigma,
			 float speed,
			 bool parameter_map)
{
	int iprev,inext,jprev,jnext;
	bool toroidal = false;

    // compute change in each cell
    for(int i = 0; i < X; i++) 
	{
		if(toroidal) { iprev = (i + X - 1) % X; inext = (i + 1) % X; }
		else { iprev=max(0,i-1); inext=min(X-1,i+1); }

        for(int j = 0; j < Y; j++) 
		{
			if(toroidal) { jprev = (j + Y - 1) % Y; jnext = (j + 1) % Y; }
			else { jprev=max(0,j-1); jnext=min(Y-1,j+1); }

            float aval = a[i][j];
            float bval = b[i][j];

			// compute the Laplacians of a and b
            float dda = a[i][jprev] + a[i][jnext] + a[iprev][j] + a[inext][j] - 4*aval;
            float ddb = b[i][jprev] + b[i][jnext] + b[iprev][j] + b[inext][j] - 4*bval;

			if(parameter_map)
			{
			}

			// compute the new rate of change of a and b
			da[i][j] = lambda*aval - aval*aval*aval - kappa - sigma*bval + du2*dda;
			db[i][j] = (aval - bval - dv2*ddb ) / tau;
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
