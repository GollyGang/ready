/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See ../../README.txt for more details.

*/

#include "gray_scott_scalar.h"

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
# define min(a,b) (((a) < (b)) ? (a) : (b))
# define minmax(v, lo, hi) max(lo, min(v, hi))
#endif

void compute_gs_scalar(float *a, float *b, float *da, float *db, long width, long height, bool toroidal,
             float r_a,float r_b,float f,float k,
             float speed)
{
    //const bool toroidal = false;

    //int iprev,inext,jprev,jnext;

    // compute change in each cell
    for(long i = 0; i < height; i++) {
        long iprev,inext;
        if (toroidal) {
            iprev = (i + height - 1) % height;
            inext = (i + 1) % height;
        } else {
            iprev = max(0,i-1);
            inext = min(height-1,i+1);
        }

        for(long j = 0; j < width; j++) {
            long jprev,jnext;
            if (toroidal) {
                jprev = (j + width - 1) % width;
                jnext = (j + 1) % width;
            } else {
                jprev = max(0,j-1);
                jnext = min(width-1,j+1);
            }

            float aval = a[i*width+j];
            float bval = b[i*width+j];

            // compute the Laplacians of a and b
            float dda = a[i*width+jprev] + a[i*width+jnext] + a[iprev*width+j] + a[inext*width+j] - 4*aval;
            float ddb = b[i*width+jprev] + b[i*width+jnext] + b[iprev*width+j] + b[inext*width+j] - 4*bval;

            // compute the new rate of change of a and b
            da[i*width+j] = r_a * dda - aval*bval*bval + f*(1-aval);
            db[i*width+j] = r_b * ddb + aval*bval*bval - (f+k)*bval;
        }
    }

    // effect change
    for(long i = 0; i < width*height; i++) {
      a[i] += speed * da[i];
      b[i] += speed * db[i];
    }
}
