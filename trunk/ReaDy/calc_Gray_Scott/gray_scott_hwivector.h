/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

//#define DICEK_EMULATE
#include "dicek.h"

typedef struct compute_params {
  DICEK_THREAD_VARS;
  float *u;
  float *v;
  float *du;
  float *dv;
  float D_u;
  float D_v;
  float F;
  float k;
  float speed;
  int num_its;
  long start_row;
  long end_row;
  int interlock_type;
} compute_params;

void compute_dispatch(float *u, float *v, float *du, float *dv,
  float D_u, float D_v, float F, float k, float speed,
  int num_its, int num_threads);

void * compute_gs_hwiv(void * param_block);  // Arg is really "compute_params * param_block"
