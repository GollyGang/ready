/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// hardware
#if (defined(__i386__) || defined(__amd64__) || defined(__x86_64__))
# include <xmmintrin.h>
#endif

// To convince yourself that the macro library works on any hardware,
// un-comment this "#define HWIV_EMULATE"
// #define HWIV_EMULATE
#define HWIV_WANT_V4F4
#include "hwi_vector.h";

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

// local:
#include "defs.h"
#include "display.h"

#define GRID_WIDTH  (X)
#define GRID_HEIGHT (Y)

void init(float a[GRID_HEIGHT][GRID_WIDTH],
          float b[GRID_HEIGHT][GRID_WIDTH]);

void compute(float a[GRID_HEIGHT][GRID_WIDTH],
             float b[GRID_HEIGHT][GRID_WIDTH],
             float da[GRID_HEIGHT][GRID_WIDTH],
             float db[GRID_HEIGHT][GRID_WIDTH],
             float D_u,float D_v,float F,float k,
             float speed,
             bool parameter_space);

void colorize(float u[GRID_HEIGHT][GRID_WIDTH],
             float v[GRID_HEIGHT][GRID_WIDTH],
             float du[GRID_HEIGHT][GRID_WIDTH],
             float red[GRID_HEIGHT][GRID_WIDTH],
             float green[GRID_HEIGHT][GRID_WIDTH],
             float blue[GRID_HEIGHT][GRID_WIDTH]);

static int g_color = 0;

int main(int argc, char * * argv)
{
  for (int i = 1; i < argc; i++) {
    if (0) {
    } else if (strcmp(argv[i],"-color")==0) {
      // do output in wonderful technicolor
      g_color = 1;
    } else {
      fprintf(stderr, "Unrecognized argument: '%s'\n", argv[i]);
      exit(-1);
    }
  }

#if (defined(__i386__) || defined(__amd64__) || defined(__x86_64__))
  /* On Intel we disable accurate handling of denorms and zeros. This is an
     important speed optimization. */
  int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
  int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
  _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
#endif

  // Here we implement the Gray-Scott model, as described here:
  // http://arxiv.org/abs/patt-sol/9304003
  //    (the seminal paper by Pearson)
  // http://www.cc.gatech.edu/~turk/bio_sim/hw3.html
  //    (a present university course project, by Greg Turk at Georgia Tech)
  // http://www.mrob.com/pub/comp/xmorphia/index.html
  //    (a web exhibit with over 100 videos and 500 images)

  // -- parameters --
  float D_u = 0.082f;
  float D_v = 0.041f;

  float k, F;
     k = 0.064;  F = 0.035;  // spots
  // k = 0.06;   F = 0.035;  // stripes
  // k = 0.065;  F = 0.056;  // long stripes
  // k = 0.064;  F = 0.04;   // dots and stripes
  // k = 0.0475; F = 0.0118; // spiral waves:
  float speed = 2.0f;
  // ----------------
  
  // these arrays store the chemical concentrations:
  float u[GRID_HEIGHT][GRID_WIDTH];
  float v[GRID_HEIGHT][GRID_WIDTH];
  // these arrays store the rate of change of those chemicals:
  float du[GRID_HEIGHT][GRID_WIDTH];
  float dv[GRID_HEIGHT][GRID_WIDTH];

  float red[GRID_HEIGHT][GRID_WIDTH];
  float green[GRID_HEIGHT][GRID_WIDTH];
  float blue[GRID_HEIGHT][GRID_WIDTH];

  // put the initial conditions into each cell
  init(u,v);
  
  const int N_FRAMES_PER_DISPLAY = 500;
  int iteration = 0;
  while(true) 
  {
    struct timeval tod_record;
    double tod_before, tod_after, tod_elapsed, fps;

    gettimeofday(&tod_record, 0);
    tod_before = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    // compute:
    for(int it=0;it<N_FRAMES_PER_DISPLAY;it++)
    {
      compute(u,v,du,dv,D_u,D_v,F,k,speed,false);
      iteration++;
    }

    if (g_color) {
      colorize(u, v, du, red, green, blue);
    }

    gettimeofday(&tod_record, 0);
    tod_after = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    tod_elapsed = tod_after - tod_before;
    fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elapsed;

    char msg[1000];
    sprintf(msg,"GrayScott - %0.2f fps", fps);

    // display:
    {
      int chose_quit;
      if (g_color) {
        chose_quit = display(red,green,blue,iteration,false,200.0f,2,10,msg);
      } else {
        chose_quit = display(u,u,u,iteration,false,200.0f,2,10,msg);
      }
      if (chose_quit) // did user ask to quit?
        break;
    }
  }
}

// return a random value between lower and upper
float frand(float lower,float upper)
{
  return lower + rand()*(upper-lower)/RAND_MAX;
}

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

void init(float a[GRID_HEIGHT][GRID_WIDTH],float b[GRID_HEIGHT][GRID_WIDTH])
{
  srand((unsigned int)time(NULL));
    
  // figure the values
  for(int i = 0; i < GRID_HEIGHT; i++) {
    for(int j = 0; j < GRID_WIDTH; j++) {
      if(hypot(i-GRID_HEIGHT/2,(j-GRID_WIDTH/2)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
      {
        a[i][j] = frand(0.0,0.1);
        b[i][j] = frand(0.9,1.0);
      }
      else {
        a[i][j] = frand(0.9,1.0);
        b[i][j] = frand(0.0,0.1);
      }
    }
  }
}

void compute(float u[GRID_HEIGHT][GRID_WIDTH],
             float v[GRID_HEIGHT][GRID_WIDTH],
             float du[GRID_HEIGHT][GRID_WIDTH],
             float dv[GRID_HEIGHT][GRID_WIDTH],
             float D_u,float D_v,float F,float k,float speed,
             bool parameter_space)
{
#ifndef HWIV_HAVE_V4F4
  fprintf(stdout, "Did not get vector macros from HWIV\n");
  exit(-1);
#endif
  V4F4 v4_speed; // vectorized version of speed scalar
  V4F4 v4_F; // vectorized version of F scalar
  V4F4 v4_k; // vectorized version of k scalar
  HWIV_4F4_ALIGNED talign; // used by FILL_4F4
  V4F4 v4_u; V4F4 v4_du;
  V4F4 v4_v; V4F4 v4_dv;
  HWIV_INIT_MUL0_4F4(mul_tmp); // used by MUL (on targets that need it)
  HWIV_INIT_MTMP_4F4(fma_tmp); // used by MADD (on targets that need it)
  V4F4 v4_neighbor;
  V4F4 v4_Du;
  V4F4 v4_Dv;
  V4F4 v4_nabu;
  V4F4 v4_nabv;
  V4F4 v4_1;
  V4F4 v4_4;

  // Initialize our vectorized scalars
  HWIV_FILL_4F4(v4_speed, speed, speed, speed, speed, talign);
  HWIV_FILL_4F4(v4_F, F, F, F, F, talign);
  HWIV_FILL_4F4(v4_k, k, k, k, k, talign);
  HWIV_FILL_4F4(v4_Du, D_u, D_u, D_u, D_u, talign);
  HWIV_FILL_4F4(v4_Dv, D_v, D_v, D_v, D_v, talign);
  HWIV_FILL_4F4(v4_1, 1.0, 1.0, 1.0, 1.0, talign);
  HWIV_FILL_4F4(v4_4, 4.0, 4.0, 4.0, 4.0, talign);

  // Scan per row
  for(int i = 0; i < GRID_HEIGHT; i++) {
    int iprev,inext;
    iprev = max(0,i-1);
    inext = min(GRID_HEIGHT-1,i+1);

    for(int j = 0; j < GRID_WIDTH; j+=4) {
      int jprev,jnext;

      jprev = max(0,j-4);
      jnext = min(GRID_WIDTH-4,j+4);

    //  float uval = u[i][j];
      HWIV_LOAD_4F4(v4_u, &(u[i][j]));
    //  float vval = v[i][j];
      HWIV_LOAD_4F4(v4_v, &(v[i][j]));

      if (parameter_space) {
        const float k_min=0.045f,k_max=0.07f,F_min=0.00f,F_max=0.14f;
        // set F and k for this location (ignore the provided values of f and k)
        k = k_min + i*(k_max-k_min)/GRID_HEIGHT;
        HWIV_FILL_4F4(v4_k, k, k, k, k, talign);

        F = F_min + j*(F_max-F_min)/GRID_WIDTH;
        // FIXME: This vectorize is an approximation, the F value should
        // vary slightly between the 4 elements of the vector
        HWIV_FILL_4F4(v4_F, F, F, F, F, talign);
      }

      // compute the Laplacians of u and v. "nabla" is the name of the
      // "upside down delta" symbol used for the Laplacian in equations
     // float nabla_u, nabla_v;

     // nabla_u = u[i][jprev] + u[i][jnext] + u[iprev][j] + u[inext][j] - 4*uval;
      // The first two are hard to load because they require unaligned access.
      // We instead need to load a whole vector from the neighboring location
      // and shift (raise or lower) the contents by one index.
      HWIV_LOAD_4F4(v4_nabu, &(u[i][jprev]));

      HWIV_LOAD_4F4(v4_neighbor, &(u[i][jnext]));
      HWIV_ADD_4F4(v4_nabu, v4_nabu, v4_neighbor);
      HWIV_LOAD_4F4(v4_neighbor, &(u[iprev][j]));
      HWIV_ADD_4F4(v4_nabu, v4_nabu, v4_neighbor);
      HWIV_LOAD_4F4(v4_neighbor, &(u[inext][j]));
      HWIV_ADD_4F4(v4_nabu, v4_nabu, v4_neighbor);
      HWIV_NMSUB_4F4(v4_nabu, v4_4, v4_u, v4_nabu, fma_tmp);

     // nabla_v = v[i][jprev] + v[i][jnext] + v[iprev][j] + v[inext][j] - 4*vval;
      HWIV_LOAD_4F4(v4_nabv, &(v[i][jprev]));
      HWIV_LOAD_4F4(v4_neighbor, &(v[i][jnext]));
      HWIV_ADD_4F4(v4_nabv, v4_nabv, v4_neighbor);
      HWIV_LOAD_4F4(v4_neighbor, &(v[iprev][j]));
      HWIV_ADD_4F4(v4_nabv, v4_nabv, v4_neighbor);
      HWIV_LOAD_4F4(v4_neighbor, &(v[inext][j]));
      HWIV_ADD_4F4(v4_nabv, v4_nabv, v4_neighbor);
      HWIV_NMSUB_4F4(v4_nabv, v4_4, v4_v, v4_nabv, fma_tmp);

      // compute the new rate of change of u and v
     // du[i][j] = D_u * nabla_u - uval*vval*vval + F*(1-uval);
              // D_u * nabla_u - (uval*vval*vval - (-(F*uval-F)) )
      HWIV_NMSUB_4F4(v4_neighbor, v4_F, v4_u, v4_F, fma_tmp);
      HWIV_MUL_4F4(v4_dv, v4_v, v4_v, mul_tmp);
      HWIV_MSUB_4F4(v4_neighbor, v4_u, v4_dv, v4_neighbor, fma_tmp);
      HWIV_MSUB_4F4(v4_du, v4_Du, v4_nabu, v4_neighbor, fma_tmp);
      HWIV_SAVE_4F4(&(du[i][j]), v4_du);
     // dv[i][j] = D_v * nabla_v + uval*vval*vval - (F+k)*vval;
              // D_v * nabla_v + uval*vval*vval - (F*vval + k*vval);
      HWIV_MUL_4F4(v4_neighbor, v4_k, v4_v, mul_tmp);
      HWIV_MADD_4F4(v4_neighbor, v4_F, v4_v, v4_neighbor, fma_tmp);
      HWIV_MUL_4F4(v4_dv, v4_v, v4_v, mul_tmp);
      HWIV_MSUB_4F4(v4_neighbor, v4_u, v4_dv, v4_neighbor, fma_tmp);
      HWIV_MADD_4F4(v4_dv, v4_Dv, v4_nabv, v4_neighbor, fma_tmp);
      HWIV_SAVE_4F4(&(dv[i][j]), v4_dv);
    }
  }

  // effect change
  {

    for(int i = 0; i < GRID_HEIGHT; i++) {
      for(int j = 0; j < GRID_WIDTH; j+=4) {
        // u[i][j] = u[i][j] + speed * du[i][j];
        HWIV_LOAD_4F4(v4_u, &(u[i][j]));
        HWIV_LOAD_4F4(v4_du, &(du[i][j]));
        HWIV_MADD_4F4(v4_u, v4_speed, v4_du, v4_u, fma_tmp);
        HWIV_SAVE_4F4(&(u[i][j]), v4_u);

        // v[i][j] = v[i][j] + speed * dv[i][j];
        HWIV_LOAD_4F4(v4_v, &(v[i][j]));
        HWIV_LOAD_4F4(v4_dv, &(dv[i][j]));
        HWIV_MADD_4F4(v4_v, v4_speed, v4_dv, v4_v, fma_tmp);
        HWIV_SAVE_4F4(&(v[i][j]), v4_v);
      }
    }
  }
}

void colorize(float u[GRID_HEIGHT][GRID_WIDTH],
             float v[GRID_HEIGHT][GRID_WIDTH],
             float du[GRID_HEIGHT][GRID_WIDTH],
             float red[GRID_HEIGHT][GRID_WIDTH],
             float green[GRID_HEIGHT][GRID_WIDTH],
             float blue[GRID_HEIGHT][GRID_WIDTH])
{
  // Step by row
  for(int i = 0; i < GRID_HEIGHT; i++) {
    // step by column
    for(int j = 0; j < GRID_WIDTH; j++) {
      float uval = u[i][j];
      float vval = v[i][j];
      float delta_u = ((du[i][j]) * 1000.0f) + 0.5f;
      delta_u = ((delta_u < 0) ? 0.0 : (delta_u > 1.0) ? 1.0 : delta_u);

      // Something simple to start (-:
      // different colour schemes result if you reorder these, or replace
      // "x" with "1.0f-x" for any of the 3 variables
      red[i][j] = delta_u; // increasing U will look pink
      green[i][j] = 1.0-uval;
      blue[i][j] = 1.0-vval;
    }
  }
}
