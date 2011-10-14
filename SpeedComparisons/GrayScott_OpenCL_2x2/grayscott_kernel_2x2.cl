__kernel void grayscott_compute(
    __global float4 *U,__global float4 *V,
    __global float4 *U2, __global float4 *V2,
    float k,float F,float D_u,float D_v,float delta_t)
{
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int X = get_global_size(0);
    const int Y = get_global_size(1);
    const int i = x*Y+y;

    const float4 u = U[i];
    const float4 v = V[i];
    
    // compute the Laplacians of a and b
    const int xm1 = max(x-1,0);
    const int xp1 = min(x+1,X-1);
    const int ym1 = max(y-1,0);
    const int yp1 = min(y+1,Y-1);
    const int iLeft = xm1*Y + y;
    const int iRight = xp1*Y + y;
    const int iUp = x*Y + ym1;
    const int iDown = x*Y + yp1;
    
    const float4 u_left = U[iLeft];
    const float4 u_right = U[iRight];
    const float4 u_up = U[iUp];
    const float4 u_down = U[iDown];
    const float4 v_left = V[iLeft];
    const float4 v_right = V[iRight];
    const float4 v_up = V[iUp];
    const float4 v_down = V[iDown];

    const float4 nabla_u = (float4)(
        u_left.y + u_up.z + u.y + u.z,
        u.x + u_up.w + u_right.x + u.w,
        u_left.w + u.x + u.w + u_down.x,
        u.z + u.y + u_right.z + u_down.y) - 4.0f*u;
    const float4 nabla_v = (float4)(
        v_left.y + v_up.z + v.y + v.z,
        v.x + v_up.w + v_right.x + v.w,
        v_left.w + v.x + v.w + v_down.x,
        v.z + v.y + v_right.z + v_down.y) - 4.0f*v;

    // compute the new rate of change
    const float4 delta_u = D_u * nabla_u - u*v*v + F*(1.0f-u);
    const float4 delta_v = D_v * nabla_v + u*v*v - (F+k)*v;

    // apply the change (to the new buffer)
    U2[i] = u + delta_t * delta_u;
    V2[i] = v + delta_t * delta_v;
}
