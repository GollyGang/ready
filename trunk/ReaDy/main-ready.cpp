/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See ../README.txt for more details.

*/

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

#include "ready_display.h"

#include "gray_scott_scalar.h"
#include "gray_scott_hwivector.h"

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

float frand(float lower,float upper);
void pearson_block(float *u, float *v, long width, long height,
                 int vpos, int hpos, int h, int w, float U, float V);
void ramp_block(float *u, float *v, long width, long height,
                int vpos, int hpos, int h, int w, int fliph);
void homogen_uv(float F, float k, float * hU, float * hV);
void i5_bkg(float *u, float *v, long width, long height, int which);
void load_option(float * u, float * v, long width, long height, const char * option);
bool digit_p(char c);
void pattern_load(float * u, float * v, long width, long height,
  const char * pattern_filename, long x, long y, int orient);
long next_patsize(long former);
void byteswap_2_double(double * array);


static int g_color = 0;
static int g_oldcolor = 0;
static int g_pastel_mode = 0;
static int g_paramspace = 0;
static bool g_wrap = false;
static float g_k, g_F;
static int g_ramprects = 0;
static int g_lowback = 0;
static float g_scale = 1.0;
static int g_density = 0;
static bool g_video = false;
static int g_threads;

#define READY_MODULE_GS_SCALAR 1
#define READY_MODULE_GS_HWIV 2

static int g_module = READY_MODULE_GS_HWIV;


int main(int argc, char * * argv)
{
  long i;
  DICEK_INIT_NTHR(g_hw_threads)
  g_threads = g_hw_threads;

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

  for (i = 1; i < argc; i++) {
    if (0) {
    } else if ((i+1<argc) && (strcmp(argv[i],"-calc-module")==0)) {
      // select a calculation module
      /* TODO: This will be replaced with two options, one to select the equations (Gray-Scott vs. Brusselator)
         and another to select a type of calculation (OpenMP, OpenCL with Images, etc.) */
      i++; g_module = atoi(argv[i]);
    } else if (strcmp(argv[i],"-color")==0) {
      // do output in wonderful technicolor
      g_color = 1;
      g_oldcolor = 17;
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
    } else if ((i+1<argc) && (strcmp(argv[i],"-oldcolor")==0)) {
      // select one of the old PDE4 color schemes
      g_color = 1; i++; g_oldcolor = atoi(argv[i]);
    } else if (strcmp(argv[i],"-paramspace")==0) {
      // do a parameter space plot, like in the Pearson paper
      g_paramspace = 1;
    } else if ((i+1<argc) && (strcmp(argv[i],"-pastel")==0)) {
      // select a pastel mode
      i++; g_pastel_mode = atoi(argv[i]);
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
      g_wrap = true;
    } else {
      fprintf(stderr, "Unrecognized argument, or parameter needed: '%s'\n", argv[i]);
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

  /* If user has selected a multi-threaded module, we need to check the threading library to
     see if we have thread synchronization capability */
  if (g_module == READY_MODULE_GS_HWIV) {
    /* We cannot use multiple threads unless DiceK supports thread synchronization */
    if (!(DICEK_SUPPORTS_BLOCKING)) {
      g_threads = 1;
    }
    printf("DiceK reported %d threads.\n", (int) g_hw_threads);
    printf("Will use %d threads.\n", g_threads);
  }

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

  double iteration = 0;
  double fps_avg = 0.0; // decaying average of fps
  double Mcgs;
  while(true) 
  {
    struct timeval tod_record;
    double tod_before, tod_after, tod_elapsed;
    double fps = 0.0;     // frames per second
    long its_done;

    gettimeofday(&tod_record, 0);
    tod_before = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    // compute:
    its_done = 0;
    switch(g_module) {
      default:
      case READY_MODULE_GS_SCALAR:
        for(i=0; i<N_FRAMES_PER_DISPLAY/5; i++) {
          compute_gs_scalar(u, v, du, dv, g_width, g_height, g_wrap, D_u, D_v, g_F, g_k, speed);
          its_done++;
        }
        break;

      case READY_MODULE_GS_HWIV:
        compute_dispatch(u, v, du, dv, g_width, g_height, g_wrap, D_u, D_v, g_F, g_k, speed, g_paramspace,
          N_FRAMES_PER_DISPLAY, g_threads);
        its_done = N_FRAMES_PER_DISPLAY;
        break;
    }
    iteration += ((double)its_done);

    if (g_color) {
      colorize(u, v, du, red, green, blue, g_width, g_height, g_oldcolor, g_pastel_mode);
    }

    gettimeofday(&tod_record, 0);
    tod_after = ((double) (tod_record.tv_sec))
                                + ((double) (tod_record.tv_usec)) / 1.0e6;

    tod_elapsed = tod_after - tod_before;
    fps = ((double)its_done) / tod_elapsed;
    // We display an exponential moving average of the fps measurement
    fps_avg = (fps_avg == 0) ? fps : (((fps_avg * 10.0) + fps) / 11.0);
    Mcgs = fps_avg * ((double) g_width) * ((double) g_height) / 1.0e6;

    char msg[1000];
    sprintf(msg,"GrayScott - %0.2f fps (%.2f Mcgs)", fps_avg, Mcgs);

    // display:
    {
      int chose_quit;
      if (g_color) {
        chose_quit = display(g_width,g_height,red,green,blue,iteration,g_scale,false,200.0f,2,10,msg,g_video);
      } else {
        chose_quit = display(g_width,g_height,u,u,u,iteration,g_scale,false,200.0f,2,10,msg,g_video);
      }
      if (chose_quit) // did user ask to quit?
        break;
    }
  }
}

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
# define min(a,b) (((a) < (b)) ? (a) : (b))
# define minmax(v, lo, hi) max(lo, min(v, hi))
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
void pearson_bkg(float *u, float *v, long width, long height)
{
  int i, j;
  for(i=0; i<height; i++) {
    for(j=0; j<width; j++) {
      u[i*width+j] = frand(0.99, 1.0);
      v[i*width+j] = frand(0.0, 0.01);
    }
  }
}

/* pearson_block creates a block filled with a given U and V value combined
   with superimposed noise of amplitude 0.01 */
void pearson_block(float *u, float *v, long width, long height,
                 int vpos, int hpos, int h, int w, float U, float V)
{
  int i, j;

  for(i=vpos; i<vpos+h; i++) {
    if ((i >= 0) && (i < height)) {
      for(j=hpos; j<hpos+w; j++) {
        if ((j >= 0) && (j < width)) {
          u[i*width+j] = frand(U-0.005, U+0.005);
          v[i*width+j] = frand(V-0.005, V+0.005);
        }
      }
    }
  }
}

/* ramp_block creates a starting pattern for B-Z spirals and continuous
   propagating wave fronts (pattern type xi in my paper). */
void ramp_block(float *u, float *v, long width, long height,
                int vpos, int hpos, int h, int w, int fliph)
{
  int i, j;
  float U, V;

  for(i=vpos; i<vpos+h; i++) {
    if ((i >= 0) && (i < height)) {
      for(j=hpos; j<hpos+w; j++) {
        if ((j >= 0) && (j < width)) {
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

          u[i*width+j] = frand(U-0.005, U+0.005);
          v[i*width+j] = frand(V-0.005, V+0.005);
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
void i5_bkg(float *u, float *v, long width, long height, int which)
{
  int i, j;

  for(i=0; i<height; i++) {
    for(j=0; j<width; j++) {
      float U, V;

      if (which == 0) {
        U = 1.0; V = 0.0;
      } else {
        homogen_uv(g_F, g_k, &U, &V);
      }
      // printf("F %g  k %g  A %g  U %g  V %g\n", F, k, A, U, V);
      u[i*width+j] = U;
      v[i*width+j] = V;
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
  while(digit_p(*p1)) { p1++; }

  /* Now we expect to see "," */
  if(!(*p1 == ',')) {
    /* We got to the end of the string, or the X coordinate was followed by something other than a comma */
    fprintf(stdout, "invalid load format: '%s'\n", option);
    exit(-1);
  }
  p1++; // skip the ','
  y = atol(p1);
  while(digit_p(*p1)) { p1++; }

  /* Now we can see "o" or ":" */
  if (*p1 == 'o') {
    /* get orientation */
    p1++; // skip the 'o'
    orient = atoi(p1);
    while(digit_p(*p1)) { p1++; }
  }
  if(!(*p1 == ':')) {
    /* We got to the end of the string, or the X and Y were not followed by ":" */
    fprintf(stdout, "invalid load format: '%s'\n", option);
    exit(-1);
  }
  p1++; // skip the ':'
  if (*p1) {
    pattern_load(u, v, width, height, p1, x, y, orient);
  }
}

bool digit_p(char c)
{
  return((c == '-') || ((c >= '0') && (c <= '9')));
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
// Based on: vcpptips.wordpress.com/tag/get-the-size-of-a-file-without-opening-it/
// TODO: This needs to be tested
#   define FILE_LENGTH(name, result) \
    ULONGLONG _LEN_res_tmp; \
    WIN32_FIND_DATA _LEN_fff_dat = { 0 }; \
    HANDLE _LEN_fff_hdl = FindFirstFile(name, &_LEN_fff_dat); \
    if (_LEN_fff_hdl != INVALID_HANDLE_VALUE) { \
      FindClose(_LEN_fff_hdl); \
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

You'll notice an odd scaling factor of "0.63324". Most of the patterns in my collection were created in my PDE4 program
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
difference is sqrt(0.164/0.409) = 0.63324.

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
  float scale = 0.63324 * sqrt(g_scale / 2.0);

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
      i2 = (long) (scale * ((float) i2));
      for(j = 0; j < csz; j++) { // j is column number, X dimension
        float u_val, v_val;
        // j2 = (j - csz/2L)*63L/100L;
        j2 = j - csz/2L;
        j2 = (long) (scale * ((float) j2));

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

  /* NOTE: We assume 2's complement notation, in which case (x & (x-1)) is zero iff x is a power of 2 */
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

void init(float *u, float *v, long width, long height, int density, int lowback, int BZrects)
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

  i5_bkg(u, v, width, height, lowback ? 0 : 1);

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
        ramp_block(u, v, width, height, v1, h1, h, 100, rand() & 1); // formerly 60
      } else {
        pearson_block(u, v, width, height, v1, h1, vs, hs, U, V);
      }
    } else {
      pearson_block(u, v, width, height, v1, h1, vs, hs, U, V);
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

/*

./ReaDy -color -threads 2 -F 0.062 -k 0.0609 -wrap -load '128,128:patterns/interactions/U-curve-15-c'

./ReaDy -color -threads 2 -F 0.062 -k 0.0609 -wrap -load '80,140o5:patterns/halftargets/ht-4-10-3' \
  -load '30,30:patterns/uskates/std'

*/
