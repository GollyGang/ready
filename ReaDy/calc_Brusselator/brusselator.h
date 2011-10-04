/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

void bruss_init(float *a, float *b, long width, long height, float A, float B);

void compute_bruss(float *a, float *b, float *da, float *db, long width, long height,
             float A, float B, float D1, float D2, float speed, bool parameter_space);
