/*
  dicek.h                                                                                                ^u 120 ^x f

This provides macros for multi-threaded programming. The intention is to allow a single program source to be compiled
for Windows, Linux or Mac OS, and accomplish multi-threading without having to have a lot of #ifdefs around the
OS-specific code.

REVISION HISTORY
 20110929 First version of simple DICEK_SPLIT_MERGE function. This is a blocking, one-time N-way parallel subroutine
call (no provision for threads to continue through multiple synchronization barriers). Works in DICEK_USE_THREADS and
DICEK_EMULATE modes (tested with math3000.cxx A094358() routine).
 20110930 Add a first (extremely speculative and untested) shot at the Windows implementation, currently protected by a
fall-back #ifdef block at the beginning that checks for Windows and reverts to EMULATE mode, pending testing by a real
Windows programmer.
 20111001 Add the thread interlock macros and get them working in EMULATE and POSIX modes.
 20111009 Initialize mutexes in Win32 version of DICEK_SPLIT_1
 20111011 Use semaphores instead of mutexes in Win32 version.

*/

/* First we test for each of the known operating system environments */

#if (defined(__linux__) || defined(__APPLE__))
# ifndef DICEK_USE_POSIX
#   ifndef DICEK_EMULATE
#     define DICEK_USE_POSIX
#   endif
# endif
#endif


#ifdef _WIN32
# ifndef DICEK_EMULATE
#   define DICEK_USE_PROCESS_H
# endif
#else
# define __stdcall
#endif


#ifndef DICEK_USE_PROCESS_H
# ifndef DICEK_USE_POSIX
#   ifndef DICEK_EMULATE
#     define DICEK_EMULATE
#   endif
# endif
#endif



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                   //
//  #include<stdio.h>                                                                                                //
//  /*                                                   xx*/                    /*                             */   //
//  /*                                                     */                    /*                             */   //
//  int                 main()  {char      a[]     ={     'T',    'h','i',     's',32,     'p','r'        ,'o','g'   //
//  ,'r','a','m',      32,   'j',   'u',   's',    't'    ,32           ,'d',    'o',   'e'     ,'s'    ,32    ,'i'  //
//  ,'t'              ,32    ,'t'    ,'h'  ,'e'   ,32,    'h'    ,97,'r','d'   ,32,    'w',97,121,33,   32,    40,   //
//  68,               'o',   'n',    39,   't'    ,32     ,'y'   ,'o',   117,    32    ,'t'             ,'h'   ,'i'  //
//  ,'n'               /*     Xy     =a     +3     +n      ++     ;a=     b-     (*     x/z              );     if   //
//  (Xy-++n<(z+*x))z  =b;a   +b,     z+=     x*/,107 ,    63,63    ,63,41,'\n'   ,00};    puts(a);}      /*.RPM.*/   //
//                                                                                                                   //
//        Emulated versions of the DICEK macros. (These also serve as documentation for what each macro does)        //
//                                                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DICEK_EMULATE

/*
 We cannot do inter-thread communication because we execute each "spawned thread" to completion before starting the next
one. Therefore, the first time any child thread tries to DICEK_INTERLOCK, the program will deadlock because there is no
way for the parent to get to the DICEK_CH_BEGIN until the child has exited.
 */
#define DICEK_SUPPORTS_BLOCKING 0

/*
DICEK_THREAD_VARS defines variables which need to be included in the parameter block of any subroutine called by
DICEK_SPLIT_MERGE
  Since this is the emulated version, there are no master_wkg and child_wkg semaphores.
 */
#define DICEK_THREAD_VARS \
  long DICEK_tnum; \
  void * DICEK_thread; \
  void * DICEK_return;

/*
 DICEK_INIT_NTHR declares an integer-type variable named "nth" and initializes it to the number of hardware threads
supported by this system. Place it in your main() or a function from which threads will be launched.
  If you want your threads to have access to the value of nth, you should do "glob = nth;" right after the
DICEK_INIT_NTHR(), where "glob" is a suitably scoped global variable.

In emulated mode we return 3 as the number of "hardware threads", partly because 3 is a fairly rare value in real
computers (the Athlon X3 being about the only one anyone will have) and therefore if you find that DICEK is running 3
"threads" then you know it probably failed to auto-detect the environment.
  Also, as of this writing (2011) 3 threads is about the average number of hardware threads across all portable and
desktop computers that are out there. (Anything by Intel with the "Core i3" or higher brand has 4 threads, and most
desktop machines have at least 2 cores and 4 threads as well)
   However, since this is the emulated-mode version of the macros, the "threads" are actually going to just run one
after the next.
 */
#define DICEK_INIT_NTHR(nth) int nth = 3;

/*
 Place DICEK_DATA in the variable declarations area of the function containing an DICEK_FORK directive. dtype should be
a struct which contains the DICEK_THREAD_VARS macro, and as many other fields as you want. nth is the number of threads
that you will be creating with DICEK_SPLIT_MERGE.
  It will declare an array aname (dynamically allocated) which will be an array of structs of type dtype, and fill the
DICEK-specific fields with a thread number and a pointer to the pthreads data for each thread that will be used by
DICEK_SPLIT_MERGE.
 */
#define DICEK_DATA(dtype, arrayname, nth) \
    dtype * arrayname; \
    arrayname = (dtype *)malloc(nth * sizeof(dtype)); \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      arrayname[_DICEK_i].DICEK_tnum = _DICEK_i; \
      arrayname[_DICEK_i].DICEK_thread = 0; \
    }

/*
 DICEK_SPLIT_1 starts a thread with the given index; the array "arrayname" should be allocated by DICEK_DATA and should
contain an element at the given index. The thread will call a function called funcname, which should be type

  void(* funcname)(void *)

The instance of funcname will be passed the index'th element of arrayname.
  After the thread has exited, execution proceeds with the instruction following DICEK_SPLIT_1.
 */
#define DICEK_SPLIT_1(funcname, arrayname, index) \
    arrayname[index].DICEK_return = 0; \
    arrayname[index].DICEK_thread = (void *) index; \
    (funcname)((void *) &(arrayname[index]));

#define DICEK_MERGE_1(arrayname, index)  /* nop */

/*
 DICEK_SPLIT_MERGE starts nth threads vis DICEK_SPLIT_1. After the last of the threads has exited, execution proceeds
with the instruction following DICEK_SPLIT_MERGE.
 */
#define DICEK_SPLIT_MERGE(funcname, arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_SPLIT_1((funcname), (arrayname), (_DICEK_i)) \
    }

#define DICEK_SPLIT_M(funcname, arrayname, index) DICEK_SPLIT_1(funcname, arrayname, index)

/* Just start the threads, without a merge. This is used for applications that do inter-thread synchronization */
#define DICEK_SPLIT(funcname, arrayname, nth) DICEK_SPLIT_MERGE(funcname, arrayname, nth)

/*
 Place a DICEK_SUB at the beginning of any routine called by a DICEK_SPLIT_MERGE. dtype is the type of the struct which
is the array elements of arrayname, and argname is the name of the subroutine's argument. For example:

  typedef struct f_vars {
    DICEK_THREAD_VARS
    int foo;
    char * bar;
  } f_vars;

  (void *)myfunc(void * params)
  {
    DICEK_SUB(f_vars, params);
    // other local variable declarations
    ...
  }

For an example of a thread that performs inter-thread synchronization, see the example below (DICEK_CH_BEGIN macro)

In emulated mode, we need a pointer to the struct that was passed in. This pointer is used by DICEK_RETURN
 */
#define DICEK_SUB(dtype, argname) \
  dtype * _DICEK_params = (dtype *) argname;

/*
 The child thread should start with DICEK_CH_BEGIN, which grabs the semaphore indicating that the child is working.
Then it should do its work, followed DICEK_CH_SYNC to tell the other threads in its group that it has finished
its section of work.
 The master thread will eventually release the semaphore used by DICEK_CH_BEGIN allowing the child to continue
with the next piece of work.

  (void *)myfunc(void * params)
  {
    DICEK_SUB(f_vars, params);
    // other local variable declarations
    while(working) {
      DICEK_CH_BEGIN
      // Do computation here
      DICEK_CH_SYNC
    }
    // We're done performing useful work
    DICEK_CH_END
    // Here we deallocate any temporary private storage
    DICEK_RETURN(result)
  }

*/
#define DICEK_CH_BEGIN \
  fprintf(stderr, "Runtime error: DICEK does not support inter-thread communication in EMULATE mode.\n"); \
  exit(-1);

/*
 DICEK_CH_SYNC releases the "master working" semaphore, which tells the master that this thread is done with its
work and the master can proceed to use the results that were produced by the child.
 */
#define DICEK_CH_SYNC /* nop */

/*
  DICEK_INTERLOCK is called by the master thread to wait for the children to be done with their bits of work.
It attempts to take all of the master_working semaphores, which are initially locked and don't get unlocked
until the child threads reach their DICEK_CH_SYNC.
 */
#define DICEK_INTERLOCK(arrayname, nth) DICEK_CH_BEGIN

/*
  DICEK_RESUME is called by the master thread to tell the child threads that they cal now continue with another
piece of work. It unlocks the child_working semaphore so that the children can execute their DICEK_CH_BEGIN
statement a second time.
 */
#define DICEK_RESUME(arrayname, nth) /* nop */

/*
 Put DICEK_CH_END at the point where a child thread has finished performing any actions that require
cooperation or synchronization, and before the process ends. This macro releases the child_working semaphore
so that the semaphore can be deallocated by the master process.
 The typical sequence is: DICEK_CH_END; deallocate private storage; DICEK_RETURN; exit. See the DICEK_CH_BEGIN
block comment for a pseudo-code example.
 */
#define DICEK_CH_END /* nop */

/*
 Place a DICEK_RETURN at the very end of any routine called by a DICEK_SPLIT_MERGE. It passes returnv back
to the calling (master) thread via a field routine's parameter block.
 WARNING: On some operating systems, this return value is passed through the kernel, while on others it is actually
written directly into the parameter block. On some systems, this routine causes the thread to exit immediately, while on
others it is possible to execute statements after the DICEK_RETURN. Users of this package should not rely on any of
these specifics to be true in runtime.
 */
#define DICEK_RETURN(returnv) _DICEK_params->DICEK_return = (void*)(returnv);

#define DICEK_MERGE_M(arrayname, index)  /* nop */

#define DICEK_MERGE(funcname, arrayname, nth) /* nop */

#endif

/* - - - - - - - - - - - - - - - - - - - - - End of the EMULATED section  - - - - - - - - - - - - - - - - - - - - - - */


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                ,        ,         //
//                                                                                               /(        )`        //
//                                                                                               \ \___   / |        //
//         _nnnn_                                                                                /- _  `-/  '        //
//        dGGGGMMb                                                                              (/\/ \ \   /\        //
//       @p~qp~~qMb                                                                             / /   | `    \       //
//       M|@||@) M|       ooooooooo.     .oooooo.    .oooooo..o ooooo ooooooo  ooooo            O O   ) /    |       //
//       @,----.JM|       `888   `Y88.  d8P'  `Y8b  d8P'    `Y8 `888'  `8888    d8'             `-^--'`<     '       //
//      JS^\__/  qKL       888   .d88' 888      888 Y88bo.       888     Y888..8P              (_.)  _  )   /        //
//     dZP        qKRb     888ooo88P'  888      888  `"Y8888o.   888      `8888'                `.___/`    /         //
//    dZP          qKKb    888         888      888      `"Y88b  888     .8PY888.                 `-----' /          //
//   fZP            SMMb   888         `88b    d88' oo     .d8P  888    d8'  `888b   <----.     __ / __   \          //
//   HZM            MMMM  o888o         `Y8bood8P'  8""88888P'  o888o o888o  o88888o <----|====O)))==) \) /====      //
//   FqM            MMMM                                                             <----'    `--' `.__,' \         //
// __| ".        |\dS"qML                                                                         |        |         //
// |    `.       | `' \Zq                                                                          \       /       /\//
//_)      \.___.,|     .'                                                                     ______( (_  / \______/ //
//\____   )MMMMMP|   .'           Versions of the DICEK macros for Linux, Mac OS,           ,'  ,-----'   |          //
//     `-'       `--' hjm             and other POSIX-compliant environments                `--{__________)          //
//                                                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DICEK_USE_POSIX

#define DICEK_SUPPORTS_BLOCKING 1

#include <pthread.h>

// Mach-based system (most notably Apple) use sysctlbyname to query the number of CPUs/cores/threads
# ifdef __MACH__
#   include <sys/types.h>
#   include <sys/sysctl.h>
# endif

// TODO: Some Linux systems might need an #include to access the mechanism for finding out the number of threads.


#define DICEK_THREAD_VARS \
  long DICEK_tnum; \
  void * DICEK_thread; \
  pthread_mutex_t DICEK_master_wkg; \
  pthread_mutex_t DICEK_child_wkg; \
  void * DICEK_return;


#ifdef __APPLE__
# define DICEK_INIT_NTHR(nth) int nth; { \
    size_t _DICEK_sz_in = sizeof(nth); \
    long _DICEK_rv2 = sysctlbyname("hw.ncpu", (void *) (&nth), &_DICEK_sz_in, 0, 0); \
    if (nth <= 0) { nth = 1; } \
    if (nth > 64) { nth = 64; } }
#else
// TODO: How to query number of threads in Linux (I suspect reading /proc/cpuinfo will work on most,
// but not all, Linuces)
# define DICEK_INIT_NTHR(nth) int nth = 3;
#endif

#define DICEK_DATA(dtype, arrayname, nth) \
    dtype * arrayname; \
    arrayname = (dtype *)malloc(nth * sizeof(dtype)); \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      arrayname[_DICEK_i].DICEK_tnum = _DICEK_i; \
      arrayname[_DICEK_i].DICEK_thread = 0; \
      arrayname[_DICEK_i].DICEK_return = 0; \
    }

/* Just start a single thread. */
#define DICEK_SPLIT_1(funcname, arrayname, index) \
    arrayname[index].DICEK_return = 0; \
    pthread_create((pthread_t *) (&(arrayname[index].DICEK_thread)), \
        0, (void* (*)(void*))funcname, (void *) &(arrayname[index]));

/* End a single thread. */
#define DICEK_MERGE_1(arrayname, index) \
    pthread_join((pthread_t) (arrayname[index].DICEK_thread), \
        &(arrayname[index].DICEK_return));

/* In the POSIX implementation, semaphore communication is supported, so the SPLIT_MERGE function
(which by definition has no communication between master and spawned threads) does not need to
initialize the semaphores, whereas DICEK_SPLIT_1 does. */
#define DICEK_SPLIT_MERGE(funcname, arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_SPLIT_1(funcname, arrayname, _DICEK_i) \
    } \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_MERGE_1(arrayname, _DICEK_i) \
    }

/* DICEK_SPLIT_M also sets up the semaphores in case the client is going to want to do that. */
#define DICEK_SPLIT_M(funcname, arrayname, index) \
    pthread_mutex_init(&(arrayname[index].DICEK_child_wkg), 0); \
    pthread_mutex_init(&(arrayname[index].DICEK_master_wkg), 0); \
    pthread_mutex_trylock(&(arrayname[index].DICEK_master_wkg)); \
    DICEK_SPLIT_1(funcname, arrayname, index)

/* Just start the threads, without a merge. This is used for applications that do inter-thread synchronization */
#define DICEK_SPLIT(funcname, arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_SPLIT_M((void* (*)(void*))funcname, (arrayname), _DICEK_i) \
    }

#define DICEK_SUB(dtype, argname) dtype * _DICEK_params = (dtype *) argname;

#define DICEK_CH_BEGIN \
  pthread_mutex_lock(&(_DICEK_params->DICEK_child_wkg));

#define DICEK_CH_SYNC \
  pthread_mutex_unlock(&(_DICEK_params->DICEK_master_wkg));

#define DICEK_INTERLOCK(arrayname, nth) \
  for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
    pthread_mutex_lock(&(arrayname[_DICEK_i].DICEK_master_wkg)); \
  }

#define DICEK_RESUME(arrayname, nth) \
  for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
    pthread_mutex_unlock(&(arrayname[_DICEK_i].DICEK_child_wkg)); \
  }

#define DICEK_CH_END \
  pthread_mutex_unlock(&(_DICEK_params->DICEK_child_wkg));

#define DICEK_RETURN(returnv) pthread_exit((void*)(returnv));

#define DICEK_MERGE_M(arrayname, index) \
    DICEK_MERGE_1(arrayname, index) \
    pthread_mutex_destroy(&(arrayname[index].DICEK_child_wkg)); \
    pthread_mutex_unlock(&(arrayname[index].DICEK_master_wkg)); \
    pthread_mutex_destroy(&(arrayname[index].DICEK_master_wkg));

#define DICEK_MERGE(funcname, arrayname, nth) \
  for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
    DICEK_MERGE_M((arrayname), _DICEK_i) \
  }

#endif

/* - - - - - - - - - - - - - - - - - - - - - - End of the POSIX section - - - - - - - - - - - - - - - - - - - - - - - */


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                   //
//       _-^^^^-_                                  ,,                    ,,                                          //
//      /      './^-____-^/  `7MMF'     A     `7MF'db                  `7MM                                          //
//     /       ./.'      /     `MA     ,MA     ,V                        MM                                          //
//     /      './.'      /      VM:   ,VVM:   ,V `7MM  `7MMpMMMb.   ,M""bMM  ,pW"Wq.`7M'    ,A    `MF',pP"Ybd        //
//    /_-^^^^-_/.'      /        MM.  M' MM.  M'   MM    MM    MM ,AP    MM 6W'   `Wb VA   ,VAA   ,V  8I   `"        //
//    /      './^-____-^/        `MM A'  `MM A'    MM    MM    MM 8MI    MM 8M     M8  VA ,V  VA ,V   `YMMMa.        //
//   /       '/.'      /          :MM;    :MM;     MM    MM    MM `Mb    MM YA.   ,A9   VVV    VVV    L.   I8        //
//   /      .'/.'      /           VF      VF    .JMML..JMML  JMML.`Wbmd"MML.`Ybmd9'     W      W     M9mmmP'        //
//  /_-^^^^-_/.'      /                                                                                              //
//            ^-___rpm           Versions of the DICEK macros for Microsoft Windows                                  //
//                                                                                                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DICEK_USE_PROCESS_H

// This is based on:
//   http://msdn.microsoft.com/en-US/library/kdzttdcb(v=VS.80).aspx  (_beginthread)
//   http://msdn.microsoft.com/en-us/library/ms687032(v=VS.85).aspx  (WaitForSingleObject)
//   http://msdn.microsoft.com/en-us/library/ms685071(v=VS.85).aspx  (ReleaseSemaphore)
//   http://msdn.microsoft.com/en-us/library/ms682438(v=VS.85).aspx  (CreateSemaphore)
//
//   http://msdn.microsoft.com/en-us/library/ms682411(v=VS.85).aspx  (CreateMutex, no longer used)
//   http://msdn.microsoft.com/en-us/library/ms685066(v=VS.85).aspx  (ReleaseMutex, no longer used)   

#define DICEK_SUPPORTS_BLOCKING 1

#include <process.h>
#include <windows.h>

#define DICEK_THREAD_VARS \
  long DICEK_tnum; \
  HANDLE DICEK_thread; \
  HANDLE DICEK_master_wkg; \
  HANDLE DICEK_child_wkg; \
  void * DICEK_return;

#define DICEK_INIT_NTHR(nth) \
    int nth; \
    { SYSTEM_INFO si; GetSystemInfo(&si); nth = si.dwNumberOfProcessors; }

#define DICEK_DATA(dtype, arrayname, nth) \
    dtype * arrayname; \
    arrayname = (dtype *)malloc(nth * sizeof(dtype)); \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      arrayname[_DICEK_i].DICEK_tnum = _DICEK_i; \
      arrayname[_DICEK_i].DICEK_thread = (HANDLE) NULL; \
      arrayname[_DICEK_i].DICEK_return = 0; \
    }

#define DICEK_SPLIT_1(funcname, arrayname, index) \
    arrayname[index].DICEK_return = 0; \
    arrayname[index].DICEK_thread = (HANDLE) _beginthreadex(NULL,0,funcname, \
        (void *) &(arrayname[index]), 0, NULL);

#define DICEK_MERGE_1(arrayname, index) \
    WaitForSingleObject(arrayname[index].DICEK_thread, INFINITE);

#define DICEK_SPLIT_MERGE(funcname, arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_SPLIT_1(funcname, arrayname, _DICEK_i) \
    } \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_MERGE_1(arrayname, _DICEK_i) \
    }

// DICEK_SPLIT_M also sets up the semaphores in case the client is going to want to do that.
// child semaphore is initially free (value 1); master semaphore is initially taken
#define DICEK_SPLIT_M(funcname, arrayname, index) \
    arrayname[index].DICEK_child_wkg = CreateSemaphore(NULL, 1, 1, NULL); \
    arrayname[index].DICEK_master_wkg = CreateSemaphore(NULL, 0, 1, NULL); \
    DICEK_SPLIT_1(funcname, arrayname, index)

/* Just start the threads, without a merge. This is used for applications that do inter-thread synchronization */
#define DICEK_SPLIT(funcname, arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_SPLIT_M((funcname), (arrayname), (_DICEK_i)) \
    }

/* Save a pointer to the param block for use by DICEK_RETURN */
#define DICEK_SUB(dtype, argname) dtype * _DICEK_params = (dtype *) argname;

#define DICEK_CH_BEGIN \
    WaitForSingleObject(_DICEK_params->DICEK_child_wkg,INFINITE);

#define DICEK_CH_SYNC \
    ReleaseSemaphore(_DICEK_params->DICEK_master_wkg, 1, NULL);

#define DICEK_INTERLOCK(arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
        WaitForSingleObject(arrayname[_DICEK_i].DICEK_master_wkg,INFINITE); \
    }

#define DICEK_RESUME(arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      ReleaseSemaphore(arrayname[_DICEK_i].DICEK_child_wkg, 1, NULL); \
    }

#define DICEK_CH_END \
    ReleaseSemaphore(_DICEK_params->DICEK_child_wkg, 1, NULL);

#define DICEK_RETURN(returnv) \
    _DICEK_params->DICEK_return = (void*)(returnv); \
    _endthread();

#define DICEK_MERGE_M(arrayname, index) \
    DICEK_MERGE_1(arrayname, index) \
    CloseHandle(arrayname[index].DICEK_thread); \
    CloseHandle(arrayname[index].DICEK_child_wkg); \
    ReleaseSemaphore(arrayname[index].DICEK_master_wkg, 1, NULL); \
    CloseHandle(arrayname[index].DICEK_master_wkg);

#define DICEK_MERGE(funcname, arrayname, nth) \
    for(int _DICEK_i=0; _DICEK_i<nth; _DICEK_i++) { \
      DICEK_MERGE_M((arrayname), _DICEK_i) \
    }

#endif

/* - - - - - - - - - - - - - - - - - - - - - End of the PROCESS_H section - - - - - - - - - - - - - - - - - - - - - - */
