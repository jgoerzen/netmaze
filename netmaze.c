/**********************************************************************
 *  NETMAZE - The Multiplayercombatgame
 *  -----------------------------------
 *
 *  Version: 0.81 (alpha)
 *  last change: April 1994
 *
 *  written and copyrights (c) by M.Hipp
 *  email:
 *     mhipp@student.uni-tuebingen.de
 *
 * --------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *********************************************************************/

#ifndef NeXT
  #include <malloc.h>
#endif

#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/wait.h>

#include "netmaze.h"

#ifdef USE_IPC
 #include <sys/ipc.h>
 #include <sys/shm.h>
#endif

#ifdef USE_IPC
 struct shared_struct *sm;
#else
 struct shared_struct shared_data;
 struct shared_struct *sm = &shared_data;
#endif

extern void draw_screen(void);
extern void move_all(PLAYER*,int*);

static void usage(FILE *s);
static void init_program(int,char**);
static void start_signal(void),solo_timer(int);
static void setup_struct(void);
#ifdef USE_IPC
static void setup_sigchild(void),child_signal(int);
static int child_pids[4];
#endif

/* extern: x11gfx.c & x11cntrl.c */
extern void x11_init(int,char**);
extern void x11_cntrl(void);
extern void x11_flush(void);
extern void draw_end(PLAYER *play,int nr);
extern void draw_info(void);
extern void draw_status(int,PLAYER*);
extern void draw_kills(int,PLAYER*);

/* extern: network.c */
extern int init_net(char*);

/* extern: maze.c */
extern int create_maze(MAZE*);

#ifdef ITIMERVAL
struct itimerval value,ovalue;
#else
struct itimerstruc_t value,ovalue;
#endif

#ifdef USE_SIGVEC
struct sigvec vec,ovec,vecio,ovecio;
#else
struct sigaction vec,ovec,vecio,ovecio;
#endif

extern struct timeval notimeout;
extern struct fd_mask readmask;

void main(int argc,char **argv)
{
  int i,nowait=FALSE;

  setup_struct();

  if(argc != 1)
  {
    for(i=1;i<argc;i++)
    {
      if(strcmp(argv[i],"-map") == 0) sm->mapdraw = TRUE;
      else if((strcmp(argv[i],"-server") == 0) || (strcmp(argv[i],"-s") == 0))
      {
        i++;
        sm->sologame = FALSE;
        if(strlen(argv[i]) > 255)
        {
          fprintf(stderr,"Hostname too long!!\n");
          exit(1);
        }
        strcpy(sm->hostname,argv[i]);
      }
      else if((strcmp(argv[i],"-help") == 0) || (strcmp(argv[i],"-h") == 0))
      {
        usage(stdout);
        exit(0);
      }
      else if(strcmp(argv[i],"-name") == 0)
      {
        i++;
        if(strlen(argv[i]) >= MAXNAME)
        {
          fprintf(stderr,"Name too long. Maximum is %d character.\n",MAXNAME-1);
          exit(1);
        }
        strcpy(sm->ownname,argv[i]);
      }
      else if(strcmp(argv[i],"-tiny") == 0)
        sm->outputsize = 14;
      else if(strcmp(argv[i],"-small") == 0)
        sm->outputsize = 13;
      else if(strcmp(argv[i],"-huge") == 0)
        sm->outputsize = 11;
      else if(strcmp(argv[i],"-big") == 0)
        sm->outputsize = -1;
      else if(strcmp(argv[i],"-mono") == 0)
        sm->monomode = TRUE;
      else if((strcmp(argv[i],"-gray") == 0) || (strcmp(argv[i],"-grey") == 0))
        sm->graymode = TRUE;
      else if(strcmp(argv[i],"-dither") == 0)
        sm->dithermode = TRUE;
      else if(strcmp(argv[i],"-nowait") == 0)
        nowait = TRUE;
      else if(strcmp(argv[i],"-camera") == 0)
        sm->camera = TRUE;
      else if(strcmp(argv[i],"-direct") == 0)
        sm->directdraw = TRUE;
      else if(strcmp(argv[i],"-sound") == 0)
        sm->usesound = TRUE;
      else if(!strcmp(argv[i],"-privatecmap"))
        sm->privatecmap = TRUE;
      else if(!strcmp(argv[i],"-noshmem"))
        sm->noshmem = TRUE;
      else if(!strcmp(argv[i],"-texture"))
      {
        sm->texturemode = TRUE;
        sm->privatecmap = TRUE;
      }
      else if(strcmp(argv[i],"-comment") == 0)
      {
        i++;
        if(strlen(argv[i]) >= MAXCOMMENT)
        {
          fprintf(stderr,"Comment too long. Maximum is %d character.\n",MAXCOMMENT-1);
          exit(1);
        }
        strcpy(sm->owncomment,argv[i]);
      }
      else
      {
        usage(stderr);
        exit(1);
      }
    }
  }
#ifndef USE_SOUND
  if(sm->usesound)
    fprintf(stderr,"You haven't compiled netmaze with USE_SOUND.\n");
#endif

  if(sm->sologame && sm->usesound)
  {
    sm->usesound = FALSE;
    fprintf(stderr,"It's not possible to use sound in the solo testmode.\n");
  }

  if( (sm->monomode + sm->graymode + sm->dithermode) > 2 )
  {
    fprintf(stderr,"Sorry .. only -mono OR -gray OR -dither is allowed\n");
    exit(1);
  }
  if( sm->texturemode && (sm->monomode || sm->graymode || sm->dithermode))
  {
    fprintf(stderr,"Warning: Textures only usefull in colormode yet.\n");
  }

    /* init Default-Maze */
  create_maze(&(sm->std_maze));
    /* Signals,Gfx,Network,(IPC:fork),keyboard initialisieren */
  init_program(argc,argv);

  if(sm->sologame)
  {
    printf("Press '1' to start game!\n");
  }
  else
  {
    printf("OK .. now wait for serverstart\n");
  }

  /**** beginning of the mainloop (endless) ****/

  while(!sm->exitprg)
  {
    int waitselect;

#ifdef USE_IPC
   if(sm->sologame) x11_cntrl();
#else
   x11_cntrl();
#endif

    if(nowait)
      waitselect = FALSE;
    else
      waitselect = TRUE;

    if(sm->bgdraw)
    {
      draw_info();
      sm->bgdraw = FALSE;
      x11_flush();
    }

    if((sm->screendraw) && (sm->redraw))
    {
      if(sm->drawwait >= DRAWWAIT)
      {
        sm->redraw = FALSE;
        sm->drawwait = 0;
        draw_screen();
        waitselect = FALSE;
      }
    }

    if((sm->winnerdraw) && (sm->redraw))
    {
      sm->redraw = FALSE;
      draw_end(sm->playfeld,sm->winner); /* does XSync() */
    }

    if((sm->statchg2) && (!sm->gameflag)) /*update Screen after the Gameover*/
    {
      sm->statchg  = TRUE;
      sm->statchg2 = FALSE;
      draw_status(-1,sm->playfeld);
      x11_flush();
    }

    if((sm->killchg) && (!sm->gameflag)) /*update Screen after the Gameover*/
    {
      draw_kills(sm->shownumber,sm->playfeld);
      x11_flush();
    }

    if(waitselect) /* test! only a timeout-wait yet */
    {              /* this reduces the load enormous :) */
      struct timeval timeout;
#ifdef USE_IPC
      if(sm->gameflag)
      {
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000; /* lower than the minval on most machines */
      }
      else
      {
#endif
        timeout.tv_sec = 0;         /* in NON-IPC versions, the SIGIO-Signal */
        timeout.tv_usec = 100000;   /* also exits the select-command */
#ifdef USE_IPC
      }
#endif
      select(0,NULL,NULL,NULL,&timeout);
    }
  }

  XCloseDisplay(sm->grafix.display);
}

/***************************/
/* Programm init           */
/***************************/

static void init_program(int argc,char **argv)
{
  x11_init(argc,argv);

  if(sm->sologame)
    start_signal();
  else
  {
#ifdef USE_IPC
    int fd[2];

    setup_sigchild();
    if(pipe(fd) < 0)
    {
       perror("pipe"); exit(1);
    }

    if( (child_pids[0] = fork()) == 0)
    {
      close(fd[1]);
      if( (child_pids[1] = fork()) == 0)
      {
                        /* we use a pipe to check if the daddy is dead: */
        dup2(fd[0],0);  /* I'm no unix-guru, so if you know a better way, */
        close(fd[0]);   /* pls. let me know (perhaps with a processgroup */
        init_net(sm->hostname); /* <- should be neverending */
        exit(1);                /* <- but sure is sure :-) */
      }
      close(fd[0]);
      x11_cntrl();
      exit(1);
    }
    if(child_pids[0] < 0)
    {
      perror("fork");
      exit(1);
    }
    close(fd[0]);
#else
    if(init_net(sm->hostname) < 0)
    {
      fprintf(stderr,"nm: Networkproblems\n");
      exit(1);
    }
#endif

  }
}


/*********************************
 * Setup Signals
 */

static void start_signal(void)
{
#ifdef USE_SIGVEC
  struct sigvec vec;
#else
  struct sigaction vec;
#endif

  value.it_value.tv_sec =  1;
  value.it_interval.tv_sec = 0;
#ifdef ITIMERVAL
  value.it_value.tv_usec = 0;
  value.it_interval.tv_usec = DRAWTIME;
#else
  value.it_value.tv_nsec = 0;
  value.it_interval.tv_nsec = DRAWTIME;
#endif

  setitimer(ITIMER_REAL,&value,&ovalue);

#ifdef USE_SIGVEC
  vec.sv_handler = (void (*)(int)) solo_timer;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  if ( sigvector(SIGALRM, &vec, &ovec) == -1) perror("SIGALRM\n");
#else
  vec.sa_handler = (void (*)(int)) solo_timer;
 #ifdef RS6000 /* ibm rs/6000 */
   sigemptyset(&vec.sa_mask);
 #else
   vec.sa_mask = 0;
 #endif
  vec.sa_flags = 0;
  if ( sigaction(SIGALRM, &vec, &ovec) == -1) perror("SIGALRM\n");
#endif

}

/*********************************++
 * SIG-CHILD
 */

#ifdef USE_IPC
static void setup_sigchild(void)
{
#ifdef USE_SIGVEC
  struct sigvec vec;

  vec.sv_handler = (void (*)(int)) child_signal;
  vec.sv_mask = 0;
  vec.sv_flags = 0;
  if ( sigvector(SIGCHLD, &vec, NULL) == -1) perror("SIGCHLD\n");
#else
  struct sigaction vec;

  vec.sa_handler = (void (*)(int)) child_signal;
 #ifdef RS6000 /* ibm rs/6000 */
   sigemptyset(&vec.sa_mask);
 #else
  vec.sa_mask = 0;
 #endif
  vec.sa_flags = 0;
  if ( sigaction(SIGCHLD, &vec, NULL) == -1) perror("SIGCHLD\n");
#endif
}

static void child_signal(int sig)
{
  int pid;
  int status=0;

  pid = wait(&status);
  fprintf(stderr,"sigchild: pid: %d status: %d\n",pid,status);

  exit(1);
}
#endif

/************************************
 * Solomode Timerfunction
 */

static void solo_timer(int sig)
{
  if((sm->gameflag) && (sm->sologame))
  {
     sm->sticks[sm->joynumber] = sm->ownstick;  /* TEST */
     move_all(sm->playfeld,sm->sticks);
  }
}

/******************************************
 * Setup Shared-Struct
 */

static void setup_struct(void)
{
  char *s;

#ifdef USE_IPC
  int shmid;

  if((shmid = shmget (IPC_PRIVATE,(sizeof(struct shared_struct)+0xfff)&0xfffff000, 01600)) < 0) /* we allocate whole 4K buffs */
  {
    perror ("shmget");
    exit(1);
  }
  sm = (struct shared_struct *) shmat(shmid,NULL,0);
  if(sm == (struct shared_struct *) -1)
  {
    perror("shmat");
    exit(1);
  }
  memset(sm,0,sizeof(struct shared_struct));
  sm->shmid = shmid;
#else
  memset(sm,0,sizeof(struct shared_struct));
#endif

  sm->gameflag   = FALSE;
  sm->winner     = 0;
  sm->sologame   = TRUE;

  sm->mapdraw    = FALSE;
  sm->debug      = FALSE;

  sm->anzplayers = sm->numteams = 8;
  sm->shownumber = sm->shownumbertmp = 0;
  sm->joynumber  = 0;

  sm->outputsize = 12;
  sm->monomode   = FALSE;
  sm->graymode   = FALSE;

  sm->drawwait   = 0;

  sm->statchg    = FALSE;
  sm->exitprg    = FALSE;
  sm->bgdraw     = TRUE;
  sm->redraw     = FALSE;
  sm->screendraw = FALSE;
  sm->winnerdraw = FALSE;
  sm->newmap     = FALSE;
  sm->locator    = FALSE;

#ifdef SH_MEM
  sm->noshmem    = FALSE;
#else
  sm->noshmem    = TRUE;
#endif

  if((s=getenv("NETMAZE_NAME"))!=NULL)
  {
    if(strlen(s) > 15)
      fprintf(stderr,"NETMAZE_NAME too long!\n");
    else
      strcpy(sm->ownname,s);
  }
  else
    strcpy(sm->ownname,"Mr. No-Name");
  if((s=getenv("NETMAZE_COMMENT"))!=NULL)
  {
    if(strlen(s) > 31)
      fprintf(stderr,"NETMAZE_COMMENT too long!\n");
    else
      strcpy(sm->owncomment,s);
  }
  else
    strcpy(sm->owncomment,"Have a nice day!");
}

static void usage(FILE *s)
{
  fprintf(s,"Usage: netmaze [ -s| -server servername] [ -h | -help ]  [-name <combatname>]\n");
  fprintf(s,"       [-tiny|-small|-huge|-big] [-mono|-gray|-dither] [-map] [-camera]\n");
  fprintf(s,"       [-comment <comment>] [-nowait] [-sound] [-texture]\n");
  fprintf(s,"  tiny/small/huge: Select a screensize.\n");
  fprintf(s,"  server:          Hostname on which the netserv runs.\n");
  fprintf(s,"  name:            Sets the playername.\n");
  fprintf(s,"  comment:         Sets the comment (if you kill someone).\n");
  fprintf(s,"  map:             Enables the rotatemap.\n");
  fprintf(s,"  mono:            Selects the Mono-Mode (on colourterminals).\n");
  fprintf(s,"  grey/gray:       Selects the Greyscale-Mode.\n");
  fprintf(s,"  dither:          Selects the (Smiley)-Dither-Mode.\n");
  fprintf(s,"  nowait:          Disable the select-timeout (test).\n");
  fprintf(s,"  camera:          Connect server in cameramode.\n");
  fprintf(s,"  texture:         Enable texture mode (needs at least 8bit colour).\n");
  fprintf(s,"  sound:           Enable sound (if compiled with USE_SOUND).\n\n");
  fprintf(s,"Important keys for the game:\n");
  fprintf(s," Cursor Up/Down/Left/Right: Move Smiley.\n");
  fprintf(s," Shift+(Left/Right): Slow Smileyrotate .\n");
  fprintf(s," Space: Shoot.\n");
  fprintf(s," l: Enable/Disable locator.\n");
  fprintf(s," c: Makes your smiley invisible (if allowed by server).\n");
  fprintf(s," m: Enable/Disable newstylemap (with radar).\n");
  fprintf(s," return: Enable/Disable radar for newstylemap (if allowed by server).\n");
  fprintf(s," p: Switch view.\n");
  fprintf(s," j: Switch joystick (only solomode).\n");
  fprintf(s," Q: Quit game.\n");
}
