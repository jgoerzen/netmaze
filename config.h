
/* select ONE of the following environments
 * Use SunOS4, if you don't find your system in the list
 */

/* #define HPUX 1  */
#define SunOS4 1
/* #define IRIX 1 */
/* #define Linux 1 */
/* #define RS6000 1 */  /* look at CREDITS */
/* #define NeXT 1   */  /* look at CREDITS */

/* #define USE_SOUND 1 */

/* set the paths, according to your environment: */
#define AUDIOPATH "./lib/netmaze.seq"
#define TEXTUREPATH "./lib/"

/*********************************************************
 * Setup machinedepend configuration
 */

#if (IRIX || SunOS4 || HPUX || Linux || RS6000 || NeXT)
#  define ITIMERVAL
#else
#  undef ITIMERVAL  /* not on all machines available */
#endif

#if (HPUX || NeXT)
#  define USE_SIGVEC
#  if NeXT
#    define sigvector sigvec
#  endif
#else
#  undef USE_SIGVEC
#endif

#if (Linux)
#  define HAVE_FDSET
#  define USE_IPC
#else
#  if (IRIX)
#    define HAVE_FDSET
#  else
#    undef HAVE_FDSET
#  endif
#  undef USE_IPC
#endif


#if (HP_SOUND || SS10_SOUND)
#  define USE_SOUND
#endif


