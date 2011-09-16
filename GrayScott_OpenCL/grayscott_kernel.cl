__kernel void grayscott_compute(__global float *A,__global float *B,__global float *A2, __global float *B2,
    float k,float f,float r_a,float r_b,float speed) 
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
    const int xm1 = (x-1+X)%X;
    const int xp1 = (x+1)%X;
    const int ym1 = (y-1+X)%Y;
    const int yp1 = (y+1)%Y;
    const int iLeft = xm1*Y + y;
    const int iRight = xp1*Y + y;
    const int iUp = x*Y + ym1;
    const int iDown = x*Y + yp1;
    const int iUpLeft = xm1*Y + ym1;
    const int iUpRight = xp1*Y + ym1;
    const int iDownLeft = xm1*Y + yp1;
    const int iDownRight = xp1*Y + yp1;
    const float dda = A[iLeft] + A[iRight] + A[iUp] + A[iDown] - 4*aval; // (faster to use von Neumann neighborhood but sometimes get grid artefacts)
    const float ddb = B[iLeft] + B[iRight] + B[iUp] + B[iDown] - 4*bval;
    //const float dda = A[iLeft] + A[iRight] + A[iUp] + A[iDown] + A[iUpLeft] + A[iUpRight] + A[iDownLeft] + A[iDownRight] - 8*aval;
    //const float ddb = B[iLeft] + B[iRight] + B[iUp] + B[iDown] + B[iUpLeft] + B[iUpRight] + B[iDownLeft] + B[iDownRight] - 8*bval;
    
    // compute the new rate of change
    const float da = r_a * dda - aval*bval*bval + f*(1-aval);
    const float db = r_b * ddb + aval*bval*bval - (f+k)*bval;
    
    // apply the change (to the new buffer)
    A2[i] = aval + speed * da;
    B2[i] = bval + speed * db;
}
