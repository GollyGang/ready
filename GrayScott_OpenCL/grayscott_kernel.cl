__kernel void grayscott_compute(__global float *A, __global float *B,__global float *A2, __global float *B2,int X,int Y) 
{
    // Get the index of the current element
    int i = get_global_id(0);
    
    int y = i%Y;
    int x = (i-y)/Y;
    
    int iLeft = ((x-1+X)%X)*Y + y;
    int iRight = ((x+1)%X)*Y + y;
    int iDown = x*Y + ((y-1+Y)%Y);
    int iUp = x*Y + ((y+1)%Y);
    
	float aval = A[i];
	float bval = B[i];
	
	// diffusion coefficients (b diffuses slower than a)
    float r_a = 0.082f;
    float r_b = 0.041f;
    
    // timestep size (bigger is faster but more unstable)
    float speed = 1.0f;

	// main parameters:
    float k = 0.064f; // spots
    float f = 0.035f;
    
	// compute the Laplacians of a and b
    float dda = A[iLeft] + A[iRight] + A[iUp] + A[iDown] - 4.0f*aval;
    float ddb = B[iLeft] + B[iRight] + B[iUp] + B[iDown] - 4.0f*bval;
    
    // compute the new rate of change
    float da = r_a * dda - aval*bval*bval + f*(1.0f-aval);
    float db = r_b * ddb + aval*bval*bval - (f+k)*bval;
    
    // apply the change (to the new buffer)
    A2[i] = aval + speed * da;
    B2[i] = bval + speed * db;
}

