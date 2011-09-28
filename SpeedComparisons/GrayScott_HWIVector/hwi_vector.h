/*
  
  hwi_vector.h                                                                                           ^u 140 ^x f

This is a hardware-independent vector library. It enables vector SIMD code to be written, compiled and run on non-vector
machines. Once the code is tested and working, a compile-time macro can be changed, and a recompile causes the program
to use actual vector instructions.

In the simplest case, that is all that is needed. In real applications there are usually several more steps:

  * The application developer decides what base hardware platform to compile for. This might be the oldest CPU the
    company will support, such as a Pentium 4 HT.

  * The compiler defines certain flags, such as __i386__ and __SSE2__ that can be tested by an #ifdef

  * The source code can set flags of its own, such as HWIV_EMULATE or HWIV_WANT_V4F4. This might be done in order to
    create two versions of a calculation routine (one that uses vector instructions and one that does not)

  * The program is built from one or more compilations of the same source. This might be done in order to create a
    "universal binary" capable of being copied to and run on a variety of computer products. A typical example is a
    program file that contains both a 32-bit and a 64-bit version, and the operating system loads whichever one is
    appropriate when the program is launched.

  * At run-time, the program tests for the presence of vector instructions using the CPUID instruction or its
    equivalent on non Intel CPUs.

  * At run-time, based on the CPUID test, the program transfers control to one or another of the calculation
    subroutines depending on which vector instructions are actually available.

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
  // Workaround because Visual Studio doesn't seem to set its _M_IX86_FP flag, 
  // or give us any indication what level of SSE support is available.
  // So we just assume SSE2 is available
# if defined(_M_X64) || defined(_M_IX86)
#  define HWIV_V4F4_SSE2
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                   //
//  #include<stdio.h>                                                                                                //
//  /*                                                     */                    /*                             */   //
//  /*                                                     */                    /*                             */   //
//  int                 main()  {char      a[]     ={     'T',    'h','i',     's',32,     'p','r'        ,'o','g'   //
//  ,'r','a','m',      32,   'j',   'u',   's',    't'    ,32           ,'d',    'o',   'e'     ,'s'    ,32    ,'i'  //
//  ,'t'              ,32    ,'t'    ,'h'  ,'e'   ,32,    'h'    ,97,'r','d'   ,32,    'w',97,121,33,   32,    40,   //
//  68,               'o',   'n',    39,   't'    ,32     ,'y'   ,'o',   117,    32    ,'t'             ,'h'   ,'i'  //
//  ,'n'               /*     Xy     =a     +3     +n      ++     ;a=     b-     (*     x/z              );     if   //
//  (Xy-++n<(z+*x))z  =b;a   +b,     z+=     x*/,107 ,    63,63    ,63,41,'\n'   ,00};    puts(a);}      /*.RPM.*/   //
//                                                                                                                   //
//        Emulated versions of the V4F4 macros. (These also serve as documentation for what each macro does)         //
//                                                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

# ifdef HWIV_V4F4_EMULATED

/* Scalar code to emulate the V4F4 vector model */
#define HWIV_HAVE_V4F4

/* The vector register type */
typedef float V4F4[4];

/* Aligned memory suitable for load to / store from the vector register */
typedef float HWIV_4F4_ALIGNED[4];

/* LOAD_4F4: dst is a vector of 4 floats. src is a pointer to an array
   of floats in memory. This opcode loads 4 consecutive floats from the
   given location into the vector. The first float from memory will be loaded
   into element 0 of the vector, the 2nd into element 1, and so on. */
#define HWIV_LOAD_4F4(dst, src)  { (dst)[0]=(src)[0]; (dst)[1]=(src)[1]; \
                                   (dst)[2]=(src)[2]; (dst)[3]=(src)[3]; }

/* LOADO_4F4: dst is a vector of 4 floats. src is a pointer to an array
   of floats in memory. offset is a byte offset, which must be a multiple
   of sizeof(float).
     This opcode loads 4 consecutive floats from the given location plus
   offset into the vector. The first float from memory will be loaded into
   element 0 of the vector, the 2nd into element 1, and so on. */
#define HWIV_LOADO_4F4(dst, src, offset)  { (dst)[0]=(src)[(offset)/4]; \
                                            (dst)[1]=(src)[(offset)/4+1]; \
                                            (dst)[2]=(src)[(offset)/4+2]; \
                                            (dst)[3]=(src)[(offset)/4+3]; }

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
#define HWIV_FILL_4F4(dst, sc0, sc1, sc2, sc3) { \
            (dst)[0]=(sc0); (dst)[1]=(sc1); (dst)[2]=(sc2); (dst)[3]=(sc3); }
#define HWIV_INIT_FILL /* nop */

#define HWIV_SPLAT_4F4(dst, s) HWIV_FILL_4F4((dst), (s), (s), (s), (s))

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
#define HWIV_SAVEO_4F4(dst, offset, src) { (dst)[(offset)/4]=(src)[0]; \
                                         (dst)[(offset)/4+1]=(src)[1]; \
                                         (dst)[(offset)/4+2]=(src)[2]; \
                                         (dst)[(offset)/4+3]=(src)[3]; }

/* ADD_4F4: dst, a, and b are each a vector of 4 floats.
     This opcode adds each of the components of a to the corresponding
   component of b and puts the result into dst. */
#define HWIV_ADD_4F4(dst, a, b) { (dst)[0]=a[0]+b[0]; (dst)[1]=a[1]+b[1]; \
                                (dst)[2]=a[2]+b[2]; (dst)[3]=a[3]+b[3]; }

#define HWIV_INIT_MUL0_4F4 /* nop */

/* MUL_4F4: dst, a, and b are each a vector of 4 floats. v0 is a variable
   declared with the HWIV_INIT_MUL0_4F4 macro.
     This opcode multiplies each of the components of a to the corresponding
   component of b and puts the result into dst. The varible v0 is used on
   hardware that has no 2-argument multiply operation. */
#define HWIV_MUL_4F4(dst, a, b) \
                             { (dst)[0]=a[0]*b[0]; (dst)[1]=a[1]*b[1]; \
                                (dst)[2]=a[2]*b[2]; (dst)[3]=a[3]*b[3]; }

#define HWIV_INIT_MTMP_4F4 /* nop */

/* MADD_4F4: dst, a, b, and c are each a vector of 4 floats. t is a variable
   declared with the HWIV_INIT_MTMP_4F4 macro.
     This opcode multiplies each of the components of a to the corresponding
   component of b, then adds the corresponding component of c, and puts the
   result into dst. The varible t is used on hardware that has no 3-argument
   multiply-add operation. */
#define HWIV_MADD_4F4(dst, a, b, c)  { (dst)[0]=a[0]*b[0] + c[0]; \
                                        (dst)[1]=a[1]*b[1] + c[1]; \
                                        (dst)[2]=a[2]*b[2] + c[2]; \
                                        (dst)[3]=a[3]*b[3] + c[3]; }

#define HWIV_MSUB_4F4(dst, a, b, c)  { (dst)[0]=a[0]*b[0] - c[0]; \
                                           (dst)[1]=a[1]*b[1] - c[1]; \
                                           (dst)[2]=a[2]*b[2] - c[2]; \
                                           (dst)[3]=a[3]*b[3] - c[3]; }

/* NMSUB_4F4: dst, a, b, and c are each a vector of 4 floats. t is a variable
   declared with the HWIV_INIT_MTMP_4F4 macro.
     This opcode multiplies each of the components of a to the corresponding
   component of b, then subtracts that product from the corresponding
   component of c, then puts the result into dst. The varible t is used on
   hardware that has no 3-argument multiply-add operation. */
#define HWIV_NMSUB_4F4(dst, a, b, c) { (dst)[0]=c[0] - a[0]*b[0]; \
                                        (dst)[1]=c[1] - a[1]*b[1]; \
                                        (dst)[2]=c[2] - a[2]*b[2]; \
                                        (dst)[3]=c[3] - a[3]*b[3]; }

// Declare this if you are doing any raise or lower operation
#define HWIV_INIT_RLTMP_4F4 /* nop */

/* RAISE_4F4: dst, src, extra, and tmp are each a vector of 4 floats.
     This opcode "raises" three of the values from src to the next-higher
   element of dst. Element 0 of dst is filled with the value from element
   3 of "extra". The varible t is used on hardware that requires the
   result to be computed in two pieces and then assembled via a blend
   operation. ("VSHR_4F4" in old macros) */
#define HWIV_RAISE_4F4(dst, src, extra) \
                                      { dst[3]=src[2]; dst[2]=src[1]; \
                                         dst[1]=src[0]; dst[0]=extra[3]; }

/* LOWER_4F4: dst, src, extra, and tmp are each a vector of 4 floats.
     This opcode "lowers" three of the values from src to the next-lower
   element of dst. Element 3 of dst is filled with the value from element
   0 of "extra". The varible t is used on hardware that requires the
   result to be computed in two pieces and then assembled via a blend
   operation. ("VSHL_4F4" in old macros) */
#define HWIV_LOWER_4F4(dst, src, extra) \
                                      { dst[0]=src[1]; dst[1]=src[2]; \
                                         dst[2]=src[3]; dst[3]=extra[0]; }


// We also define a small set of macros for FORTRAN-style code. Using these you can build up expressions like
//
//    a = v4ADD(v4MUL(b,c),d);  /*  a = b*c + d;  */
//
// These macros do not form a complete solution, you still need things like LOAD and SAVE to do any real work.

#define v4ADD(a,b) {(a)[0]+(b)[0],(a)[1]+(b)[1],(a)[2]+(b)[2],(a)[3]+(b)[3]}
#define v4SUB(a,b) {(a)[0]-(b)[0],(a)[1]-(b)[1],(a)[2]-(b)[2],(a)[3]-(b)[3]}
#define v4MUL(a,b) {(a)[0]*(b)[0],(a)[1]*(b)[1],(a)[2]*(b)[2],(a)[3]*(b)[3]}
#define v4SET(v0,v1,v2,v3) {(v0),(v1),(v2),(v3)}
#define v4SPLAT(a) {(a),(a),(a),(a)}
#define v4ROUP(a) {(a)[3],(a)[0],(a)[1],(a)[2]}
#define v4RODN(a) {(a)[1],(a)[2],(a)[3],(a)[0]}
#define v4RAISE(a, new) {(new)[3],(a)[0],(a)[1],(a)[2]}
#define v4LOWER(a, new) {(a)[1],(a)[2],(a)[3],(new)[0]}

# endif
/* - - - - - - - - - - - - - - - - - - - - - End of the EMULATED section  - - - - - - - - - - - - - - - - - - - - - - */


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                   //
//                             @@@@                       @@@@                     @@@@                              //
//                             @@@@                       @@@@                     @@@@                              //
//                             """"                       @@@@                     @@@@                              //
//                             eeee    eeee  ,e@@e..   eee@@@@eee                  @@@@                              //
//                             @@@@    @@@@@@@@@@@@@@. @@@@@@@@@@                  @@@@                              //
//                             @@@@    @@@@f'    `@@@@    @@@@                     @@@@                              //
//                             @@@@    @@@@       @@@@    @@@@        ,e@@@e.      @@@@                              //
//                             @@@@    @@@@       @@@@    @@@@     e@@@@@@@@@@@e   @@@@                              //
//                             @@@@    @@@@       @@@@    @@@@   .@@@@'     `@@@@i @@@@                              //
//                             @@@@    @@@@       @@@@    @@@@kee@@@@eeeeeeeee@@@@ @@@@                              //
//                             @@@@    @@@@       @@@@    `@@@@@@@@@@@@@@@@@@@@@@@@@@@@  (R)                         //
//                                                               @@@@.                                               //
//                                                               `@@@@e.    .eeee-                                   //
//                                                                 *@@@@@@@@@@@*                                     //
//                                                                    "*@@@@*"                                       //
//                                                                                                                   //
//             Versions of the V4F4 macros for the Intel SSE2 (or later) 128-bit vector instruction set              //
//                                                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

#ifdef _WIN32
# define ALIGNED_16 __declspec( align( 16 ) )
#else
# define ALIGNED_16 __attribute__((aligned (16)))
#endif

typedef float ALIGNED_16 HWIV_4F4_ALIGNED[4];

#define HWIV_LOAD_4F4(dst, src)        (dst) = _mm_load_ps(src)
#define HWIV_LOADO_4F4(dst, src, offset) \
                                       (dst) = _mm_load_ps((src)+(offset)/4)
#define HWIV_COPY_4F4(dst, src)        (dst) = (src)

#define HWIV_FILL_4F4(dst, sc0, sc1, sc2, sc3) \
            { HWIV_fill_4F4[0]=(sc0); HWIV_fill_4F4[1]=(sc1); \
               HWIV_fill_4F4[2]=(sc2); HWIV_fill_4F4[3]=(sc3); \
               HWIV_LOAD_4F4(dst, HWIV_fill_4F4); }

#define HWIV_INIT_FILL float ALIGNED_16 HWIV_fill_4F4[4];


#define HWIV_SPLAT_4F4(dst, s)         (dst) = _mm_set1_ps(s)

#define HWIV_SAVE_4F4(dst, src)        _mm_store_ps((dst), (src))

#define HWIV_SAVEO_4F4(dst, offset, src) \
                                        _mm_store_ps((dst)+(offset)/4, (src))

#define HWIV_ADD_4F4(dst, a, b) (dst) = _mm_add_ps((a), (b))

// For INIT_MUL0, on Intel we do nothing because Intel actually has a
// 2-operand multiply operation.
#define HWIV_INIT_MUL0_4F4 /* nop */

// For INIT_MTMP, on Intel SSE2 we need to declare a variable because
// Intel SSE2 has no FMA (fused multiply-add) operations (this is
// expected to come wth AVX2 on Haskell in 2013)
#define HWIV_INIT_MTMP_4F4     V4F4 HWIV_mtmp_4F4 = _mm_setzero_ps()

#define HWIV_MUL_4F4(dst, a, b)    (dst) = _mm_mul_ps((a), (b))

#define HWIV_MADD_4F4(dst, a, b, c) { HWIV_mtmp_4F4 = _mm_mul_ps((a), (b)); \
                                    (dst) = _mm_add_ps(HWIV_mtmp_4F4, (c)); }
#define HWIV_MSUB_4F4(dst, a, b, c) { HWIV_mtmp_4F4 = _mm_mul_ps((a), (b)); \
                                    (dst) = _mm_sub_ps(HWIV_mtmp_4F4, (c)); }
#define HWIV_NMSUB_4F4(dst, a, b, c) { HWIV_mtmp_4F4 = _mm_mul_ps((a), (b)); \
                                    (dst) = _mm_sub_ps((c), HWIV_mtmp_4F4); }

#define HWIV_INIT_RLTMP_4F4 /* nop */


/*
HWIV_RODN_4F4 (ROtate DOwn) does a "downwards rotate" of a 4-element vector using the Intel VSHUFPS instruction
(intrinsic _mm_shuffle_ps). If the input is {a,b,c,d} (with a being element 0) the result of the downwards rotate is
{b,c,d,a} (with each element moving down tot he next-lower slot, except for a which rotates into the top position).

  SRC1  {  x3 ,  x2 ,  x1 ,  x0  }
  SRC2  {  y3 ,  y2 ,  y1 ,  y0  }
  DEST  {  y0 ,  y3 ,  x2 ,  x1  }
  imm8:    00    11    10    01      = 0x39

 */
#define HWIV_RODN_4F4(dest, src)  (dest) = _mm_shuffle_ps((src), (src), 0x39)

/*
HWIV_ROUP_4F4 is an "upwards rotate": if the input is {a,b,c,d} (with a being element 0) the result is {d,a,b,c}.

  SRC1  {  x3 ,  x2 ,  x1 ,  x0  }
  SRC2  {  y3 ,  y2 ,  y1 ,  y0  }
  DEST  {  y2 ,  y1 ,  x0 ,  x3  }
  imm8:    10    01    00    11      = 0x93

 */
#define HWIV_ROUP_4F4(dest, src)  (dest) = _mm_shuffle_ps((src), (src), 0x93)

/*
HWIV_RAISE_4F4 is an "upwards shift": if the input is {a,b,c,d} (with a being element 0) the result is {X,a,b,c} with
the new element X coming from element 3 of the "new" argument.

  new   {  x3 ,  x2 ,  x1 ,  x0  }
  src   {  y3 ,  y2 ,  y1 ,  y0  }
  dest  {  y0 ,  y0 ,  x3 ,  x3  }
  imm8:    00    00    11    11      = 0x0F
          src0  src0  new3  new3

  dest  {  x3 ,  x2 ,  x1 ,  x0  }
   src  {  y3 ,  y2 ,  y1 ,  y0  }
  dest  {  y2 ,  y1 ,  x2 ,  x0  }
  imm8:    10    01    10    00      = 0x98
          src2  src1  src0  new3
*/
#define HWIV_RAISE_4F4(dest, src, new) { (dest) = _mm_shuffle_ps((new), (src), 0x0f); \
                                         (dest) = _mm_shuffle_ps((dest), (src), 0x98);  }

/*
HWIV_LOWER_4F4 is an "downwards shift": if the input is {a,b,c,d} (with a being element 0) the result is {b,c,d,X} with
the new element X coming from element 0 of the "new" argument.

To accomplish a downwards shift we can just use _mm_move_ss to move a single scalar into the bottom position and then do
a RODN (downwards rotate)
 */
#define HWIV_LOWER_4F4(dest, src, new) {  (dest) = _mm_move_ss((src), (new));  \
                                           HWIV_RODN_4F4(dest, dest); }

// Here is the subset for FORTRAN-style code
#define v4ADD(a,b) _mm_add_ps((a), (b))
#define v4SUB(a,b) _mm_sub_ps((a), (b))
#define v4MUL(a,b) _mm_mul_ps((a), (b))
// in v4SET, note the reversal of argument order
#define v4SET(v0,v1,v2,v3) _mm_set_ps((v3),(v2),(v1),(v0))
#define v4SPLAT(a) _mm_set1_ps(a)
#define v4ROUP(src) _mm_shuffle_ps((src), (src), 0x93)
#define v4RODN(src) _mm_shuffle_ps((src), (src), 0x39)
#define v4RAISE(src, new) _mm_shuffle_ps(_mm_shuffle_ps((new), (src), 0x0f), (src), 0x98)
#define v4LOWER(src, new) _mm_shuffle_ps(_mm_move_ss((src),(new)), _mm_move_ss((src),(new)), 0x39)

# endif
/* - - - - - - - - - - - - - - - - - - - - - - End of the INTEL section - - - - - - - - - - - - - - - - - - - - - - - */



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                   //
//                                                                                                                   //
//                         /^^^^^\                                       /^^^^^\  _-^^^^^^/ TM                       //
//                        /  ,-   )_-----_ --- ---- ----..----.  ---.---/  ,-   )/   ,---/                           //
//                       /  /_)  //  __   )| |/   |/  _// .-   )/    _//  /_)  //   /                                //
//                      /  //   //  / /  /|          / / (/___//   .^ /  //   /(   |                                 //
//                     /  / ^^^'(   (/  / |   /|   _/ (  `----/   /  /  / ^^^' |   `---/                             //
//                    /__/       \____-'  |__/ |__/    \____//___/  /__/        \_____/                              //
//                                                                                                                   //
//                                                                                                                   //
//                Versions of the V4F4 macros for the PowerPC AltiVec 128-bit vector instruction set                 //
//                                                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*

The AltiVec instruction set first came with the "G4" (744x and 745x) processors from Motorola, then the "G5" (97x)
series from IBM, the "Cell" 8-core CPU used in the Sony Playstation 3, and in IBM's POWER6 (and later) server CPUs.

  Not yet implemented -- do we care about AltiVec? */

#endif
