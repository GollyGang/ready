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
    
    // compute the Laplacians of u and v
#ifdef WRAP
    // speedy modulo operator for when X and Y are powers of 2 (we'll enforce this)
    // http://forums.amd.com/devforum/messageview.cfm?catid=390&threadid=143648
    const int xm1 = ((x-1+X) & (X-1));
    const int xp1 = ((x+1) & (X-1));
    const int ym1 = ((y-1+Y) & (Y-1));
    const int yp1 = ((y+1) & (Y-1));
    //const int xm1 = ((x-1+X)%X);
    //const int xp1 = ((x+1)%X);
    //const int ym1 = ((y-1+Y)%Y);
    //const int yp1 = ((y+1)%Y);
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
}
