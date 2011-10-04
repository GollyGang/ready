/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

void compute_gs_scalar(float *a, float *b, float *da, float *db, long width, long height, bool wrap,
             float r_a,float r_b,float f,float k, float speed);
