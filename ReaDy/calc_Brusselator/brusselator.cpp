/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

#include <time.h>
#include <stdlib.h>

// return a random value between lower and upper
float br_frand(float lower,float upper)
{
  return lower + rand()*(upper-lower)/RAND_MAX;
}

void bruss_init(float *a, float *b, long width, long height, float A, float B)
{
  srand((unsigned int)time(NULL));
    
  // figure the values
  for(long i = 0; i < width*height; i++) 
  {
    a[i] = A + br_frand(-0.01f,0.01f);
    b[i] = B/A + br_frand(-0.01f,0.01f);
  }
}

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
# define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void compute_bruss(float *a, float *b, float *da, float *db, long width, long height,
             float A, float B, float D1, float D2, float speed, bool parameter_space)
{
    // compute change in each cell
    for(long i = 0; i < height; i++) 
    {
        //long iprev = (i + height - 1) % height;
        //long inext = (i + 1) % height;
        long iprev = max(0,i-1);
        long inext = min(height-1,i+1);

        for(long j = 0; j < width; j++) 
        {
            //long jprev = (j + width - 1) % width;
            //long jnext = (j + 1) % width;
            long jprev = max(0,j-1);
            long jnext = min(width-1,j+1);

            float aval = a[i*width+j];
            float bval = b[i*width+j];

            if(parameter_space)
            {
                const float A1=0.0f,A2=4.0f,B1=0.0f,B2=15.0f;
                A = A1+(A2-A1)*i/height;
                B = B1+(B2-B1)*j/width;
            }

            // compute the Laplacians of a and b
            float dda = a[i*width+jprev] + a[i*width+jnext] + a[iprev*width+j] + a[inext*width+j] - 4*aval;
            float ddb = b[i*width+jprev] + b[i*width+jnext] + b[iprev*width+j] + b[inext*width+j] - 4*bval;

            // compute the new rate of change of a and b
            da[i*width+j] = A-(B+1)*aval + aval*aval*bval + D1*dda;
            db[i*width+j] = B*aval - aval*aval*bval + D2*ddb;
        }
    }

    // effect change
    for(long i = 0; i < width*height; i++) {
        a[i] += (speed * da[i]);
        b[i] += (speed * db[i]);
    }
}

