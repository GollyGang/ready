/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

#include <time.h>
#include <stdlib.h>

#include "brusselator.h"
#include "../util.h"

static int g_width;
static int g_height;
static bool g_wrap;
static bool g_paramspace;


void bruss_init(float *a, float *b, int width, int height, float A, float B)
{
  srand((unsigned int)time(NULL));
    
  // figure the values
  for(int i = 0; i < width*height; i++) 
  {
    a[i] = A + ut_frand(-0.01f,0.01f);
    b[i] = B/A + ut_frand(-0.01f,0.01f);
  }
}

void bruss_compute_setup(int width, int height, bool wrap, bool paramspace)
{
  g_width = width;
  g_height = height;
  g_wrap = wrap;
  g_paramspace = paramspace;
}

void compute_bruss(float *a, float *b, float *da, float *db,
             float A, float B, float D1, float D2, float speed)
{
    // compute change in each cell
    for(int i = 0; i < g_height; i++) 
    {
        int iprev, inext;
        if (g_wrap) {
          iprev = (i + g_height - 1) % g_height;
          inext = (i + 1) % g_height;
        } else {
          iprev = max(0,i-1);
          inext = min(g_height-1,i+1);
        }

        for(int j = 0; j < g_width; j++) 
        {
            int jprev, jnext;
            if (g_wrap) {
              jprev = (j + g_width - 1) % g_width;
              jnext = (j + 1) % g_width;
            } else {
              jprev = max(0,j-1);
              jnext = min(g_width-1,j+1);
            }

            float aval = a[i*g_width+j];
            float bval = b[i*g_width+j];

            if(g_paramspace)
            {
                const float A1=0.0, A2=4.0, B1=0.0, B2=15.0;
                A = A1+(A2-A1)*((float)i)/((float)g_height);
                B = B1+(B2-B1)*((float)j)/((float)g_width);
            }

            // compute the Laplacians of a and b
            float dda = a[i*g_width+jprev] + a[i*g_width+jnext] + a[iprev*g_width+j] + a[inext*g_width+j] - 4*aval;
            float ddb = b[i*g_width+jprev] + b[i*g_width+jnext] + b[iprev*g_width+j] + b[inext*g_width+j] - 4*bval;

            // compute the new rate of change of a and b
            da[i*g_width+j] = A-(B+1)*aval + aval*aval*bval + D1*dda;
            db[i*g_width+j] = B*aval - aval*aval*bval + D2*ddb;
        }
    }

    // effect change
    for(int i = 0; i < g_width*g_height; i++) {
        a[i] += (speed * da[i]);
        b[i] += (speed * db[i]);
    }
}

