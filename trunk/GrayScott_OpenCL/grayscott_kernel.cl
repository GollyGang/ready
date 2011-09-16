__kernel void grayscott_compute(__global float *A,__global float *B,__global float *A2, __global float *B2) 
{
    // Get the index of the current element
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int X = get_global_size(0);
    const int Y = get_global_size(1);
    const int i = x*Y+y;
    
    const float aval = A[i];
    const float bval = B[i];
    
    // compute the Laplacians of a and b
    const int iLeft = ((x-1+X)%X)*Y + y;
    const int iRight = ((x+1)%X)*Y + y;
    const int iUp = x*Y + (y-1+Y)%Y;
    const int iDown = x*Y + (y+1)%Y;
    const float dda = A[iLeft] + A[iRight] + A[iUp] + A[iDown] - 4*aval;
    const float ddb = B[iLeft] + B[iRight] + B[iUp] + B[iDown] - 4*bval;
    
    // main parameters:
    const float k = 0.064f; // spots
    const float f = 0.035f;
    
    // diffusion coefficients (b diffuses slower than a)
    const float r_a = 0.082f;
    const float r_b = 0.041f;
    
    // compute the new rate of change
    const float da = r_a * dda - aval*bval*bval + f*(1-aval);
    const float db = r_b * ddb + aval*bval*bval - (f+k)*bval;
    
    // timestep size (bigger is faster but more unstable)
    const float speed = 1.0f;

    // apply the change (to the new buffer)
    A2[i] = aval + speed * da;
    B2[i] = bval + speed * db;
}
