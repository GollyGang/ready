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
#define INDEX(a,x,y) ((a)[(x)*g_width+(y)])

float *allocate(long width, long height, const char * error_text);
float *allocate(long width, long height, const char * error_text)
{
  size_t sz;
  float * rv;
  // we add 32 bytes: 16 so we can align it here, and another 16 so the v_j2
  // index can go one past the end of the last row
  sz = ((size_t) width) * ((size_t) height) * sizeof(*rv) + 32;
  rv = (float *) malloc(sz);
  while(((long)rv) & 0x0000000F) { rv++; }
  if (rv == NULL) {
    fprintf(stderr, "allocate: Could get %ld bytes for %s\n", ((long) sz),
      error_text);
    exit(-1);
  }
  return (rv);
}

void init(float *a, float *b, long width, long height,
          int density, int lowback, int BZrects);

#define MAX_LOADS 100
char * load_opts[MAX_LOADS];
int num_loads = 0;

void load_option(float * u, float * v, long width, long height, const char * option);
void pattern_load(float * u, float * v, long width, long height,
  const char * pattern_filename, long x, long y, int orient);
long next_patsize(long former);
long next_A029744(long former);
void byteswap_2_double(double * array);

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

void colorize(float *u, float *v, float *du,
             float *red, float *green, float *blue);

static int g_color = 0;
static int g_paramspace = 0;
static int g_wrap = 0;
static float g_k, g_F;
static int g_ramprects = 0;
static int g_lowback = 0;
static float g_scale = 1.0;
static int g_density = 0;
static bool g_video = false;

#include <unistd.h>

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

  bool custom_Fk = false;

  for (int i = 1; i < argc; i++) {
    if (0) {
    } else if (strcmp(argv[i],"-color")==0) {
      // do output in wonderful technicolor
      g_color = 1;
    } else if ((i+1<argc) && (strcmp(argv[i],"-density")==0)) {
      // set density of spots for initial pattern
      i++; g_density = atoi(argv[i]);
    } else if ((i+1<argc) && 
         ((strcmp(argv[i],"-F")==0) || (strcmp(argv[i],"-f")==0)) ) {
      // set parameter F
      i++; g_F = atof(argv[i]);
      custom_Fk = true;
    } else if ((i+1<argc) && (strcmp(argv[i],"-height")==0)) {
      // set height
      i++; g_height = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-k")==0)) {
      // set parameter k
      i++; g_k = atof(argv[i]);
      custom_Fk = true;
    } else if (strcmp(argv[i],"-paramspace")==0) {
      // do a parameter space plot, like in the Pearson paper
      g_paramspace = 1;
    } else if (strcmp(argv[i],"-ramprects")==0) {
      // use ramped rectangles in the initial pattern
      g_ramprects = 1;
    } else if ((i+1<argc) && (strcmp(argv[i],"-load")==0)) {
      // load a pattern
      i++; if (num_loads < MAX_LOADS) {
        load_opts[num_loads++] = argv[i];
      }
    } else if (strcmp(argv[i],"-lowback")==0) {
      // use "low" background value in init
      g_lowback = 1;
    } else if ((i+1<argc) && (strcmp(argv[i],"-scale")==0)) {
      // set scale
      i++; g_scale = atof(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-size")==0)) {
      // set both width and height
      i++; g_height = g_width = atoi(argv[i]);
    } else if ((i+1<argc) && (strcmp(argv[i],"-threads")==0)) {
      // specify number of threads to use
      i++; g_threads = atoi(argv[i]);
    } else if (strcmp(argv[i],"-video")==0) {
      // Create a video media file
      g_video = true;
    } else if ((i+1<argc) && (strcmp(argv[i],"-width")==0)) {
      // set width
      i++; g_width = atoi(argv[i]);
    } else if (strcmp(argv[i],"-wrap")==0) {
      // patterns wrap around ("torus", also called "continuous boundary
      // condition")
      g_wrap = 1;
    } else {
      fprintf(stderr, "Unrecognized argument: '%s'\n", argv[i]);
      exit(-1);
    }
  }

  if ((g_scale == 1.0) && (custom_Fk)) {
    g_scale = 2.0;
  }
  D_u = D_u * g_scale;
  D_v = D_v * g_scale;
  speed = speed / g_scale;

  if (g_width%4) {
    g_width = ((g_width/4)+1)*4;
  }

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

  u = allocate(g_width, g_height, "U array");
  v = allocate(g_width, g_height, "V array");
  du = allocate(g_width, g_height, "D_u array");
  dv = allocate(g_width, g_height, "D_v array");
  red = allocate(g_width, g_height, "red array");
  green = allocate(g_width, g_height, "green array");
  blue = allocate(g_width, g_height, "blue array");

  // put the initial conditions into each cell
  init(u,v, g_width, g_height, g_density, g_lowback, g_ramprects);

  int N_FRAMES_PER_DISPLAY;

  if (g_video) {
    N_FRAMES_PER_DISPLAY = 500;
  } else {
#   ifdef HWIV_EMULATE
      N_FRAMES_PER_DISPLAY = 200;
#   else
      N_FRAMES_PER_DISPLAY = 1000;
#   endif
  }

  int iteration = 0;
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
    iteration+=N_FRAMES_PER_DISPLAY;

    if (g_color) {
      colorize(u, v, du, red, green, blue);
    }

    gettimeofday(&tod_record, 0);
    tod_after = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    tod_elapsed = tod_after - tod_before;
    fps = ((double)N_FRAMES_PER_DISPLAY) / tod_elapsed;
    // We display an exponential moving average of the fps measurement
    fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0);
    Mcgs = fps_avg * ((double) g_width) * ((double) g_height) / 1.0e6;

    char msg[1000];
    sprintf(msg,"GrayScott - %0.2f fps (%.2f Mcgs)", fps_avg, Mcgs);

    // display:
    {
      int chose_quit;
      if (g_color) {
        chose_quit = display(g_width,g_height,red,green,blue,iteration,false,200.0f,2,10,msg,g_video);
      } else {
        chose_quit = display(g_width,g_height,u,u,u,iteration,false,200.0f,2,10,msg,g_video);
      }
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
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

// return a random value between lower and upper
float frand(float lower,float upper)
{
  float rv;

  rv = lower + rand()*(upper-lower)/RAND_MAX;
  rv = max(0.0, min(1.0, rv));
  return rv;
}

/* pearson_bkg fills everything with the trivial state (U=1, V=0) combined
   with random noise of magnitude 0.01, and it does this while keeping all
   U and V values between 0 and 1. */
void pearson_bkg(float *u, float *v)
{
  int i, j;
  for(i=0; i<g_height; i++) {
    for(j=0; j<g_width; j++) {
      INDEX(u,i,j) = frand(0.99, 1.0);
      INDEX(v,i,j) = frand(0.0, 0.01);
    }
  }
}

/* pearson_block creates a block filled with a given U and V value combined
   with superimposed noise of amplitude 0.01 */
void pearson_block(float *u, float *v,
                 int vpos, int hpos, int h, int w, float U, float V)
{
  int i, j;

  for(i=vpos; i<vpos+h; i++) {
    if ((i >= 0) && (i < g_height)) {
      for(j=hpos; j<hpos+w; j++) {
        if ((j >= 0) && (j < g_width)) {
          INDEX(u,i,j) = frand(U-0.005, U+0.005);
          INDEX(v,i,j) = frand(V-0.005, V+0.005);
        }
      }
    }
  }
}

/* ramp_block creates a starting pattern for B-Z spirals and continuous
   propagating wave fronts (pattern type xi in my paper). */
void ramp_block(float *u, float *v,
                int vpos, int hpos, int h, int w, int fliph)
{
  int i, j;
  float U, V;

  for(i=vpos; i<vpos+h; i++) {
    if ((i >= 0) && (i < g_height)) {
      for(j=hpos; j<hpos+w; j++) {
        if ((j >= 0) && (j < g_width)) {
          U = ((float) (j-hpos)) / ((float) w);
          if (fliph) {
            U = 1.0 - U;
          }
          V = U;

          if (U < 0.1) {
            U = U / 0.1;
          } else {
            U = (1.0 - U) / 0.9;
          }
          U = 1.0 - sin((1.0 - U) * 1.5708);
          U = 1.0 - (U * 0.95); // formerly 0.85
          
          if (V < 0.08) { // formerly 0.05
            V = V / 0.08; // formerly 0.05
          } else if (V < 0.5) {
            V = (0.5 - V) / 0.42; // formerly 0.45
          } else {
            V = 0;
          }
          V = 1.0 - sin((1.0 - V) * 1.5708);
          V = V * 0.4;

          INDEX(u,i,j) = frand(U-0.005, U+0.005);
          INDEX(v,i,j) = frand(V-0.005, V+0.005);
        }
      }
    }
  }
}

/* Given an F and k, returns the U and V values for the homogeneous state at
 that F and k. Tries to return the secondary (blue) state, but if that
 doesn't exist it returns the trivial (red) state.
   The formula is derived from Muratov and Osipov 2000 formula 2.12 (with A
 defined by 2.10, and the other variables defined as in 2.3, 2.4 and 2.5 (all
 of this is on pages 8-9 of "Muratov 2000 Spike.pdf").
   There is a more obvious formula (expressed in terms of F and k) in
 "Leppanen 2004 Computational.pdf" page 40 (his equation 3.22).
   To get the values for the center F and k settings, use (g_F_CTR, g_k_CTR) */
void homogen_uv(float F, float k, float * hU, float * hV)
{
  float sqrt_F, A;
  float U, V;

  sqrt_F = sqrt(F);

  if (k < (sqrt_F - 2.0 * F) / 2.0) {
    A = sqrt_F / (F + k);
    U = (A - sqrt(A*A - 4.0)) / (2.0 * A);
    U = max(0.0, min(1.0, U));
    V = sqrt_F * (A + sqrt(A*A - 4.0)) / 2.0;
    V = max(0.0, min(1.0, V));
  } else {
    U = 1.0;
    V = 0.0;
  }
  *hU = U;
  *hV = V;
}

/* i5_bkg is the background for my init routines. It fills the space either
   with the trivial stable state (V=0, U=1), or when the other stable "blue"
   state exists it uses that state. This makes for much more interesting
   initial patterns for the areas near the various bifurcation lines and for
   Uskate world. */
void i5_bkg(float *u, float *v, int which)
{
  int i, j;

  for(i=0; i<g_height; i++) {
    for(j=0; j<g_width; j++) {
      float U, V;

      if (which == 0) {
        U = 1.0; V = 0.0;
      } else {
        homogen_uv(g_F, g_k, &U, &V);
      }
      // printf("F %g  k %g  A %g  U %g  V %g\n", F, k, A, U, V);
      INDEX(u,i,j) = U;
      INDEX(v,i,j) = V;
    }
  }
}

/*
 Execute one of the "-load" command-line options. This requires parsing out an X and Y coordinate, an option
orientation, and a filename or pathname. It calls pattern_load to actually read the pattern into the grid.
 */
void load_option(float * u, float * v, long width, long height, const char * option)
{
  long x;
  long y;
  int orient = 0;
  char * p1;

  p1 = (char *) option;
  x = atol(p1);
  while((*p1 >= '0') && (*p1 <= '9')) { p1++; }

  /* Now we expect to see "," */
  if(!(*p1 == ',')) {
    /* We got to the end of the string, or the X coordinate was followed by something other than a comma */
    fprintf(stdout, "invalid load format: '%s'\n", option);
    exit(-1);
  }
  p1++;
  y = atol(p1);
  while((*p1 >= '0') && (*p1 <= '9')) { p1++; }

  /* Now we can see "o" or ":" */
  if (*p1 == 'o') {
    /* get orientation */
    p1++;
    orient = atoi(p1);
    while((*p1 >= '0') && (*p1 <= '9')) { p1++; }
  }
  if(!(*p1 == ':')) {
    /* We got to the end of the string, or the X and Y were not followed by ":" */
    fprintf(stdout, "invalid load format: '%s'\n", option);
    exit(-1);
  }
  p1++;
  if (*p1) {
    pattern_load(u, v, g_width, g_height, p1, x, y, orient);
  }
}


#define MIN_CLIPSIZE 4
#define MAX_CLIPSIZE 256

#if (defined(__APPLE__) || defined(__linux__))
# define FILE_LENGTH(name, result) \
    struct stat _FILESTAT; int _STAT_result; \
    _FILESTAT.st_size = 0; \
    _STAT_result = stat(name, &_FILESTAT); \
    *(result) = (_STAT_result < 0) ? 0 : _FILESTAT.st_size;
#else
# ifdef _WIN32
// TODO: This needs to be tested
#   define FILE_LENGTH(name, result) \
    ULONGLONG _LEN_res_tmp; \
    WIN32_FIND_DATA _LEN_fff_dat = { 0 }; \
    HANDLE _LEN_fff_hdl = FindFirstFile(name, &_LEN_fff_dat); \
    if (_LEN_fff_hdl != INVALID_HANDLE_VALUE) { \
      FindClose(_LEN_fff_lhd); \
      _LEN_res_tmp = (_LEN_fff_dat.nFileSizeHigh) << (sizeof(_LEN_fff_dat.nFileSizeHigh)*8) | (_LEN_fff_dat.nFileSizeLow); \
      *(result) = _LEN_res_tmp; \
    } else { *(result) = 0; }
# else
// Neither Unix nor Windows
#   define FILE_LENGTH(name, result) \
    sprintf(stdout, "Getting length of file is not yet supported in this environment.\n"); exit(-1);
# endif
#endif

/*
   Load a PDE4 pattern file with the given filename (pathname) into the U and V arrays, at the given X and Y location.

                        6   7          Starting from orientation 0: orient. 5 is rotated 90 degrees clockwise;
  Orientation             ^            orient. 3 is rotated 180 degrees, and orient. 6 is rotated another 90 degrees
   diagram:               |            clockwise.
                   2      |      0       Orientation 1 is a mirror image of orientation 0, flipped around a horizontal
                    <-----o----->      axis. Compared to orientation 1, orient. 4 is rotated 90 degrees clockwise;
                   3      |      1     orient. 2 is rotated 180 degrees, and orient. 7 is rotated another 90 degrees
                          |            clockwise.
                          v
                        4   5

You'll notice an odd scaling factor of "0.633". Most of the patterns in my collection were created in my PDE4 program
using what I call the "standard model" parameters. PDE4 converts intervals of time (frames) and space (pixels) into the
dimensionless time and space units in the actual Pearson equations, and is adjusted to use a grid that has about sqrt(2)
finer resolution than the grid Pearson used in his 1993 paper.
  Here are the relevant variables and the u formula as calculated by both programs:

  PDE4:                                                            HWIV:
    g_SPATIAL_REZ = 0.007                                            g_scale = 2.0
    dxy = (1/143)^2 = 20449                                          speed = 0.5
    delta_t = 0.05                                                   D_u = 0.164
    D_u = 2.0e-5                                                     D_v = 0.082
    D_v = 1.0e-5
    u -> u + delta_t * (D_u * nabla_u/dxy^2 - u v^2 + F(1-u))        u -> u + speed * (D_u * nabla_u - u v^2 + F(1-u))
       = u + 0.5 * (2e-5 * dxy * nabla_u - u v^2 + F(1-u))              = u + 0.5 * (0.164 * nabla_u - u v^2 + F(1-u))
       = u + 0.5 * (0.409 * nabla_u - u v^2 + F(1-u))
    (v equation is similar)                                          (v equation is similar)

After all the constants are combined, the result is that HWIV differs from PDE4 only in using a different effective D_u
value, namely 0.164 as compared to 0.409. This causes a difference in scale of all simulated patterns, the scale
difference is sqrt(0.164/0.409) = 0.633.

*/

void pattern_load(float * u, float * v, long width, long height,
  const char * pattern_filename, long x, long y, int orient)
{
  long i, j;
  FILE * patfile;
  long fsize, csz;
  double data[2];
  char * test_endian;
  int big_endian;

  data[0] = 1.0f;
  test_endian = (char *) data;
  // printf("endian test bytes: %02X .. %02X\n", test_endian[0], test_endian[7]);
  if (test_endian[0] == 0x3F) {
    // The first byte in memory is part of the exponent. This means we're on a big-endian machine.
    big_endian = 1;
  } else {
    big_endian = 0;
  }

  FILE_LENGTH(pattern_filename, &fsize);

  if (fsize <= 0) {
    fprintf(stderr, "Could not find pattern file '%s'\n", pattern_filename);
    exit(-1);
  }

  /* PDE4 pattern files are a square grid of pixels, each pixel is a U followed by a V, all values are IEEE
  /* double-precision floating-point. The size of the square is a power of 2 or a power of 2 times 1.5, ranging from 4
  /* to 256. */
  csz = MIN_CLIPSIZE;
  while((csz < MAX_CLIPSIZE) && (csz * csz * 2L * sizeof(double) < fsize)) {
    csz = next_patsize(csz);
  }

  if (csz * csz * 2L * sizeof(double) == fsize) {
    long i2, j2, i3, j3, prev_i2, prev_j2;
    double u_avg, v_avg, perim_count;
    long min_i = height; long max_i = 0;
    long min_j = width; long max_j = 0;
    prev_i2 = prev_j2 = -1;

    // Size matched exactly
    // printf("Clip file is %ld bytes long, size %ldx%ld.\n", fsize, csz, csz);
    printf("Placing pattern %s at position (%ld,%ld) orientation %d\n", pattern_filename, x, y, orient);
    patfile = fopen(pattern_filename, "r");

    u_avg = v_avg = perim_count = 0.0;
    for(i = 0; i < csz; i++) { // i is row number, Y dimension
      // i2 = (i - csz/2L)*63L/100L;
      i2 = i - csz/2L;
      i2 = (long) (0.633 * ((float) i2));
      for(j = 0; j < csz; j++) { // j is column number, X dimension
        float u_val, v_val;
        // j2 = (j - csz/2L)*63L/100L;
        j2 = j - csz/2L;
        j2 = (long) (0.633 * ((float) j2));

        // Copy to i3, j3 because we're going to change the values
        i3 = i2; j3 = j2;

        // Now apply orientation
        if (orient & 1) { i3 = - i3; }
        if (orient & 2) { j3 = - j3; }
        if (orient & 4) { int t = i3; i3 = j3; j3 = t; }

        i3 = y + i3;
        j3 = (width-1) - (x + j3);

        fread((void *) data, sizeof(double), 2, patfile);
        if (big_endian) {
          byteswap_2_double(data);
        }

        /* Add this point to the perimeter average */
        if ((i == 0) || (i == csz-1) || (j == 0) || (j == csz-1)) {
          u_avg += data[0]; v_avg += data[1]; perim_count += 1.0;
        }

        /* Because we are ADDING the pattern to the existing grid values, rather than simply replacing grid values,
           we need to modify each pixel at most once. */
        if ((i2 != prev_i2) && (j2 != prev_j2)) {
          /* Next we clip to the actual grid dimensions */
          if ((i3 >= 0) && (i3 < height) && (j3 >= 0) && (j3 < width)) {
            /* Keep track of the min and max values of both coordinates (used below) */
            if (i3 < min_i) { min_i = i3; } if (i3 > max_i) { max_i = i3; }
            if (j3 < min_j) { min_j = j3; } if (j3 > max_j) { max_j = j3; }
            u_val = (float) data[0]; v_val = (float) data[1];
            u[i3 * width + j3] += u_val;
            v[i3 * width + j3] += v_val;
          }
        }
        prev_j2 = j2;
      }
      prev_i2 = i2;
    }
    fclose(patfile);

    /* Now we subtract the average perimeter value from all the pixels that were adjusted. This allows the user to place
       two patterns very close to one another, even if the "bounding rectangles" would normally cause part of the first
       pattern to get wiped out by the second.
         Note that the global limit to the range [0,1] is enforced in the main init() routine after all patterns are
       loaded. */
    u_avg = u_avg / perim_count;
    v_avg = v_avg / perim_count;
    for(i=min_i; i<=max_i; i++) {
      for(j=min_j; j<=max_j; j++) {
        u[i * width + j] -= u_avg;
        v[i * width + j] -= v_avg;
      }
    }
  } else {
    printf("Error: %s size %ld is not canonical.\n", pattern_filename, fsize);
  }
}

long next_patsize(long former)
{
  int newval;

  if ((former & (former-1)) == 0) {
    // it was a power of 2
    newval = (former * 3) / 2;
  } else {
    newval = (former * 4) / 3;
  }
  if (newval > MAX_CLIPSIZE) {
    newval = MAX_CLIPSIZE;
  }
  return(newval);
}

/* Find the next number in sequence A029744, which consists of the powers of 2 and powers of 2 times 3. */
long next_A029744(long former)
{
  long newval;

  if (former <= 0) {
    return(1);
  }
  /* NOTE: We assume 2's complement notation, in which case (x & (x-1)) is zero iff x is a power of 2 */
  if ((former & (former-1)) == 0) {
    // it was a power of 2
    newval = former + (former >> 1);
  } else {
    // Anything else: first it round down to a power of 2
    newval = former;
    newval = newval | (newval >> 1);
    newval = newval | (newval >> 2);
    newval = newval | (newval >> 4);
    newval = newval | (newval >> 8);
    newval = newval | (newval >> 16);
    newval = newval | (newval >> 32);
    newval++;
    // Now double it.
    newval = newval * 2;
  }
  return(newval);
}

void byteswap_2_double(double * array)
{
  char * bytes;
  char t;
  bytes = (char *) array;
  int i;

  /* We have two doubles to swap */
  for(i=0; i<2; i++) {
    t = bytes[8*i+0]; bytes[8*i+0] = bytes[8*i+7]; bytes[8*i+7] = t;
    t = bytes[8*i+1]; bytes[8*i+1] = bytes[8*i+6]; bytes[8*i+6] = t;
    t = bytes[8*i+2]; bytes[8*i+2] = bytes[8*i+5]; bytes[8*i+5] = t;
    t = bytes[8*i+3]; bytes[8*i+3] = bytes[8*i+4]; bytes[8*i+4] = t;
  }
}

void init(float *u, float *v, long width, long height,
          int density, int lowback, int BZrects)
{
  long nsp, i;
  long base, var;

  srand((unsigned int)time(NULL));

  if (density <= 0) {
    nsp = 0;
  } else {
    if (density == 3) {
      base = (height * width) / 512;
      var = 2;
    } else {
      base = density * 20;
      var = density * (height * width) / 1000;
    }

    nsp = base + (rand() % var);
    printf("Adding %ld random rectangles\n", nsp);
  }

  i5_bkg(u, v, lowback ? 0 : 1);

  for(i=0; i<nsp; i++) {
    int v1, vs, h1, hs;
    float U, V;

    vs = 10 + (rand() % 12) * (rand() % 12) / 5;
    hs = 10 + (rand() % 12) * (rand() % 12) / 5;

    v1 = (rand() % height) - (vs / 2);
    h1 = (rand() % width) - (hs / 2);

    U = frand(0.0, 1.0);
    if (BZrects) {
      V = frand(0.0, 1.0 - U);
    } else {
      V = frand(0.0, 1.0);
    }
    if (BZrects) {
      if ((rand() & 0x3) == 0) {
        int h;
        h = 40 + (rand() & 31); // formerly 20 + ...
        ramp_block(u, v, v1, h1, h, 100, rand() & 1); // formerly 60
      } else {
        pearson_block(u, v, v1, h1, vs, hs, U, V);
      }
    } else {
      pearson_block(u, v, v1, h1, vs, hs, U, V);
    }
  }
    
  if ((num_loads <= 0) && (nsp <= 0)) {
  // old init pattern
  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      if(hypot(i-height/2,(j-width/2)/1.5)<=frand(2,5)) // start with a uniform field with an approximate circle in the middle
      {
        INDEX(u,i,j) = frand(0.0,0.1);
        INDEX(v,i,j) = frand(0.9,1.0);
      }
    }
  }
  }

  for(int i=0; i<num_loads; i++) {
    load_option(u, v, width, height, load_opts[i]);
  }

  /* Finally, enforce the limits for Gray-Scott, which are that U and V must be in the range [0.0,1.0]. */
  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      INDEX(u,i,j) = max(0.0, min(INDEX(u,i,j), 1.0));
      INDEX(v,i,j) = max(0.0, min(INDEX(v,i,j), 1.0));
    }
  }
}

#ifdef HWIV_V4F4_SSE2

#define VECSIZE 4

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

void colorize(float *u, float *v, float *du,
             float *red, float *green, float *blue)
{
  // Step by row
  for(long i = 0; i < g_height; i++) {
    // step by column
    for(long j = 0; j < g_width; j++) {
      float uval = INDEX(u,i,j);
      float vval = INDEX(v,i,j);
      float delta_u = (INDEX(du,i,j) * 1000.0f) + 0.5f;
      delta_u = ((delta_u < 0) ? 0.0 : (delta_u > 1.0) ? 1.0 : delta_u);

      // Something simple to start (-:
      // different colour schemes result if you reorder these, or replace
      // "x" with "1.0f-x" for any of the 3 variables
      INDEX(red,i,j) = delta_u; // increasing U will look pink
      INDEX(green,i,j) = 1.0-uval;
      INDEX(blue,i,j) = 1.0-vval;
    }
  }
}

#ifdef HWIV_EMULATE

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

/*

GrayScott_HWIVector/GrayScott_HWIVector -color -threads 2 -F 0.062 -k 0.0609 -wrap -load '128,128:patterns/interactions/U-curve-15-c'

GrayScott_HWIVector/GrayScott_HWIVector -color -threads 2 -F 0.062 -k 0.0609 -wrap -load '30,30:patterns/uskates/std' -load '80,140o5:patterns/halftargets/ht-4-10-3'

*/
