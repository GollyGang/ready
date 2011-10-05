/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// hardware
#if (defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || 	defined(_M_X64) || defined(_M_IX86))
# include <xmmintrin.h>
#endif

// To convince yourself that the macro library works on any hardware,
// un-comment this "#define HWIV_EMULATE", and you'll get the macros inside
// the #ifdef HWIV_V4F4_EMULATED block in hwi_vector.h.  The emulated
// macros do everything with normal floats and arrays, and run about 3-4 times
// slower.
//#define HWIV_EMULATE
#define HWIV_WANT_V4F4
#include "hwi_vector.h";

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
    #include <sys/timeb.h>
    #include <sys/types.h>
    #include <winsock.h>
    // http://www.linuxjournal.com/article/5574
    void gettimeofday(struct timeval* t,void* timezone)
    {       struct _timeb timebuffer;
          _ftime( &timebuffer );
          t->tv_sec=timebuffer.time;
          t->tv_usec=1000*timebuffer.millitm;
    }

	// TODO: Make sure this is the correct include file for the FindFirstFile function (used in FILE_LENGTH macro below)
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <sys/time.h>
#endif

// local:
//#define DICEK_EMULATE
#include "dicek.h"
#include "display_hwiv.h"

static long g_width = 256;
static long g_height = 256;

float *allocate(long width, long height, const char * error_text, bool for_mm);
float *allocate(long width, long height, const char * error_text, bool for_mm)
{
  long sz;
  float * rv;

  sz = width *  height * sizeof(*rv);
  if (for_mm) {
    rv = (float *) _mm_malloc(sz, 16);
  } else {
    rv = (float *) malloc(((size_t)sz));
  }

   if (rv == NULL) {
    fprintf(stderr, "allocate: Could not get %ld bytes for %s\n", ((long) sz),
      error_text);
    exit(-1);
  }
  return (rv);
}

void init(float *a, float *b, long width, long height);

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
  int parameter_space;
  int num_its;
  long start_row;
  long end_row;
  int interlock_type;
} compute_params;

void * compute(void * param_block);  // Arg is really "compute_params * param_block"

void compute_dispatch(float *u, float *v, float *du, float *dv,
  float D_u, float D_v, float F, float k, float speed,
  int parameter_space, int num_its, int num_threads);

static int g_paramspace = 0;
static int g_wrap = 0;
static float g_k = 0.064;
static float g_F = 0.035;
static bool g_video = false;

#define VECSIZE 4

int main(int argc, char * * argv)
{
  DICEK_INIT_NTHR(g_threads)
  printf("DiceK reports %d threads.\n", g_threads);

  // Here we implement the Gray-Scott model, as described here:
  // http://arxiv.org/abs/patt-sol/9304003
  //    (the seminal paper by Pearson)
  // http://www.cc.gatech.edu/~turk/bio_sim/hw3.html
  //    (a present university course project, by Greg Turk at Georgia Tech)
  // http://www.mrob.com/pub/comp/xmorphia/index.html
  //    (a web exhibit with over 100 videos and 500 images)

  // -- parameters --
  float D_u = 0.082;
  float D_v = 0.041;

  // The default is equivalent to options:   -F 0.035 -k 0.064
  g_k = 0.064;  g_F = 0.035;

  // Other pattern-types to try (pass -F and -k as argument on command line, some require "-density 1" as well):
  //
  // -F 0.0118 -k 0.0475              Spiral waves
  // -F 0.022  -k 0.059               Spots that multiply and keep killing each other off
  // -F 0.035  -k 0.06                Stripes with branching (fingerprint)
  // -F 0.04   -k 0.064               For spots that multiply, with stripes mixed in
  // -F 0.056  -k 0.065               Long stripes ("-density 1" option helps here)
  // -F 0.062  -k 0.0609 -density 2   "Uskate world", where I found all the Wolfram-class-4 behaviour
  // -F 0.094  -k 0.059  -density 1   "soap bubbles"
  // -F 0.094  -k 0.057 -lowback -density 1   Inverse soap bubbles

  float speed = 1.0;

  for (int i = 1; i < argc; i++) {
    if (0) {
    } else if ((i+1<argc) && (strcmp(argv[i],"-height")==0)) {
      // set height
      i++; g_height = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-size")==0)) {
      // set both width and height
      i++; g_height = g_width = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-threads")==0)) {
      // specify number of threads to use
      i++; g_threads = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-width")==0)) {
      // set width
      i++; g_width = atoi(argv[i]);
    } else if (strcmp(argv[i],"-wrap")==0) {
      // patterns wrap around ("torus", also called "continuous boundary
      // condition")
      g_wrap = 1;
    } else {
      fprintf(stderr, "Unrecognized argument, or parameter needed: '%s'\n", argv[i]);
      exit(-1);
    }
  }

  if (g_width % VECSIZE) {
    g_width = ((g_width/VECSIZE)+1)*VECSIZE;
  }
  long full_width = g_width + 2*VECSIZE;

  /* We cannot use multiple threads unless DiceK supports thread synchronization */
  if (!(DICEK_SUPPORTS_BLOCKING)) {
    g_threads = 1;
  }

  printf("Will use %d threads.\n", g_threads);

  // ----------------
  
  // these arrays store the chemical concentrations:
  float *u = 0;
  float *v = 0;
  // these arrays store the rate of change of those chemicals:
  float *du = 0;
  float *dv = 0;

  float *red = 0;
  float *green = 0;
  float *blue = 0;

  u = allocate(full_width, g_height, "U array", true);
  v = allocate(full_width, g_height, "V array", true);
  du = allocate(full_width, g_height, "D_u array", true);
  dv = allocate(full_width, g_height, "D_v array", true);
  red = allocate(full_width, g_height, "red array", false);
  green = allocate(full_width, g_height, "green array", false);
  blue = allocate(full_width, g_height, "blue array", false);

  // put the initial conditions into each cell
  init(u,v, full_width, g_height);

#define N_FRAMES_PER_DISPLAY 1000

  double iteration = 0;
  double fps_avg = 0.0; // decaying average of fps
  double Mcgs;
  while(true) 
  {
    struct timeval tod_record;
    double tod_before, tod_after, tod_elapsed;
    double fps = 0.0;     // frames per second

    gettimeofday(&tod_record, 0);
    tod_before = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    // compute:
    compute_dispatch(u, v, du, dv, D_u, D_v, g_F, g_k, speed, g_paramspace, N_FRAMES_PER_DISPLAY, g_threads);
    iteration += ((double)N_FRAMES_PER_DISPLAY);

    gettimeofday(&tod_record, 0);
    tod_after = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    tod_elapsed = tod_after - tod_before;
    fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elapsed;
    // We display an exponential moving average of the fps measurement
    fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0);
    Mcgs = fps_avg * ((double) full_width) * ((double) g_height) / 1.0e6;

    char msg[1000];
    sprintf(msg,"GrayScott - %0.2f fps (%.2f Mcgs)", fps_avg, Mcgs);

    // display:
    {
      int chose_quit;
      chose_quit = display(g_width,g_height,u,u,u,iteration,1.0,false,200.0f,2,10,msg,false);
      if (chose_quit) // did user ask to quit?
        break;
    }
  }
}

/* Spawn threads and dispatch to compute() routine inside threads */
void compute_dispatch(float *u, float *v, float *du, float *dv,
  float D_u, float D_v, float F, float k, float speed,
  int parameter_space, int num_its, int nthreads)
{
  if (nthreads <= 0) {
    nthreads = 1;
  }
  DICEK_DATA(compute_params, cp, nthreads);

  long i;
  long a_row = 0;

  /* Set up all the parameter blocks */
  for(i=0; i<nthreads; i++) {
    cp[i].u = u; cp[i].v = v; cp[i].du = du; cp[i].dv = dv; cp[i].D_u = D_u; cp[i].D_v = D_v;
    cp[i].F = F; cp[i].k = k; cp[i].speed = speed; cp[i].parameter_space = parameter_space;
    cp[i].num_its = num_its; cp[i].start_row = a_row;
    a_row = (g_height * (i+1)) / nthreads;
    cp[i].end_row = a_row;
    cp[i].interlock_type = (nthreads>1) ? 1 : 0;
  }

  if (nthreads > 1) {
    /* Start N threads, each will immediately begin the first part of its computation */
    DICEK_SPLIT(compute, cp, nthreads);

    /* Now for each iteration we need to sync the threads twice. Each iteration consists of two
       work phases: during the first phase u and v are being read and du,dv are written; during
       the second phase u,v are overwritten with the next generation. */
    for(i=0; i<num_its; i++) {
      DICEK_INTERLOCK(cp, nthreads); // Wait for threads to complete derivative calculation

      /* Now the threads have all finished the "compute derivative" loop */

      DICEK_RESUME(cp, nthreads);  // Wait for threads to update u and v arrays with new generation
      DICEK_INTERLOCK(cp, nthreads);

      /* Now the threads have finished updating u and v arrays with the next generation */

      DICEK_RESUME(cp, nthreads); // Tell threads they can now do the next iteration
    }
    DICEK_MERGE(compute, cp, nthreads);
  } else {
    /* With 1 thread it's more efficient to just call the compute routine directly */
    compute((void *)&(cp[0]));
  }
}

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
# define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef minmax
# define minmax(v, lo, hi) max(lo, min(v, hi))
#endif

// return a random value between lower and upper
float frand(float lower,float upper)
{
  float rv;

  rv = lower + rand()*(upper-lower)/RAND_MAX;
  rv = minmax(rv, 0.0, 1.0);
  return rv;
}

void init(float *u, float *v, long width, long height)
{
  long nsp, i;
  long base, var;

  srand((unsigned int)time(NULL));

  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      // start with a uniform field with an approximate circle in the middle
      if (hypot(i-height/2,j-width/3)<=frand(2,5))
      {
        u[i*width+j] = frand(0.0,0.1);
        v[i*width+j] = frand(0.9,1.0);
      } else {
        u[i*width+j] = frand(0.9,1.0);
        v[i*width+j] = frand(0.0,0.1);
      }
    }
  }
}

#define INDEX(a,x,y) ((a)[(x)*full_width+(y)])

/* The parameter space code, specifically the "if(parameter_space)" test itself, causes a 2.5% slowdown even when the
   parameter_space flag is false */
//#define SUPPORT_PARAM_SPACE

void * compute(void * gpb)
{
  DICEK_SUB(compute_params, gpb);
  const int full_width = g_width + 2*VECSIZE;
  const int wid_v = full_width / VECSIZE;

#if (defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || 	defined(_M_X64) || defined(_M_IX86))
  /* On Intel we disable accurate handling of denorms and zeros. This is an
     important speed optimization. */
  int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
  int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
  _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
#endif

  compute_params * param_block;
  param_block = (compute_params *) gpb;
  float *u = param_block->u;
  float *v = param_block->v;
  float *du = param_block->du;
  float *dv = param_block->dv;
  float D_u = param_block->D_u;
  float D_v = param_block->D_v;
  float F = param_block->F;
  float k = param_block->k;
  float speed = param_block->speed;
  int parameter_space = param_block->parameter_space;
  int num_its = param_block->num_its;
  int start_row = param_block->start_row;
  int end_row = param_block->end_row;
  int interlock = param_block->interlock_type;



  int iter;

#ifdef SUPPORT_PARAM_SPACE
  const float k_min=0.045f, k_max=0.07f, F_min=0.01f, F_max=0.09f;
  float k_diff;
  V4F4 v4_kdiff;
  k_diff = (k_max-k_min)/g_height;
  v4_kdiff = v4SET(0, -k_diff, -2*k_diff, -3*k_diff);
#endif

  // Initialize our vectorized scalars

  // Scan per iteration
  for(iter = 0; iter < num_its; iter++) {

  if (interlock) { DICEK_CH_BEGIN }

//printf("iter %d rows [%ld,%ld)\n",iter,start_row,end_row);

  // Scan per row
  for(int i = start_row; i < end_row; i++) {
#ifdef SUPPORT_PARAM_SPACE
    V4F4 v4_F = v4SPLAT(F);
    V4F4 v4_k = v4SPLAT(k);
#else
    const V4F4 v4_F = v4SPLAT(F);
    const V4F4 v4_k = v4SPLAT(k);
#endif
    const V4F4 v4_Du = v4SPLAT(D_u);
    const V4F4 v4_Dv = v4SPLAT(D_v);
    int iprev,inext;
    if (g_wrap) {
      /* Periodic boundary condition */
      iprev = (i+g_height-1) % g_height;
      inext = (i+1) % g_height;
    } else {
      /* The edges are their own neighbors. This amounts to a von Neumann boundary condition. */
      iprev = max(i-1, 0);
      inext = min(i+1, g_height-1);
    }

#ifdef SUPPORT_PARAM_SPACE
    if (parameter_space) {
      // set F for this row (ignore the provided value)
      F = F_min + (g_height-i-1) * (F_max-F_min)/g_width;
      v4_F = v4SPLAT(F);
    }
#endif

    // Scan per column in steps of vector width
    for(int j = 1; j < wid_v-1; j++) {
      V4F4 * v_ubase = ((V4F4 *)u)+i*wid_v+j;
      V4F4 * v_vbase = ((V4F4 *)v)+i*wid_v+j;
      V4F4 * v_dubase = ((V4F4 *)du)+i*wid_v+j;
      V4F4 * v_dvbase = ((V4F4 *)dv)+i*wid_v+j;
      V4F4 u_left = _mm_loadu_ps(((float*)v_ubase)-1);
      V4F4 u_right = _mm_loadu_ps(((float*)v_ubase)+1);
      V4F4 v_left = _mm_loadu_ps(((float*)v_vbase)-1);
      V4F4 v_right = _mm_loadu_ps(((float*)v_vbase)+1);
      V4F4 * v_ub_prev = ((V4F4 *)u)+iprev*wid_v+j;
      V4F4 * v_ub_next = ((V4F4 *)u)+inext*wid_v+j;
      V4F4 * v_vb_prev = ((V4F4 *)v)+iprev*wid_v+j;
      V4F4 * v_vb_next = ((V4F4 *)v)+inext*wid_v+j;

#ifdef SUPPORT_PARAM_SPACE
      if (parameter_space) {
        // set k for this column (ignore the provided value)
        k = k_min + (g_width-(j*4)-5)*k_diff;
        // k decreases by k_diff each time j increases by 1, so this vector
        // needs to contain 4 different k values. We use v4_kdiff, pre-computed
        // above, to accomplish this.
        v4_k = v4ADD(v4SPLAT(k),v4_kdiff);
      }
#endif

      // To compute the Laplacians of u and v, we use the 5-point neighbourhood for the Euler discrete method:
      //    nabla(x) = x[i][j-1]+x[i][j+1]+x[i-1][j]+x[i+1][j] - 4*x[i][j];
      // ("nabla" is the name of the "upside down delta" symbol used for the Laplacian in equations)
#     define NABLA_5PT(ctr,left,right,up,down) \
         v4SUB(v4ADD(v4ADD(v4ADD(left,right),up),down),v4MUL(ctr,v4SPLAT(4.0f)))

      // compute the new rate of change of u and v
      V4F4 v4_uvv = v4MUL(*v_ubase,v4MUL(*v_vbase,*v_vbase)); // u*v^2 is used twice

      /* Scalar code is:     du[i][j] = D_u * nabla_u - u*v^2 + F*(1-u);
         We treat it as:                D_u * nabla_u - (u*v^2 - F*(1-u)) */
      *v_dubase = v4SUB(v4MUL(v4_Du,
         NABLA_5PT(*v_ubase, u_left, u_right, *v_ub_prev, *v_ub_next)),
                           v4SUB(v4_uvv,v4MUL(v4_F,v4SUB(v4SPLAT(1.0f),*v_ubase))));

      /* dv formula is similar:  dv[i][j] = D_v * nabla_v + u*v^2 - (F+k)*v; */
      *v_dvbase = v4ADD(v4MUL(v4_Dv,
         NABLA_5PT(*v_vbase, v_left, v_right, *v_vb_prev, *v_vb_next)),
                           v4SUB(v4_uvv,v4MUL(v4ADD(v4_F,v4_k),*v_vbase)));
    }

  } // End of scan per row

  // First thread interlock goes here
  if (interlock) { DICEK_CH_SYNC }
  if (interlock) { DICEK_CH_BEGIN }

  {
    int right_b, left_b;
    if (g_wrap) {
      right_b = wid_v-2;
      left_b = 1;
    } else {
      right_b = 1;
      left_b = wid_v-2;
    }
  // effect change
    for(int i = start_row; i < end_row; i++) {
      for(int j = 1; j < wid_v-1; j++) {
        const V4F4 v4_speed = v4SPLAT(speed);
        V4F4 * v_ubase = ((V4F4 *)u)+i*wid_v+j;
        V4F4 * v_vbase = ((V4F4 *)v)+i*wid_v+j;
        V4F4 * v_dubase = ((V4F4 *)du)+i*wid_v+j;
        V4F4 * v_dvbase = ((V4F4 *)dv)+i*wid_v+j;
        // u[i][j] = u[i][j] + speed * du[i][j];
        *v_ubase = v4ADD(v4MUL(v4_speed, *v_dubase), *v_ubase);
        // v[i][j] = v[i][j] + speed * dv[i][j];
        *v_vbase = v4ADD(v4MUL(v4_speed, *v_dvbase), *v_vbase);
      }
      // Update cells on boundary from one row inland
      *(((V4F4 *)u)+i*wid_v) = *(((V4F4 *)u)+i*wid_v+right_b);
      *(((V4F4 *)v)+i*wid_v) = *(((V4F4 *)v)+i*wid_v+right_b);
      *(((V4F4 *)u)+i*wid_v+wid_v-1) = *(((V4F4 *)u)+i*wid_v+left_b);
      *(((V4F4 *)v)+i*wid_v+wid_v-1) = *(((V4F4 *)v)+i*wid_v+left_b);
    }
  }

  // second thread interlock goes here
  if (interlock) { DICEK_CH_SYNC }

  } // End of scan per iteration
}



#ifdef COMPUTE_A

/* The parameter space code, specifically the "if(parameter_space)" test itself, causes a 2.5% slowdown even when the
   parameter_space flag is false */
//#define SUPPORT_PARAM_SPACE

void * compute(void * gpb)
{
  DICEK_SUB(compute_params, gpb);

#if (defined(__i386__) || defined(__amd64__) || defined(__x86_64__) || 	defined(_M_X64) || defined(_M_IX86))
  /* On Intel we disable accurate handling of denorms and zeros. This is an
     important speed optimization. */
  int oldMXCSR = _mm_getcsr(); //read the old MXCSR setting
  int newMXCSR = oldMXCSR | 0x8040; // set DAZ and FZ bits
  _mm_setcsr( newMXCSR ); //write the new MXCSR setting to the MXCSR
#endif

  compute_params * param_block;
  param_block = (compute_params *) gpb;
  float *u = param_block->u;
  float *v = param_block->v;
  float *du = param_block->du;
  float *dv = param_block->dv;
  float D_u = param_block->D_u;
  float D_v = param_block->D_v;
  float F = param_block->F;
  float k = param_block->k;
  float speed = param_block->speed;
  int parameter_space = param_block->parameter_space;
  int num_its = param_block->num_its;
  long start_row = param_block->start_row;
  long end_row = param_block->end_row;
  int interlock = param_block->interlock_type;

  int iter;
#ifndef HWIV_HAVE_V4F4
  fprintf(stdout, "Did not get vector macros from HWIV\n");
  exit(-1);
#endif
  // Vector "constants": speed, F, k, D_u, D_v
  V4F4 v4_speed, v4_F, v4_k, v4_Du, v4_Dv;
  // Pointers used to load data from rows of the grid
  V4F4 *v_ub_prev, *v_ubase, *v_ub_next;
  V4F4 *v_vb_prev, *v_vbase, *v_vb_next;
  // Actual grid data
  V4F4 v4_u_l, v4_u, v4_u_r;
  V4F4 v4_v_l, v4_v, v4_v_r;
  V4F4 v4_uvv;
  // Pointers to second grid where we write the results of the main computation
  V4F4 *v_dubase, *v_dvbase;

#ifdef SUPPORT_PARAM_SPACE
  const float k_min=0.045f, k_max=0.07f, F_min=0.01f, F_max=0.09f;
  float k_diff;
  V4F4 v4_kdiff;
  k_diff = (k_max-k_min)/g_height;
  v4_kdiff = v4SET(0, -k_diff, -2*k_diff, -3*k_diff);
#endif

  // Initialize our vectorized scalars
  v4_speed = v4SPLAT(speed);
  v4_F = v4SPLAT(F);
  v4_k = v4SPLAT(k);
  v4_Du = v4SPLAT(D_u);
  v4_Dv = v4SPLAT(D_v);

  // Scan per iteration
  for(iter = 0; iter < num_its; iter++) {

  if (interlock) { DICEK_CH_BEGIN }

//printf("iter %d rows [%ld,%ld)\n",iter,start_row,end_row);

  // Scan per row
  for(long i = start_row; i < end_row; i++) {
    long iprev,inext;
    long v_j2;
    if (g_wrap) {
      /* Periodic boundary condition */
      iprev = (i+g_height-1) % g_height;
      inext = (i+1) % g_height;
    } else {
      /* The edges are their own neighbors. This amounts to a Neumann boundary condition. */
      iprev = max(i-1, 0);
      inext = min(i+1, g_height-1);
    }

    /* Get pointers to beginning of rows for each of the grids. We access
       3 rows each for u and v, and 1 row each for du and dv. */
    v_ubase = (V4F4 *)&INDEX(u,i,0);
    v_ub_prev = (V4F4 *)&INDEX(u,iprev,0);
    v_ub_next = (V4F4 *)&INDEX(u,inext,0);
    v_vbase = (V4F4 *)&INDEX(v,i,0);
    v_vb_prev = (V4F4 *)&INDEX(v,iprev,0);
    v_vb_next = (V4F4 *)&INDEX(v,inext,0);
    v_dubase = (V4F4 *)&INDEX(du,i,0);
    v_dvbase = (V4F4 *)&INDEX(dv,i,0);

#ifdef SUPPORT_PARAM_SPACE
    if (parameter_space) {
      // set F for this row (ignore the provided value)
      F = F_min + (g_height-i-1) * (F_max-F_min)/g_width;
      v4_F = v4SPLAT(F);
    }
#endif

    /* Pre-load the first two blocks of data we need, which are the "center"
       and "right" blocks from the end of the row (as if we have just wrapped
       around from the end of the row back to the beginning) */
    v_j2 = g_wrap ? ((g_width-4)/VECSIZE) : 0;

    v4_u = *(v_ubase+v_j2);
    v4_v = *(v_vbase+v_j2);
    v4_u_r = *v_ubase++;
    v4_v_r = *v_vbase++;

    // Scan per column in steps of vector width
    for(long j = 0; j < g_width-VECSIZE; j+=VECSIZE) {
      // Get a new 4 pixels from the current row and shift the other 8 pixels over
      v4_u_l = v4_u; v4_u = v4_u_r; v4_u_r = *v_ubase;
      v4_v_l = v4_v; v4_v = v4_v_r; v4_v_r = *v_vbase;

#ifdef SUPPORT_PARAM_SPACE
      if (parameter_space) {
        // set k for this column (ignore the provided value)
        k = k_min + (g_width-j-1)*k_diff;
        // k decreases by k_diff each time j increases by 1, so this vector
        // needs to contain 4 different k values. We use v4_kdiff, pre-computed
        // above, to accomplish this.
        v4_k = v4ADD(v4SPLAT(k),v4_kdiff);
      }
#endif

      // To compute the Laplacians of u and v, we use the 5-point neighbourhood for the Euler discrete method:
      //    nabla(x) = x[i][j-1]+x[i][j+1]+x[i-1][j]+x[i+1][j] - 4*x[i][j];
      // ("nabla" is the name of the "upside down delta" symbol used for the Laplacian in equations)
#     define NABLA_5PT(ctr,left,right,up,down) \
         v4SUB(v4ADD(v4ADD(v4ADD(left,right),up),down),v4MUL(ctr,v4SPLAT(4.0f)))

      // compute the new rate of change of u and v
      v4_uvv = v4MUL(v4_u,v4MUL(v4_v,v4_v)); // u*v^2 is used twice

      /* Scalar code is:     du[i][j] = D_u * nabla_u - u*v^2 + F*(1-u);
         We treat it as:                D_u * nabla_u - (u*v^2 - F*(1-u)) */
      *v_dubase = v4SUB(v4MUL(v4_Du,
         NABLA_5PT(v4_u, v4RAISE(v4_u,v4_u_l), v4LOWER(v4_u,v4_u_r), *v_ub_prev, *v_ub_next)),
                           v4SUB(v4_uvv,v4MUL(v4_F,v4SUB(v4SPLAT(1.0f),v4_u))));

      /* dv formula is similar:  dv[i][j] = D_v * nabla_v + u*v^2 - (F+k)*v; */
      *v_dvbase = v4ADD(v4MUL(v4_Dv,
         NABLA_5PT(v4_v, v4RAISE(v4_v,v4_v_l), v4LOWER(v4_v,v4_v_r), *v_vb_prev, *v_vb_next)),
                           v4SUB(v4_uvv,v4MUL(v4ADD(v4_F,v4_k),v4_v)));

      v_ub_prev++; v_ub_next++;
      v_ubase++;   v_vbase++;
      v_vb_prev++; v_vb_next++;
      v_dubase++;  v_dvbase++;
    }

    /* Now we do the last 4 pixels. This is unrolled out of the main loop just to avoid having to do the g_wrap
       test every time in the j loop. */
    v4_u_l = v4_u; v4_u = v4_u_r;
    v4_v_l = v4_v; v4_v = v4_v_r;
    if (g_wrap) {
      /* The 4 cells to the "right" are the first 4 in this row */
      v4_u_r = *((V4F4 *)&INDEX(u,i,0));
      v4_v_r = *((V4F4 *)&INDEX(v,i,0));
    } else {
      /* just leave them alone, retaining the rightmost 4 values in this row, which were loaded on the last iteration
         through the loop */
    }
    v4_uvv = v4MUL(v4_u,v4MUL(v4_v,v4_v));
    *v_dubase = v4SUB(v4MUL(v4_Du,
         NABLA_5PT(v4_u, v4RAISE(v4_u,v4_u_l), v4LOWER(v4_u,v4_u_r), *v_ub_prev, *v_ub_next)),
                           v4SUB(v4_uvv,v4MUL(v4_F,v4SUB(v4SPLAT(1.0f),v4_u))));
    *v_dvbase = v4ADD(v4MUL(v4_Dv,
         NABLA_5PT(v4_v, v4RAISE(v4_v,v4_v_l), v4LOWER(v4_v,v4_v_r), *v_vb_prev, *v_vb_next)),
                           v4SUB(v4_uvv,v4MUL(v4ADD(v4_F,v4_k),v4_v)));
  } // End of scan per row

  // First thread interlock goes here
  if (interlock) { DICEK_CH_SYNC }
  if (interlock) { DICEK_CH_BEGIN }

  {
  // effect change
    for(long i = start_row; i < end_row; i++) {
      v_ubase = ((V4F4 *) (&INDEX(u,i,0)));
      v_vbase = ((V4F4 *) (&INDEX(v,i,0)));
      v_dubase = ((V4F4 *) (&INDEX(du,i,0)));
      v_dvbase = ((V4F4 *) (&INDEX(dv,i,0)));
      for(long j = 0; j < g_width; j+=VECSIZE) {
        // u[i][j] = u[i][j] + speed * du[i][j];
        *v_ubase = v4ADD(v4MUL(v4_speed, *v_dubase), *v_ubase); v_ubase++; v_dubase++;
        // v[i][j] = v[i][j] + speed * dv[i][j];
        *v_vbase = v4ADD(v4MUL(v4_speed, *v_dvbase), *v_vbase); v_vbase++; v_dvbase++;
      }
    }
  }

  // second thread interlock goes here
  if (interlock) { DICEK_CH_SYNC }

  } // End of scan per iteration
}
#endif

#ifdef COMPUTE_PRE_THREADS

/* This is the old version of compute() that used all "assembly-language" syntax */

void compute(float *u, float *v, float *du, float *dv,
             float D_u,float D_v,float F,float k,float speed,
             int parameter_space)
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
  HWIV_INIT_MUL0_4F4; // used by MUL (on targets that need it)
  HWIV_INIT_MTMP_4F4; // used by MADD (on targets that need it)
  HWIV_INIT_FILL;     // used by FILL
  HWIV_INIT_RLTMP_4F4; // used by RAISE and LOWER
  V4F4 v4_tmp;
  V4F4 v4_Du;
  V4F4 v4_Dv;
  V4F4 v4_nabla_u;
  V4F4 v4_nabla_v;
  V4F4 v4_1;
  V4F4 v4_4;
  const float k_min=0.045f, k_max=0.07f, F_min=0.01f, F_max=0.09f;
  float k_diff;
  V4F4 v4_kdiff;
  float * ubase;
  float * ub_prev;
  float * ub_next;
  float * vbase; float * vb_prev; float * vb_next;
  float * dubase; float * dvbase;

  V4F4 v4_u_l;
  V4F4 v4_u_r;
  V4F4 v4_v_l;
  V4F4 v4_v_r;

  //F_diff = (F_max-F_min)/g_width;
  k_diff = (k_max-k_min)/g_height;

  // Initialize our vectorized scalars
  HWIV_SPLAT_4F4(v4_speed, speed);
  HWIV_SPLAT_4F4(v4_F, F);
  HWIV_SPLAT_4F4(v4_k, k);
  HWIV_SPLAT_4F4(v4_Du, D_u);
  HWIV_SPLAT_4F4(v4_Dv, D_v);
  HWIV_SPLAT_4F4(v4_1, 1.0);
  HWIV_SPLAT_4F4(v4_4, 4.0);
  HWIV_FILL_4F4(v4_kdiff, 0, -k_diff, -2*k_diff, -3*k_diff);

  // Scan per row
  for(int i = 0; i < g_height; i++) {
    int iprev,inext;
    int j2;

    if (g_wrap) {
      iprev = (i+g_height-1) % g_height;
      inext = (i+1) % g_height;
    } else {
      iprev = max(i-1, 0);
      inext = min(i+1, g_height-1);
    }
    /* Get pointers to beginning of rows for each of the grids. We access
       3 rows each for u and v, and 1 row each for du and dv. */
    ubase = &INDEX(u,i,0);
    ub_prev = &INDEX(u,iprev,0);
    ub_next = &INDEX(u,inext,0);
    vbase = &INDEX(v,i,0);
    vb_prev = &INDEX(v,iprev,0);
    vb_next = &INDEX(v,inext,0);
    dubase = &INDEX(du,i,0);
    dvbase = &INDEX(dv,i,0);

    if (parameter_space) {
      // set F for this row (ignore the provided value)
      F = F_min + (g_height-i-1) * (F_max-F_min)/g_width;
      HWIV_SPLAT_4F4(v4_F, F);
    }

    /* Pre-load the first two blocks of data we need, which are the "center"
       and "right" blocks from the end of the row (as if we have just wrapped
       around from the end of the row back to the beginning) */
    j2 = g_wrap ? (g_width-4) : 0;
    HWIV_LOAD_4F4(v4_u, ubase+j2);
    HWIV_LOAD_4F4(v4_u_r, ubase);
    HWIV_LOAD_4F4(v4_v, vbase+j2);
    HWIV_LOAD_4F4(v4_v_r, vbase);

    // Scan per column in steps of vector width
    for(int j = 0; j < g_width; j+=4) {
      if (g_wrap) {
        j2 = (j+4) % g_width;
      } else {
        j2 = min(j+4, g_height-4);
      }

      HWIV_COPY_4F4(v4_u_l, v4_u);
      HWIV_COPY_4F4(v4_v_l, v4_v);
      HWIV_COPY_4F4(v4_u, v4_u_r);
      HWIV_COPY_4F4(v4_v, v4_v_r);
      HWIV_LOAD_4F4(v4_u_r, ubase+j2);
      HWIV_LOAD_4F4(v4_v_r, vbase+j2);

      if (parameter_space) {
        // set k for this column (ignore the provided value)
        k = k_min + (g_width-j-1)*k_diff;
        // k decreases by k_diff each time j increases by 1, so this vector
        // needs to contain 4 different k values.
        HWIV_SPLAT_4F4(v4_tmp, k);
        HWIV_ADD_4F4(v4_k, v4_tmp, v4_kdiff);
      }

      // compute the Laplacians of u and v. "nabla" is the name of the
      // "upside down delta" symbol used for the Laplacian in equations

      /* Scalar code is:
         nabla_u = u[i][jprev]+u[i][jnext]+u[iprev][j]+u[inext][j] - 4*uval; */
      HWIV_RAISE_4F4(v4_nabla_u, v4_u, v4_u_l);

      HWIV_LOWER_4F4(v4_tmp, v4_u, v4_u_r);
      HWIV_ADD_4F4(v4_nabla_u, v4_nabla_u, v4_tmp);

      // Now we add in the "up" and "down" neighbors
      HWIV_LOAD_4F4(v4_tmp, ub_prev+j);
      HWIV_ADD_4F4(v4_nabla_u, v4_nabla_u, v4_tmp);
      HWIV_LOAD_4F4(v4_tmp, ub_next+j);
      HWIV_ADD_4F4(v4_nabla_u, v4_nabla_u, v4_tmp);

      // Now we compute -(4*u-neighbors)  = neighbors - 4*u
      HWIV_NMSUB_4F4(v4_nabla_u, v4_4, v4_u, v4_nabla_u);

      // Same thing all over again for the v's
      HWIV_RAISE_4F4(v4_nabla_v, v4_v, v4_v_l);
      HWIV_LOWER_4F4(v4_tmp, v4_v, v4_v_r);
      HWIV_ADD_4F4(v4_nabla_v, v4_nabla_v, v4_tmp);
      HWIV_LOAD_4F4(v4_tmp, vb_prev+j);
      HWIV_ADD_4F4(v4_nabla_v, v4_nabla_v, v4_tmp);
      HWIV_LOAD_4F4(v4_tmp, vb_next+j);
      HWIV_ADD_4F4(v4_nabla_v, v4_nabla_v, v4_tmp);
      HWIV_NMSUB_4F4(v4_nabla_v, v4_4, v4_v, v4_nabla_v);

      // compute the new rate of change of u and v

      /* Scalar code is:
           du[i][j] = D_u * nabla_u - uval*vval*vval + F*(1-uval);
         We treat it as:
                      D_u * nabla_u - (uval*vval*vval - (-(F*uval-F)) ) */

      HWIV_NMSUB_4F4(v4_tmp, v4_F, v4_u, v4_F);        // -(F*u-F) = F-F*u = F(1-u)
      HWIV_MUL_4F4(v4_dv, v4_v, v4_v);                 // v^2
      HWIV_MSUB_4F4(v4_tmp, v4_u, v4_dv, v4_tmp);      // u*v^2 - F(1-u)
      HWIV_MSUB_4F4(v4_du, v4_Du, v4_nabla_u, v4_tmp); // D_u*nabla_u - (u*v^2 - F(1-u))
                                                       // = D_u*nabla_u - u*v^2 + F(1-u)
      HWIV_SAVE_4F4(dubase+j, v4_du);

      /* dv formula is similar:
           dv[i][j] = D_v * nabla_v + uval*vval*vval - (F+k)*vval;
         We treat it as:
                      D_v * nabla_v + uval*vval*vval - (F*vval + k*vval); */
      HWIV_MUL_4F4(v4_tmp, v4_k, v4_v);                // k*v
      HWIV_MADD_4F4(v4_tmp, v4_F, v4_v, v4_tmp);       // F*v+k*v = (F+k)v
                                                       // v^2 is still in v4_dv
      HWIV_MSUB_4F4(v4_tmp, v4_u, v4_dv, v4_tmp);      // u*v^2 - (F+k)v
      HWIV_MADD_4F4(v4_dv, v4_Dv, v4_nabla_v, v4_tmp); // D_v*nabla_v + u*v^2 - (F+k)v
      HWIV_SAVE_4F4(dvbase+j, v4_dv);
    }
  }

  // effect change
    for(int i = 0; i < g_height; i++) {
      ubase = &INDEX(u,i,0);
      vbase = &INDEX(v,i,0);
      dubase = &INDEX(du,i,0);
      dvbase = &INDEX(dv,i,0);
      for(int j = 0; j < g_width; j+=4) {
        // u[i][j] = u[i][j] + speed * du[i][j];
        HWIV_LOAD_4F4(v4_u, ubase+j);               // get u
        HWIV_LOAD_4F4(v4_du, dubase+j);             // get du
        HWIV_MADD_4F4(v4_u, v4_speed, v4_du, v4_u); // speed*du + u
        HWIV_SAVE_4F4(ubase+j, v4_u);               // write it back

        // v[i][j] = v[i][j] + speed * dv[i][j];
        HWIV_LOAD_4F4(v4_v, vbase+j);
        HWIV_LOAD_4F4(v4_dv, dvbase+j);
        HWIV_MADD_4F4(v4_v, v4_speed, v4_dv, v4_v);
        HWIV_SAVE_4F4(vbase+j, v4_v);
      }
    }
}

#endif
