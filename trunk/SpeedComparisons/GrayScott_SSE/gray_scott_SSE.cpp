/*

A port of part of Greg Turk's reaction-diffusion code, from:
http://www.cc.gatech.edu/~turk/reaction_diffusion/reaction_diffusion.html

See README.txt for more details.

*/

// stdlib:
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h> // for _mm_malloc() on Windows

// SSE:
#include <xmmintrin.h>

// local:
#include "defs.h"
#include "display.h"

// N.B.! This is not yet a working implementation of Gray-Scott, still just exploring SSE tutorials:

int main()
{
    printf("Starting calculation...\n");

    clock_t start,end;
    start = clock();

	const int length = 64000;

	// We will be calculating Y = sqrt(x) / x, for x = 1->64000

	// If you do not properly align your data for SSE instructions, you may take a huge performance hit.
    float *pResult = (float*)_mm_malloc(length*sizeof(float),16);

	__m128 x;
	__m128 xDelta = _mm_set1_ps(4.0f);		// Set the xDelta to (4,4,4,4)
	__m128 *pResultSSE = (__m128*) pResult;

	const int SSELength = length / 4;

	for (int stress = 0; stress < 1000; stress++)
	{
#define USE_SSE	// Define this if you want to run with SSE
#ifdef USE_SSE
		x = _mm_set_ps(4.0f, 3.0f, 2.0f, 1.0f);	// Set the initial values of x to (4,3,2,1)

		for (int i=0; i < SSELength; i++)
		{
			__m128 xSqrt = _mm_sqrt_ps(x);
			// Note! Division is slow. It's actually faster to take the reciprocal of a number and multiply
			// Also note that Division is more accurate than taking the reciprocal and multiplying

			pResultSSE[i] = _mm_div_ps(xSqrt, x);
			
			// NOTE! Sometimes, the order in which things are done in SSE may seem reversed.
			// When the command above executes, the four floating elements are actually flipped around
			// We have already compensated for that flipping by setting the initial x vector to (4,3,2,1) instead of (1,2,3,4)

			x = _mm_add_ps(x, xDelta);	// Advance x to the next set of numbers
		}
#endif	// USE_SSE
#ifndef USE_SSE
		float xFloat = 1.0f;
		for (int i=0 ; i < length; i++)
		{
			pResult[i] = sqrt(xFloat) / xFloat;	// Even though division is slow, there are no intrinsic functions like there are in SSE
			xFloat += 1.0f;
		}
#endif	// !USE_SSE
	}

    end = clock();

	// To prove that the program actually worked
	for (int i=0; i < 20; i++)
	{
		printf("Result[%d] = %f\n", i, pResult[i]);
	}

    printf("%f seconds\n",(end-start)/(float)CLOCKS_PER_SEC);

	_mm_free(pResult);

	return 0;
}

