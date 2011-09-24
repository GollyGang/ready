/*
  
  hwi_vector.h

This is a hardware-independent vector library. It enables vector SIMD code
to be written, compiled and run on non-vector machines. Once the code is
tested and working, a compile-time macro can be changed, and a recompile
causes the program to use actual vector instructions.

In the simplest case, that is all that is needed. In real applications
there are usually several more steps:

  * The application developer decides what base hardware platform to
    compile for. This might be the oldest CPU the company will support,
    such as a Pentium 4 HT.
  * The compiler defines certain flags, such as __i386__ and __SSE2__
    that can be tested by an #ifdef
  * The source code can set flags of its own, such as HWIV_EMULATE
    or HWIV_WANT_V4F4. This might be done in order to create two versions
    of a calculation routine (one that uses vector instructions and one that
    does not)
  * The program is built from one or more compilations of the same source.
    This might be done in order to create a "universal binary" capable of
    being copied to and run on a variety of computer products. A typical
    example is a program file that contains both a 32-bit and a 64-bit
    version, and the operating system loads whichever one is appropriate
    when the program is launched.
  * At run-time, the program tests for the presence of vector instructions
    using the CPUID instruction or its equivalent on non Intel CPUs.
  * At run-time, based on the CPUID test, the program transfers control
    to one or another of the calculation subroutines depending on which
    vector instructions are actually available.

*/

// First, honor the user's request to use emulation
#ifdef HWIV_EMULATE
# define HWIV_V4F4_EMULATED
#endif

// Next, find out if the compiler will give us SSE2 intrinsics
#ifndef HWIV_V4F4_EMULATED
# if defined(__SSE2__)
#   define HWIV_V4F4_SSE2
# endif
#endif

// Finally, fall back to emulated if no hardware option is available
#ifndef HWIV_V4F4_SSE2
# ifndef HWIV_V4F4_EMULATED
#   define HWIV_V4F4_EMULATED
# endif
#endif


// See is client wants V4F4
#ifdef HWIV_WANT_V4F4

// Okay, now see how we should create the macros for V4F4

# ifdef HWIV_V4F4_EMULATED

/* Scalar code to emulate the V4F4 vector model */
#define HWIV_HAVE_V4F4

typedef float V4F4[4];

/* LOAD_4F4: dst is a vector of 4 floats. src is a pointer to an array
   of floats in memory. This opcode loads 4 consecutive floats from the
   given location into the vector. The first float from memory will be loaded
   into element 0 of the vector, the 2nd into element 1, and so on. */
#define HWIV_LOAD_4F4(dst, src) ({ (dst)[0]=(src)[0]; (dst)[1]=(src)[1]; \
                                   (dst)[2]=(src)[2]; (dst)[3]=(src)[3]; })

/* LOADO_4F4: dst is a vector of 4 floats. src is a pointer to an array
   of floats in memory. offset is a byte offset, which must be a multiple
   of sizeof(float).
     This opcode loads 4 consecutive floats from the given location plus
   offset into the vector. The first float from memory will be loaded into
   element 0 of the vector, the 2nd into element 1, and so on. */
#define HWIV_LOADO_4F4(dst, src, offset) ({ (dst)[0]=(src)[(offset)/4]; \
                                            (dst)[1]=(src)[(offset)/4+1]; \
                                            (dst)[2]=(src)[(offset)/4+2]; \
                                            (dst)[3]=(src)[(offset)/4+3]; })

/* COPY_4F4: dst and src are each a vector of 4 floats. This opcode copies
   the contents of the source vector into the destination vector. */
#define HWIV_COPY_4F4(dst, src)  HWIV_LOAD_4F4((dst), (src))

/* FILL_4F4: dst is a vector of 4 floats. sc0, sc1, sc2, and sc3 ("scalars")
   are each a float. tmp is a (float *) pointing to memory of sufficient
   size to hold four floats, and aligned to a 16-byte boundary. Use the
   HWIV_4F4_ALIGNED typedef to declare a suitable float[4] which can
   be passed as tmp to this and other similar macros.
     This opcode loads the 4 scalar values into the vector. The sc0 will be
   loaded into element 0 of the vector, sc1 into element 1, and so on. */
#define HWIV_FILL_4F4(dst, sc0, sc1, sc2, sc3, tmp) ({ \
            (dst)[0]=(sc0); (det)[1]=(sc1); (dst)[2]=(sc2); (dst)[3]=(sc3); })
typedef float HWIV_4F4_ALIGNED[4];

/* SAVE_4F4: dst is a pointer to an array of floats in memory. src is a
   vector of 4 floats. 
     This opcode stores the 4 floats in the source vector into 4 consecutive
   float-sized blocks of memory (i.e. 16 consecutive bytes, 4 bytes per
   float) beginning at the given destination location. Element 0 of the
   vector will be stored into the first 4 bytes in memory, element 1 into
   the next 4 bytes of memory, and so on. */
#define HWIV_SAVE_4F4(dst, src)  memcpy((void *) (dst), (void *) (src), 16);

/* SAVEO_4F4: dst is a pointer to an array of floats in memory. src is a
   vector of 4 floats. offset is a byte offset, which must be a multiple
   of sizeof(float). 
     This opcode stores the 4 floats in the source vector into 4 consecutive
   float-sized blocks of memory (i.e. 16 consecutive bytes, 4 bytes per
   float) beginning at the given destination location plus offset. Element
   0 of the vector will be stored into the first 4 bytes in memory, element
   1 into the next 4 bytes of memory, and so on. */
#define HWIV_SAVEO_4F4(dst, offset, src) ({ (dst)[(offset)/4]=(src)[0]; \
                                         (dst)[(offset)/4+1]=(src)[1]; \
                                         (dst)[(offset)/4+2]=(src)[2]; \
                                         (dst)[(offset)/4+3]=(src)[3]; })

/* ADD_4F4: dst, a, and b are each a vector of 4 floats.
     This opcode adds each of the components of a to the corresponding
   component of b and puts the result into dst. */
#define HWIV_ADD_4F4(dst, a, b) ({ (dst)[0]=a[0]+b[0]; (dst)[1]=a[1]+b[1]; \
                                (dst)[2]=a[2]+b[2]; (dst)[3]=a[3]+b[3]; })

#define HWIV_INIT_MUL0_4F4(varname)

/* MUL_4F4: dst, a, and b are each a vector of 4 floats. v0 is a variable
   declared with the HWIV_INIT_MUL0_4F4 macro.
     This opcode multiplies each of the components of a to the corresponding
   component of b and puts the result into dst. The varible v0 is used on
   hardware that has no 2-argument multiply operation. */
#define HWIV_MUL_4F4(dst, a, b, v0) \
                             ({ (dst)[0]=a[0]*b[0]; (dst)[1]=a[1]*b[1]; \
                                (dst)[2]=a[2]*b[2]; (dst)[3]=a[3]*b[3]; })

#define HWIV_INIT_MTMP_4F4(varname)

/* MADD_4F4: dst, a, b, and c are each a vector of 4 floats. t is a variable
   declared with the HWIV_INIT_MTMP_4F4 macro.
     This opcode multiplies each of the components of a to the corresponding
   component of b, then adds the corresponding component of c, and puts the
   result into dst. The varible t is used on hardware that has no 3-argument
   multiply-add operation. */
#define HWIV_MADD_4F4(dst, a, b, c, t)  ({ (dst)[0]=a[0]*b[0] + c[0]; \
                                        (dst)[1]=a[1]*b[1] + c[1]; \
                                        (dst)[2]=a[2]*b[2] + c[2]; \
                                        (dst)[3]=a[3]*b[3] + c[3]; })

/* NMSUB_4F4: dst, a, b, and c are each a vector of 4 floats. t is a variable
   declared with the HWIV_INIT_MTMP_4F4 macro.
     This opcode multiplies each of the components of a to the corresponding
   component of b, then subtracts that product from the corresponding
   component of c, then puts the result into dst. The varible t is used on
   hardware that has no 3-argument multiply-add operation. */
#define HWIV_NMSUB_4F4(dst, a, b, c, t) ({ (dst)[0]=c[0] - a[0]*b[0]; \
                                        (dst)[1]=c[1] - a[1]*b[1]; \
                                        (dst)[2]=c[2] - a[2]*b[2]; \
                                        (dst)[3]=c[3] - a[3]*b[3]; })

# endif


# ifdef HWIV_V4F4_SSE2

/* SSE3 implementation of the V4F4 vector model */
#define HWIV_HAVE_V4F4

# if (defined(HWIV_USE_IMMINTRIN) || defined (__AVX__))
#   include <immintrin.h>
# else
#   include <xmmintrin.h>
#   include <emmintrin.h>
# endif


typedef __m128 V4F4;

#define HWIV_LOAD_4F4(dst, src)        (dst) = _mm_load_ps(src)
#define HWIV_LOADO_4F4(dst, src, offset) \
                                       (dst) = _mm_load_ps((src)+(offset)/4)
#define HWIV_COPY_4F4(dst, src)        (dst) = (src)

#define HWIV_FILL_4F4(dst, sc0, sc1, sc2, sc3, tmp) ({ (tmp)[0]=(sc0); \
               (tmp)[1]=(sc1); (tmp)[2]=(sc2); (tmp)[3]=(sc3); \
               HWIV_LOAD_4F4(dst, tmp); })
typedef float __attribute__((aligned (16))) HWIV_4F4_ALIGNED[4];

#define HWIV_SAVE_4F4(dst, src)        _mm_store_ps((dst), (src))

#define HWIV_SAVEO_4F4(dst, offset, src) \
                                        _mm_store_ps((dst)+(offset)/4, (src))

#define HWIV_ADD_4F4(dst, a, b) (dst) = _mm_add_ps((a), (b))

// For INIT_MUL0, on Intel we do nothing because Intel actually has a
// 2-operand multiply operation.
#define HWIV_INIT_MUL0_4F4(varname) /* nop */

// For INIT_MTMP, on Intel SSE2 we need to declare a variable because
// Intel SSE2 has no FMA (fused multiply-add) operations (this is
// expected to come wth AVX2 on Haskell in 2013)
#define HWIV_INIT_MTMP_4F4(varname) V4F4 varname = _mm_setzero_ps()

#define HWIV_MUL_4F4(dst, a, b, v0)    (dst) = _mm_mul_ps((a), (b))

#define HWIV_MADD_4F4(dst, a, b, c, t) ({ (t) = _mm_mul_ps((a), (b)); \
                                        (dst) = _mm_add_ps((t), (c)); })
#define HWIV_NMSUB_4F4(dst, a, b, c, t) ({ (t) = _mm_mul_ps((a), (b)); \
                                        (dst) = _mm_sub_ps((c), (t)); })

# endif


#endif
