/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See ../../README.txt for more details.

*/

#include "gray_scott_scalar.h"
#include "../util.h"

static int g_width;
static int g_height;
static bool g_wrap;
static bool g_paramspace;

void gs_scl_compute_setup(int width, int height, bool wrap, bool paramspace)
{
  g_width = width;
  g_height = height;
  g_wrap = wrap;
  g_paramspace = paramspace;
}

void compute_gs_scalar(float *a, float *b, float *da, float *db,
             float r_a, float r_b, float f, float k, float speed)
{
    // compute change in each cell
    for(int i = 0; i < g_height; i++) {
        int iprev,inext;
        if (g_wrap) {
            iprev = (i + g_height - 1) % g_height;
            inext = (i + 1) % g_height;
        } else {
            iprev = max(0,i-1);
            inext = min(g_height-1,i+1);
        }

        for(int j = 0; j < g_width; j++) {
            int jprev,jnext;
            if (g_wrap) {
                jprev = (j + g_width - 1) % g_width;
                jnext = (j + 1) % g_width;
            } else {
                jprev = max(0,j-1);
                jnext = min(g_width-1,j+1);
            }

            float aval = a[i*g_width+j];
            float bval = b[i*g_width+j];

            if(g_paramspace)	{
                const float kmin=0.045, kmax=0.07, fmin=0.01, fmax=0.09;
                // set f and k for this location (ignore the provided values of f and k)
                k = kmin + (g_width-j-1)*(kmax-kmin)/g_width;
                f = fmin + (g_height-i-1)*(fmax-fmin)/g_height;
            }

            // compute the Laplacians of a and b
            float dda = a[i*g_width+jprev] + a[i*g_width+jnext] + a[iprev*g_width+j] + a[inext*g_width+j] - 4*aval;
            float ddb = b[i*g_width+jprev] + b[i*g_width+jnext] + b[iprev*g_width+j] + b[inext*g_width+j] - 4*bval;

            // compute the new rate of change of a and b
            da[i*g_width+j] = r_a * dda - aval*bval*bval + f*(1-aval);
            db[i*g_width+j] = r_b * ddb + aval*bval*bval - (f+k)*bval;
        }
    }

    // effect change
    for(int i = 0; i < g_width*g_height; i++) {
      a[i] += speed * da[i];
      b[i] += speed * db[i];
    }
}
