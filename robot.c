/* This is a demonstration on how to write robots.
 * This is the basecode.
 * Write your own: own_action()/init_robot() - functions (see dummy.c)
 * and compile the whole stuff with robot.o,allmove.o,network.o!
 */

#include "netmaze.h"

#include <sys/types.h>

#ifdef RS6000
 #include <sys/select.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <memory.h>

#include "network.h"

#include "trigtab.h"

#define TRUE  1
#define FALSE 0

struct shared_struct shstr;
struct shared_struct *sm = &shstr;

void io_handler(int a);

/* extern: allmove.c */
extern void move_all(PLAYER*,int*);
extern void run_game(MAZE*,PLAYER*);
extern void myrandominit(long);

/* extern: user-defined-functions */
extern int own_action(void);
extern void start_robot(int);
extern int init_robot(void);

/* extern: network.c */
extern struct robot_functions robot;
extern void handle_socket(void);
extern void ident_player(void);

#ifdef HAVE_FDSET
struct fd_set readmask;
#else
struct fd_mask readmask;
#endif

static struct hostent *hp;            /* pointer to host info for remote host */
static struct sockaddr_in remoteaddr; /* server connect */

int own_socket;         /* streams socket descriptor */

int main(int argc,char **argv)
{
  int  ret;
  char hostname[256];

#ifdef USE_SIGVEC
  struct sigvec vec;
#else
  struct sigaction vec;
#endif

  if(argc != 2)
  {
    fprintf(stderr,"%s: usage: %s <serverhostname>\n",argv[0],argv[0]);
    exit(1);
  }
  strcpy(hostname,argv[1]);

  if(!init_robot())
  {
    fprintf(stderr,"%s: Can't initialize the robot.\n",argv[0]);
    exit(1);
  }
  robot.valid = TRUE;
  robot.action = own_action;
  robot.start  = start_robot;

#ifdef USE_SIGVEC
  vec.sv_handler = (void (*)(int)) io_handler;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  if ( sigvector(SIGIO, &vec, (struct sigvec *) 0) == -1)
    perror(" sigaction(SIGIO)");
#else
  vec.sa_handler = (void (*)(int)) io_handler;
 #ifdef RS6000 /* ibm rs/6000 */
   sigemptyset(&vec.sa_mask);
 #else
  vec.sa_mask = 0;
 #endif
  vec.sa_flags = 0;
  if ( sigaction(SIGIO, &vec, (struct sigaction *) 0) == -1)
    perror(" sigaction(SIGIO)");
#endif
  remoteaddr.sin_family = AF_INET;

  sm->sologame = FALSE;

  if ((hp = gethostbyname(hostname) ) == NULL)
  {
    fprintf(stderr, "netmaze: %s not found in /etc/hosts\n",hostname); exit(1);
  }

  remoteaddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

  if((own_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    fprintf(stderr, "netmaze: unable to create socket\n"); exit(1);
  }

  remoteaddr.sin_port = htons(NETMAZEPORT);
  FD_ZERO(&readmask);
  FD_SET(own_socket,&readmask);

  if ( (ret=connect(own_socket,(struct sockaddr *) &remoteaddr, sizeof(struct sockaddr))) == -1)
  {
     perror("connect"); exit(1);
  }

  ident_player();
  printf("Connectet to %s. Connectcode: %d.\n",hostname,ret);

  for(;;) io_handler(0);
}

void io_handler(int a)
{
  int	numfds;

#ifdef HAVE_FDSET
  struct fd_set readmask1;
#else
  struct fd_mask readmask1;
#endif

  for(;;)
  {
    readmask1 = readmask;

    if((numfds = select(32,&readmask1,NULL,NULL,NULL))< 0)
      continue;

    if(numfds == 0) break;

    if(FD_ISSET(own_socket,&readmask1))
    {
      handle_socket();
    }
  }
}


/***********
 * dummy audio-functions
 */

void play_sound(int a) { }

