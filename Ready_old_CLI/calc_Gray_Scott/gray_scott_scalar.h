/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

void gs_scl_compute_setup(int width, int height, bool wrap, bool paramspace);

void compute_gs_scalar(float *a, float *b, float *da, float *db,
  float r_a, float r_b, float f, float k, float speed);
