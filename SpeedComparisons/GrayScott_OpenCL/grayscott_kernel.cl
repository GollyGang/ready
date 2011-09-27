__kernel void grayscott_compute(
    __global float *U,__global float *V,
    __global float *U2, __global float *V2,
    float k,float F,float D_u,float D_v,float delta_t, int wrap)
{
    // Get the index of the current element. X and Y are oriented like
    // in high school math class, with the origin (0,0) in the bottom-left
    // corner.
    const int x = get_global_id(0);   // column (0=leftmost)
    const int y = get_global_id(1);   // row (0=bottom-most)
    const int X = get_global_size(0); // number of columns
    const int Y = get_global_size(1); // number of rows
    const int i = x*Y+y; // column * number_of_rows + row

    const float u = U[i]; // get U
    const float v = V[i]; // get V
    
    // compute the Laplacians of a and b
    const int xm1 = wrap ? ((x-1+X)%X) : max(x-1,0);
    const int xp1 = wrap ? ((x+1)%X)   : min(x+1,X-1);
    const int ym1 = wrap ? ((y-1+Y)%Y) : max(y-1,0);
    const int yp1 = wrap ? ((y+1)%Y)   : min(y+1,Y-1);
    const int iLeft = xm1*Y + y;
    const int iRight = xp1*Y + y;
    const int iUp = x*Y + ym1;   // Actually down: y=0 is the bottom edge
    const int iDown = x*Y + yp1; // Actually up

    // Standard 5-point stencil
    const float nabla_u = U[iLeft] + U[iRight] + U[iUp] + U[iDown] - 4*u;
    const float nabla_v = V[iLeft] + V[iRight] + V[iUp] + V[iDown] - 4*v;

   // 9-point stencil of Arad et al. 1997
   //    Arad, A Yakhot, G Ben-Dor. A Highly Accurate Numerical Solution
   //    of a Biharmonic Equation. Numer. Meth. PDE, 13, pp. 375-397, 1997.
   //    PDF at www.bgu.ac.il/~yakhot/homepage/publications/nmpde_4_97.pdf
   //    (see page 379)
   // gives more correct results (no vertical/horizontal bias) but slows down
   // the kernal a lot mainly because of the extra accesses to get neighboring
   // U[] and V[] values.
   //
   // const int iUpLeft = xm1*Y + ym1;
   // const int iUpRight = xp1*Y + ym1;
   // const int iDownLeft = xm1*Y + yp1;
   // const int iDownRight = xp1*Y + yp1;
   // const float nabla_u = (U[iLeft]+U[iRight]+U[iUp]+U[iDown])*2.0f/3.0f
   // +(U[iUpLeft]+U[iUpRight]+U[iDownLeft]+U[iDownRight])/6.0f - 10.0f*u/3.0f;
   // const float nabla_v = (V[iLeft]+V[iRight]+V[iUp]+V[iDown])*2.0f/3.0f
   // +(V[iUpLeft]+V[iUpRight]+V[iDownLeft]+V[iDownRight])/6.0f - 10.0f*v/3.0f;

    // compute the new rate of change
    const float delta_u = D_u * nabla_u - u*v*v + F*(1.0f-u);
    const float delta_v = D_v * nabla_v + u*v*v - (F+k)*v;

    // apply the change (to the new buffer)
    U2[i] = u + delta_t * delta_u;
    V2[i] = v + delta_t * delta_v;

    // This is how to discover the row and column numbering. If you enable
    // the following, and get a black bar near the left edge, that means
    // "x=10" is a column near the left edge. Similarly "(y==10)" causes
    // a dark line near the bottom edge.
    // U2[i] = (x==10) ? 0.0f : (u + delta_t * delta_u);
}

/*

//  This version loads data into a local cache, which was expected to be faster, see http://www.khronos.org/message_boards/viewtopic.php?p=11081#p11081
//  Maybe the if() statement outweighs any speedup. 
//  Maybe OpenCL is already using local memory, hence why using NDRange local(8,8) is faster than (1,1).

#define LOCAL_X 8
#define LOCAL_Y 8
// annoyingly, we have to keep these manually synced with the size of the local group in the host

__kernel void grayscott_compute(
    __global float *U,__global float *V,
    __global float *U2, __global float *V2,
    float k,float F,float D_u,float D_v,float delta_t, int wrap)
{
    // Get the index of the current element. X and Y are oriented like
    // in high school math class, with the origin (0,0) in the bottom-left
    // corner.
    const int x = get_global_id(0);   // column (0=leftmost)
    const int y = get_global_id(1);   // row (0=bottom-most)
    const int X = get_global_size(0); // number of columns
    const int Y = get_global_size(1); // number of rows
    const int i = x*Y+y; // column * number_of_rows + row

    // make a local cache of nearby data, for speed    
    const int local_x = get_local_id(0);
    const int local_y = get_local_id(1);
    __local float local_U[LOCAL_X][LOCAL_Y];
    __local float local_V[LOCAL_X][LOCAL_Y];
    local_U[local_x][local_y] = U[i];
    local_V[local_x][local_y] = V[i];
    barrier(CLK_LOCAL_MEM_FENCE);  // wait until all the local threads have copied their data into the local cache
    
    const float u = U[i]; // get U
    const float v = V[i]; // get V
    
    if(local_x>0 && local_y>0 && local_x<LOCAL_X-1 && local_y<LOCAL_Y-1)
    {
        // faster version, can use local cache
        
        // compute the Laplacians of a and b
    
        // Standard 5-point stencil
        const float nabla_u = local_U[local_x-1][local_y] + local_U[local_x+1][local_y] + local_U[local_x][local_y-1] + local_U[local_x][local_y+1] - 4*u;
        const float nabla_v = local_V[local_x-1][local_y] + local_V[local_x+1][local_y] + local_V[local_x][local_y-1] + local_V[local_x][local_y+1] - 4*v;

        // compute the new rate of change
        const float delta_u = D_u * nabla_u - u*v*v + F*(1.0f-u);
        const float delta_v = D_v * nabla_v + u*v*v - (F+k)*v;

        // apply the change (to the new buffer)
        U2[i] = u + delta_t * delta_u;
        V2[i] = v + delta_t * delta_v;
    }
    else
    {
        // slower version, must use global data

        // compute the Laplacians of a and b
        const int xm1 = wrap ? ((x-1+X)%X) : max(x-1,0);
        const int xp1 = wrap ? ((x+1)%X)   : min(x+1,X-1);
        const int ym1 = wrap ? ((y-1+Y)%Y) : max(y-1,0);
        const int yp1 = wrap ? ((y+1)%Y)   : min(y+1,Y-1);
        const int iLeft = xm1*Y + y;
        const int iRight = xp1*Y + y;
        const int iUp = x*Y + ym1;   // Actually down: y=0 is the bottom edge
        const int iDown = x*Y + yp1; // Actually up
    
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
}

*/
