#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "netmaze.h"

#ifdef RS6000
 #include <sys/select.h>
#endif

#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#ifdef HAVE_FDSET
 fd_set readmask;
 fd_set writemask;
#else
 struct fd_mask readmask;
 struct fd_mask writemask;
#endif

struct timeval notimeout;

static struct hostent *hp;            /* pointer to host info for remote host */
static struct sockaddr_in remoteaddr; /* server connect */

static void io_handler(int);
extern void handle_socket(void);
extern void ident_player(void);
extern int init_audio(void);
extern void kick_audio(void);
int own_socket,max_fd=0;
static int audio;

int init_net(char *hostname)
{
  int  ret;
#ifdef USE_SIGVEC
  struct sigvec vec;
#else
  struct sigaction vec;
#endif

#ifndef USE_IPC
  int flag = 1;
#endif

  notimeout.tv_sec = 0;
  notimeout.tv_usec = 0;

#ifdef USE_SIGVEC
  vec.sv_handler = (void (*)) io_handler;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  if ( sigvector(SIGIO, &vec, (struct sigvec *) 0) == -1)
    perror(" sigaction(SIGIO)");
#else
  vec.sa_handler = (void (*)) io_handler;
 #if defined(RS6000) || defined(__linux__) /* ibm rs/6000 */
   sigemptyset(&vec.sa_mask);
 #else
  vec.sa_mask = 0;
 #endif
  vec.sa_flags = 0;
  if ( sigaction(SIGIO, &vec, (struct sigaction *) 0) == -1)
    perror(" sigaction(SIGIO)");
#endif
  remoteaddr.sin_family = AF_INET;

  if ((hp = gethostbyname(hostname) ) == NULL)
  {
    fprintf(stderr, "netmaze: can't resolve hostname: %s.\n",hostname);
    exit(1);
  }

  remoteaddr.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

  if((own_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    fprintf(stderr, "netmaze: unable to create socket.\n"); exit(1);
  }

  remoteaddr.sin_port = htons(NETMAZEPORT);
  FD_ZERO(&readmask);
  FD_ZERO(&writemask);
  FD_SET(own_socket,&readmask);
  max_fd = own_socket+1;

#ifdef USE_SOUND
  audio = init_audio();
  if(audio >= 0)
  {
    FD_SET(audio,&writemask);
    if(audio > max_fd-1)
      max_fd = audio+1;
  }
#endif

#ifdef USE_IPC
  FD_SET(0,&readmask); /* pipe! */
#else
  if (ioctl(own_socket, FIOASYNC, &flag) == -1)
  {
    perror("socket: ioctl(FIOASYNC)"); exit(1);
  }
#endif

  if ( (ret=connect(own_socket,(struct sockaddr *) &remoteaddr, sizeof(struct sockaddr))) == -1)
  {
     perror("connect"); exit(1);
  }

#ifndef USE_IPC
  flag = -getpid();
  if (ioctl(own_socket, SIOCSPGRP, &flag) == -1)
  {
    perror("process group");
  }
#endif

  printf("Connectet to %s. Connectcode: %d.\n",hostname,ret);
  ident_player();

#ifdef USE_IPC
  for(;;) io_handler(0);
#else
  io_handler(0);
#endif

  return 0; /* all is OK */
}

/**********************************************
 * io-handler (socket/pipes)
 */

static void io_handler(int a)
{
  int	numfds;
  static char	buf[256];

#ifdef HAVE_FDSET
  fd_set	 readmask1;
  fd_set	 writemask1;
#else
  struct fd_mask readmask1;
  struct fd_mask writemask1;
#endif

  for(;;)
  {
    extern int playit;
    readmask1 = readmask;
    if(playit)
      writemask1 = writemask;
    else
      FD_ZERO(&writemask1);

#ifdef USE_IPC
    if((numfds = select(max_fd,&readmask1,&writemask1,NULL,NULL))< 0)
      continue;
#else
    if((numfds = select(max_fd,&readmask1,&writemask1,NULL,&notimeout))<0)
    {
      perror("select failed ");
      exit(2);
    }
#endif

    if(numfds == 0) break;

    if(FD_ISSET(own_socket,&readmask1))
      handle_socket();

#ifdef USE_IPC
    if(FD_ISSET(0,&readmask1))
    {
      if(read(0,buf,256) == 0) /* sure is sure */
      {
        fprintf(stderr,"Parent died!\n"); exit(1);
      }
    }
#endif
    if(audio >= 0)
      if(FD_ISSET(audio,&writemask1))
        kick_audio();
  }
}
