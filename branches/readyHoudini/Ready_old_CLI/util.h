/*

  util.h  from ReaDy module

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

 */

#ifndef max
# define max(a,b) (((a) > (b)) ? (a) : (b))
# define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef minmax
# define minmax(v, lo, hi) max(lo, min(v, hi))
#endif

float ut_frand(float lower, float upper);
