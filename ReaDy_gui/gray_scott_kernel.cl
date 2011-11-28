__kernel void grayscott_compute(
    __global float *U,__global float *V,
    __global float *U2, __global float *V2,
    float k,float F,float D_u,float D_v,float delta_t)
{
    // Get the index of the current element.
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int X = get_global_size(0);
    const int Y = get_global_size(1);
    const int i = x*Y+y;

    const float u = U[i];
    const float v = V[i];
    
    const int xm1 = ((x-1+X) & (X-1));
    const int xp1 = ((x+1) & (X-1));
    const int ym1 = ((y-1+Y) & (Y-1));
    const int yp1 = ((y+1) & (Y-1));
    const int iLeft = xm1*Y + y;
    const int iRight = xp1*Y + y;
    const int iUp = x*Y + ym1;
    const int iDown = x*Y + yp1;

    // Standard 5-point stencil
    const float nabla_u = U[iLeft] + U[iRight] + U[iUp] + U[iDown] - 4*u;
    const float nabla_v = V[iLeft] + V[iRight] + V[iUp] + V[iDown] - 4*v;

    // compute the new rate of change
    const float delta_u = D_u * nabla_u - u*v*v + F*(1.0f-u);
    const float delta_v = D_v * nabla_v + u*v*v - (F+k)*v;

    // apply the change (to the new buffer)
    U2[i] = u + delta_t * delta_u;
    V2[i] = v + delta_t * delta_v;
}
