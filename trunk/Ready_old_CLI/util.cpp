/*

  util.cpp  from ReaDy module

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

 */

#include <stdlib.h>

// return a random value between lower and upper
float ut_frand(float lower, float upper)
{
  return lower + rand()*(upper-lower)/RAND_MAX;
}
