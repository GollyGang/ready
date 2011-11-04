__kernel void grayscott_compute(
    __global float2 *UV,
    __global float2 *UV2,
    float k,float F,float D_u,float D_v,float delta_t)
{
    // Get the index of the current element
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int X = get_global_size(0);
    const int Y = get_global_size(1);
    const int i = x*Y+y;

    const float2 uv = UV[i];
    
    // compute the Laplacians of a and b
#ifdef WRAP
    // speedy modulo operator for when X and Y are powers of 2
    // http://forums.amd.com/devforum/messageview.cfm?catid=390&threadid=143648
    const int xm1 = ((x-1+X) & (X-1));    // = ((x-1+X)%X);
    const int xp1 = ((x+1) & (X-1));      // = ((x+1)%X);
    const int ym1 = ((y-1+Y) & (Y-1));    // = ((y-1+Y)%Y);
    const int yp1 = ((y+1) & (Y-1));      // = ((y+1)%Y);
#else
    const int xm1 = max(x-1,0);
    const int xp1 = min(x+1,X-1);
    const int ym1 = max(y-1,0);
    const int yp1 = min(y+1,Y-1);
#endif
    const int iLeft = xm1*Y + y;
    const int iRight = xp1*Y + y;
    const int iUp = x*Y + ym1;
    const int iDown = x*Y + yp1;
    // Standard 5-point stencil
    const float2 nabla_uv = UV[iLeft] + UV[iRight] + UV[iUp] + UV[iDown] - 4*uv;

    // compute the new value
    UV2[i] = uv + delta_t * (float2)(D_u * nabla_uv.x - uv.x*uv.y*uv.y + F*(1.0f-uv.x), D_v * nabla_uv.y + uv.x*uv.y*uv.y - (F+k)*uv.y);
}
