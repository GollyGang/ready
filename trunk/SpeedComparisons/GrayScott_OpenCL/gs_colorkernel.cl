__kernel void grayscott_colour(
    __global float *U, __global float *V, __global float *U2,
    __global float *R, __global float *G, __global float *B)
{
    // Get the index of the current element. X and Y are oriented like
    // in high school math class, with the origin (0,0) in the bottom-left
    // corner.
    const int x = get_global_id(0);   // column (0=leftmost)
    const int y = get_global_id(1);   // row (0=bottom-most)
    const int X = get_global_size(0); // number of columns
    const int Y = get_global_size(1); // number of rows
    const int i = x*Y+y; // column * number_of_rows + row
    
    const float u = U[i]; // get u (called "t2" in my PDE4 colourmap 17)
    const float v = V[i]; // get V
    const float old_u = U2[i]; // previous U

    // compute the derivative (actually the differential times delta_t)
    // scale it up to a level that is likely to be visible and make sure
    // it is within the range [0..1]
    float delta_u = ((u - old_u) * 1000.0f) + 0.5f;
    delta_u = clamp(delta_u, 0.0f, 1.0f);

    // Something simple to start (-:
    // different colour schemes result if you reorder these, or replace
    // "x" with "1.0f-x" for any of the 3 variables
    R[i] = delta_u; // increasing U will look pink
    G[i] = 1.0f-u;
    B[i] = 1.0f-v;
}
