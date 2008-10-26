/* This is the Netserv V0.36-alpha. (March 1994)
 * ------------------------------------------------
 * written & copyrights (c) by M.Hipp
 *  It's hard to understand the code, (also for me)
 *  because the server is grown enormous.
 *
 * A lot of machines are having problems if more than one application wants
 * to use the itimer. In this case the signal is delivered delayed
 * and the game slows down extremly.
 *
 * Subserv: I have removed all of the subserv code, because the concept
 *          was braindead.
 *
 * still in progress:
 *   The rewriting of the whole servercode!
 *
 */

#include "netmaze.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef RS6000
 #include <sys/select.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <memory.h>

#include "netcmds.h"
#include "network.h"
#include "netserv.h"

struct cqueue *cfirst;
struct gqueue *gfirst;

volatile unsigned int timerticks=0;
unsigned int lasttick=0,starttick=0;

int acc_socket;
int menu_in=0,menu_out=1,use_exmenu=FALSE;
char *exmenu_name;
int nowait=FALSE;

/* extern: maze.c */
extern int create_maze(MAZE *mazeadd);
extern int load_maze(char *name,MAZE *mazeadd);
extern int random_maze(MAZE*,int,int);

void init_slots(int first,int *,struct gqueue *);

void send_message(char *str,struct cqueue *);
void send_names(struct gqueue *);
void send_joydata(struct gqueue *);
void send_maze(struct gqueue *);
void send_start(struct gqueue *);
void start_game(struct gqueue *,int *);
void end_game(struct gqueue *);
void send_comments(struct gqueue *);
void send_comment(struct gqueue *g,struct pqueue *p);
void send_inactive(struct gqueue *g,struct pqueue *p);
void send_mes(char*,struct pqueue*,int type,struct pqueue*,struct gqueue*);

void send_command(struct cqueue *q,int mask,char *data,int len);
void send_group(struct gqueue *g,int mask,char *data,int len);

void usage(void);
void print_menu(void);
void print_timerinfo(unsigned int);
void list_connections(void);

void handle_input(void);
void do_command(struct gqueue *g,struct pqueue *p,char *line);
char *get_hostname(struct cqueue *,char *name);
void setup_sigchild(void);
void inter(int);
void work_input(unsigned char *buf,int len,struct cqueue *);
void do_timer(int);
void start_signal(void);
void io_cntl(void);
void handle_sigchild(int s);
int read_pipe(int socket, char *buf, int maxlen);
int read_buffer(int fd, char *buf, int count);

void accept_socket(void);
void close_timeout_sockets(struct gqueue *);
void remove_player(struct pqueue *p,struct cqueue *c);
struct cqueue *close_socket(struct cqueue *);
struct pqueue *add_player(struct cqueue *c,int,int,char*);
struct gqueue *new_group(int no);
int leave_group(struct pqueue *p);
int join_group(int no,struct pqueue *p);
struct pqueue *find_player_by_cn(struct cqueue *c,int cn);
struct pqueue *find_player(char*);


#ifdef HAVE_FDSET
  fd_set readmask;
#else
  struct fd_mask readmask;
#endif

#ifdef USE_SIGVEC
  struct sigvec vec,vec1;
#else
  struct sigaction vec,vec1;
#endif

#ifdef ITIMERVAL
  struct itimerval value,ovalue;
#else
  struct itimerstruct value,ovalue;
#endif

int use_tclmenu = 0; /* JG HACK */
int suppressXnag = 0;
char tclcommand[200];

void sendcommand(void);

/**************************************
 * main()
 *
 *  - initialize most of the stuff
 *  - spawn ex-menu
 *  - setup acceptsocket
 */

int main(int argc,char **argv)
{
  int i;
  struct sockaddr_in myaddr; 	/* local socket address */

  if(argc != 1)
  {
    for(i=1;i<argc;i++)
    {
      if((strcmp(argv[i],"-help") == 0) || (strcmp(argv[i],"-h") == 0))
        usage();
      else if(strcmp(argv[i],"-exmenu") == 0)
      {
        i++;
        use_exmenu = TRUE;
        exmenu_name = argv[i];
      }
      else if(strcmp(argv[i],"-nowait") == 0)
        nowait = TRUE;
      else if(strcmp(argv[i],"-suppressXnag") == 0)
        suppressXnag = 1;
      else if(strcmp(argv[i],"-tclmenu") == 0)
        use_tclmenu = TRUE;
      else
        usage();
    }
  }

  if(use_exmenu)
  {
    int f,fd1[2],fd2[2];
    char *argv1[] = { "NetservExternalMenu" , NULL };

    pipe(fd1);
    pipe(fd2);
    menu_in = fd1[0];
    menu_out = fd2[1];
    setup_sigchild();

    if((f=fork()) == 0)
    {
      close(0); close(1);
      dup(fd2[0]); dup(fd1[1]);
      close(fd1[0]); close(fd1[1]); close(fd2[0]); close(fd2[1]);
      if(execv(exmenu_name,argv1) < 0)
      {
        fprintf(stderr,"Can't spawn external menu.\n");
        exit(1);
      }
    }
    if(f < 0)
    {
      fprintf(stderr,"Can't fork external menu!\n");
      use_exmenu = FALSE;
    }
  }

  cfirst = NULL;
  new_group(1); /* for now we have only one group */
  if(create_maze(&(gfirst->maze)))
    gfirst->nomaze=FALSE;

  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = htons(NETMAZEPORT);
  if( (acc_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) { perror(argv[0]); exit(1); }

  FD_ZERO(&readmask);
  FD_SET(acc_socket, &readmask);
  if(!use_exmenu)
    FD_SET(0,&readmask); /* stdin */
  else
    FD_SET(menu_in,&readmask); /* external menu */

  if (bind(acc_socket,(struct sockaddr *) &myaddr, sizeof(struct sockaddr)) == -1)
  {
    perror("bind"); exit(1);
  }

  if (listen(acc_socket, 5) == -1 ) { perror(argv[0]); exit(1); }

  start_signal();
  printf(" Netmaze Server 0.36alpha/March 1994 by MH; ");
  printf(" Modified by John Goerzen Oct, 1996 <jgoerzen@complete.org> (.81)\n");
  printf(" Using Port: %d\n",NETMAZEPORT);
  if (!suppressXnag) {
    printf(">>>>>>>>>>>>\a\a CHECK OUT THE NEW xnetserv PROGRAM!\a\n");
    printf(">>>>>>>>>>>> The all new interface to the Netmaze server.\n");
  }
  print_menu();

  for(;;)
    io_cntl();
}

/**********************************
 * handle external menu input
 */

void handle_exinput(void)
{
  int len;
  static char line[256];
  static int cmdlen,bytesleft=0;
  struct gqueue *g=gfirst; /* test */

  if(!bytesleft) /* read a new command */
  {
      if((len = read(menu_in,line,1)) != 1)
      {
        fprintf(stderr,"ExMenuReadError! -> EXIT\n");
        exit(1);
      }
      cmdlen = 1; bytesleft = 0;
      switch((int) line[0])
      {
        case MENU_LOADMAZE:
          bytesleft = read(menu_in,line+1,1);
          cmdlen = bytesleft + 1;
          break;
        case MENU_RUNGAME:
          break;
        case MENU_RUNTEAMGAME:
          break;
        case MENU_STOPGAME:
          break;
        case MENU_LIST:
          break;
        case MENU_DISCONNECT:
          bytesleft = 1;
          cmdlen = 2;
          break;
        case MENU_SETMODE:
          bytesleft = 4;
          cmdlen = 5;
          break;
        case MENU_SETDIVIDER:
          bytesleft = 1;
          cmdlen = 2;
          break;
        case MENU_SETTEAMS:
          bytesleft = 32;
          cmdlen = 33;
          break;
      }
  }
  else
  {
    len = read(menu_in,line+cmdlen-bytesleft,bytesleft);
    bytesleft -= len;
  }

  if(bytesleft == 0) /* execute the command */
    do_command(g,NULL,line);
}

/**********************************
 * execute a command
 */

void print_player(char *m,struct pqueue *p)
{
  if(p == NULL)
    printf("%s",m);
  else
    send_mes(m,NULL,MSG_PLAYER,p,NULL);
}

void do_command(struct gqueue *g,struct pqueue *pl,char *line)
{
  int i;
  struct cqueue *q;
  struct pqueue *p;
  char d[256];

    switch((int) line[0])
    {
      case MENU_LOADMAZE:
        break;
      case MENU_RUNGAME:
        if(g->playing) return;
        if(g->nomaze == TRUE)
        {
          print_player("Load/Reinit a Maze first!\n",pl);
          return;
        }
        if(g->numplayers == 0)
        {
          print_player("No player yet!\nTry again later\n",pl);
          return;
        }
        g->numteams = 0;
        start_game(g,NULL);
        break;
      case MENU_RUNTEAMGAME:
        if(g->playing) return;
        if(g->nomaze == TRUE)
        {
          print_player("Load/Reinit a Maze first!\n",pl);
          return;
        }
        if(g->numplayers == 0)
        {
          print_player("No players yet!\nTry again later\n",pl);
          return;
        }
        g->numteams = 0;
        for(p=g->first;p!=NULL;p=p->gnext)
        {
          if(p->team > g->numteams) g->numteams = p->team;
        }
        g->numteams++;
        start_game(g,NULL);
        break;
      case MENU_STOPGAME:
        end_game(g);
        break;
      case MENU_LIST:
        if(pl == NULL)
          list_connections();
        else
        {
          for(p=g->first,i=0;p!=NULL;p=p->gnext,i++)
          {
            sprintf(d,"%3d | %3d | %16s | %s\n",i,p->team,p->name,p->connection->hostname);
            print_player(d,pl);
          }
        }
        break;
      case MENU_DISCONNECT:
        close(acc_socket);
        if(pl != NULL)
          break;
        i = (int) line[1];
/*        for(p=g->first,i=0;p!=NULL;p=p->gnext,i++)  
	for(p=g->first;p!=NULL;p=p->gnext,i--)
	{
	  if(!i)
	  {
	    break;
	  }
	}
	i = (int) line[1]; */
        for(q=cfirst;q!=NULL;q=q->next,i--)
        {
          if(!i)
          {
            close_socket(q);
            break;
          }
        }
        break;
      case MENU_SETMODE:
        if(line[1] == 2)
          g->gamemode = GM_REFLECTINGSHOTS|GM_DECAYINGSHOTS|GM_MULTIPLESHOTS|
                        GM_WEAKINGSHOTS | GM_ALLOWHIDE | GM_ALLOWRADAR;
        else if(line[1] == 3)
          g->gamemode = GM_REFLECTINGSHOTS|GM_DECAYINGSHOTS|GM_MULTIPLESHOTS |
                        GM_FASTWALKING | GM_FASTRECHARGE | GM_ALLOWRADAR;
        else
          g->gamemode = 0;
        break;
      case MENU_SETDIVIDER:
        g->divider = line[1] - 1;
        if((g->divider < 0 ) || (g->divider > 2)) g->divider = 0;
        break;
      case MENU_SETTEAMS:
        for(p=g->first,i=0;p!=NULL;p=p->gnext,i++)
	
        {
          if((p->team < 0) || (p->team > MAXPLAYERS))
            p->team = 0;
          else
            p->team = (int) line[i+1];   
            
          if((q->players->team < 0) || (q->players->team > MAXPLAYERS))
            q->players->team = 0;
          else
            q->players->team = (int) line[i+1];
        }
        break;
    }
}

/*********************************+
 * handle input (ugly)
 */

void handle_input(void)
{
  char line[256],name[100];
  int len,c=0,a,i,j;
  int teams[MAXPLAYERS];
  char *num;
  struct cqueue *q,*cq;
  struct gqueue *g=gfirst;
  struct pqueue *p;

  memset(line,0,256);
  if (use_exmenu && use_tclmenu) {
    len = read_pipe(menu_in, line, 255);
  } else {
     len = read(0,line,255);
  }
  		/* JG HACK */

  if(len == 0)
  {
    fprintf(stderr,"OOPS: end-of-file??\n");
    exit(255);				/* JG added this line */
    return;
  }

  a = sscanf(line,"%d%s",&c,name);
  if(a == 0)
  {
    printf("Unknown command\n");
    sprintf(tclcommand, "MESSAGE\nUnknown command\n");
    sendcommand();
    print_menu();
    return;
  }
  switch(c)
  {
    case 1:
      if(g->playing > 0) return;
      if(a == 1)
      {
        printf("OK .. re-initalize builtin-maze\n");
        sprintf(tclcommand, "MESSAGE\nOK... re-initialize built-in maze\n");
        sendcommand();
        if(create_maze(&(g->maze)))
          g->nomaze=FALSE;
        else
        {
          printf("Can't init maze\n");
          sprintf(tclcommand, "MESSAGE\nCan't initialize maze.\n");
          sendcommand();
          g->nomaze=TRUE;
        }
      }
      else
      {
        if(sscanf(line,"%d%s%d",&c,name,&j) == 2)
        {
          if(load_maze(name,&(g->maze)) == TRUE)
          {
            g->nomaze=FALSE;
            printf("Maze is OK!\n");
            sprintf(tclcommand, "MESSAGE\nMaze is OK!\n");
            sendcommand();
          }
          else
          {
            g->nomaze=TRUE;
            printf("Can't load '%s'\n",name);
            sprintf(tclcommand, "MESSAGE\nCan't load \"%s\"\n", name);
            sendcommand();
          }
        }
        else
        {
          sscanf(line,"%d%d%d",&c,&i,&j);
          if(random_maze(&(g->maze),i,j)) {
            printf("Randommaze is ok!\n");
            sprintf(tclcommand, "MESSAGE\nRandom maze is ok.\n");
            sendcommand();
          }
          else {
            printf("Error, size too big!\n");
            sprintf(tclcommand, "MESSAGE\nError, size too big.\n");
            sendcommand();
          }
        }
      }
      break;
    case 2:
      if(g->playing) return;
      if(g->nomaze == TRUE)
      {
        printf("Load/Reinit a Maze first!\n");
        sprintf(tclcommand, "MESSAGE\nLoad/reinit a maze first!\n");
        sendcommand();
        return;
      }
      if(g->numplayers == 0)
      {
        printf("No players yet!\nTry again later\n");
        sprintf(tclcommand, "MESSAGE\nNo players net; try again later.\n");
        sendcommand();
        return;
      }
      if(a == 1)
      {
        g->numteams = 0;
        start_game(g,NULL);
      }
      else
      {
        g->numteams = 0;
        for(i=0;i<MAXPLAYERS;i++) teams[i] = 0;
        num = strtok(line,"\n\t ");
        for(i=0;;i++)
        {
          num = strtok(NULL,"\n\t ");
          if(num == NULL) break;
          sscanf(num,"%d",&teams[i]);
          if((teams[i] < 0) || (teams[i] > MAXPLAYERS))
          {
            i = 0;
            break;
          }
        }
        if(i > 0)
        {
          for(g->numteams=0,j=0;j<i;j++)
            if(teams[j] > g->numteams) g->numteams = teams[j];
          g->numteams++;

          if(g->numplayers == 0) {
            printf("No player yet!\nTry again later\n");
            sprintf(tclcommand, "MESSAGE\nNo player yet; try again later.\n");
            sendcommand();
          }
          else
          {
            start_game(g,teams);
          }
        }
        else {
          printf("Illegal team-selection!\n");
          sprintf(tclcommand, "MESSAGE\nIllegal team selection.\n");
          sendcommand();
        }
      }
      break;
    case 3:
      end_game(g);
      break;
    case 4:
      list_connections();
      break;
    case 5:
      if(a == 2)
      {
        sscanf(line,"%d %d",&j,&i);
        for(q=cfirst;q!=NULL;q=q->next,i--)
        {
          if(!i)
          {
            close_socket(q);
            break;
          }
        }
      }
      else {
        printf("Too few parameters.\n");
        sprintf(tclcommand, "MESSAGE\nToo few parameters.\n");
        sendcommand();
      }
      break;

/* JG HACK.... */
      case 21: g->gamemode = (g->gamemode & GM_REFLECTINGSHOTS) ? 
               g->gamemode - GM_REFLECTINGSHOTS : g->gamemode | GM_REFLECTINGSHOTS;
               printf("Reflect (bounce) toggled.\n");
               break;
      case 22: g->gamemode = (g->gamemode & GM_DECAYINGSHOTS) ? 
               g->gamemode - GM_DECAYINGSHOTS : g->gamemode | GM_DECAYINGSHOTS;
               printf("Decay (shots lose power) toggled.\n");
               break;
      case 23: g->gamemode = (g->gamemode & GM_MULTIPLESHOTS) ? 
               g->gamemode - GM_MULTIPLESHOTS : g->gamemode | GM_MULTIPLESHOTS;
               printf("Multishots toggled.\n");
               break;
      case 24: g->gamemode = (g->gamemode & GM_WEAKINGSHOTS) ? 
               g->gamemode - GM_WEAKINGSHOTS : g->gamemode | GM_WEAKINGSHOTS;
               printf("Hurts to shoot toggled..\n");
               break;
      case 25: g->gamemode = (g->gamemode & GM_REPOWERONKILL) ? 
               g->gamemode - GM_REPOWERONKILL : g->gamemode | GM_REPOWERONKILL;
               printf("Full power after killing someone toggled.\n");
               break;
      case 26: g->gamemode = (g->gamemode & GM_FASTRECHARGE) ? 
               g->gamemode - GM_FASTRECHARGE : g->gamemode | GM_FASTRECHARGE;
               printf("Fast recharge toggled.\n");
               break;
      case 27: g->gamemode = (g->gamemode & GM_FASTWALKING) ? 
               g->gamemode - GM_FASTWALKING : g->gamemode | GM_FASTWALKING;
               printf("Fast walking toggled.\n");
               break;
/*      case 28: g->gamemode = (g->gamemode & GM_SHOWGHOST) ? 
               g->gamemode - GM_SHOWGHOST : g->gamemode | GM_SHOWGHOST;
               printf("Show ghost (ghostmode) toggled.\n");
               break;  */
      case 29: g->gamemode = (g->gamemode & GM_ALLOWHIDE) ? 
               g->gamemode - GM_ALLOWHIDE : g->gamemode | GM_ALLOWHIDE;
               printf("Allow hide (cloak) toggled.\n");
               break;
      case 30: g->gamemode = (g->gamemode & GM_ALLOWRADAR) ? 
               g->gamemode - GM_ALLOWRADAR : g->gamemode | GM_ALLOWRADAR;
               printf("Radar toggled.\n");
               break;
/*      case 31: g->gamemode = (g->gamemode & GM_DESTRUCTSHOTS) ? 
               g->gamemode - GM_DESTRUCTSHOTS : g->gamemode | GM_DESTRUCTSHOTS;
               printf("Destruct shots on kill toggled.\n");
               break;  */
      case 32: g->gamemode = (g->gamemode & GM_DECSCORE) ? 
               g->gamemode - GM_DECSCORE : g->gamemode | GM_DECSCORE;
               printf("Decrease score of killed player toggled.\n");
               break;
/*      case 33: g->gamemode = (g->gamemode & GM_RANKSCORE) ? 
               g->gamemode - GM_RANKSCORE : g->gamemode | GM_RANKSCORE;
               printf("Rankingdepend score-bonus toggled.\n");
               break;  */
        case 34: g->gamemode = (g->gamemode & GM_TEAMSHOTHURT) ? 
               g->gamemode - GM_TEAMSHOTHURT : g->gamemode | GM_TEAMSHOTHURT;
               printf("Shots from team members hurt toggled.\n");
               break;
/*    case 6:
      if(a == 1)
      {
        if(g->gamemode == 0)
        {
          g->gamemode = GM_REFLECTINGSHOTS|GM_DECAYINGSHOTS|GM_MULTIPLESHOTS |
                        GM_WEAKINGSHOTS | GM_ALLOWHIDE | GM_ALLOWRADAR;
          printf(" Enhanced gamemode selected!\n");
        }
        else if(!(g->gamemode & GM_FASTWALKING))
        {
          g->gamemode = GM_REFLECTINGSHOTS|GM_DECAYINGSHOTS | GM_MULTIPLESHOTS |
                        GM_FASTWALKING | GM_FASTRECHARGE | GM_ALLOWRADAR;
          printf(" Enhanced just-for-fun gamemode selected!\n");
        }
        else
        {
          g->gamemode = 0;
          printf(" Classic gamemode selected!\n");
        }
      }
      break;
*/
    case 7:
      g->divider++;
      if(g->divider == 3)
        g->divider = 0;
      switch(g->divider)
      {
        case 0:
          printf(" OK. Now, the divider is: 1.\n");
          break;
        case 1:
          printf(" OK. Now, the divider is: 2.\n");
          break;
        case 2:
          printf(" OK. Now, the divider is: 4.\n");
          break;
      }
      break;
    case 9:
      if(g->playing > 0) return;
      for(q=cfirst;q!=NULL;)
      {
        q = close_socket(q);
      }
      exit(0);
      break;
    case 96: /* JG HACK */   /* Will refresh the X interface's playerlist */
            list_connections();
            break;
    case 97: /* JG HACK */
             printf("Sending message: %s\n", (char *)(line+3));
             for(q=cfirst;q!=NULL;q=q->next,i--)
               send_message((char *)(line+3), q);
             break;
    case 98: g->gamemode = 0;
             printf("All options off.\n");
             g->divider = 0;
             printf("Beat divider reset to 1.\n");
             break;
    case 99:
      printf("******** INFO: *********\n\n");
      printf("numplayers: %d response: %d playing: %d gameflag: %d mode: %d.\n",g->numplayers,g->response,g->playing,g->gameflag,g->mode);
      for(p=gfirst->first,i=0;p!=NULL;p=p->gnext,i++)
      {
        printf("G[%d] %s pl: %d joy: %d cn: %d num: %d gno: %d team: %d ch: %d.\n",i,p->name,p->playing,p->joydata,p->cnumber,p->number,p->groupnumber,p->team,p->checked);
      }
      for(cq=cfirst,i=0;cq!=NULL;cq=cq->next,i++)
      {
        if(cq->players != NULL)
          printf("C[%d] %s plnum: %d mode: %d socket: %d response: %d.\n",i,cq->players->name,cq->plnum,cq->mode,cq->socket,cq->response);
        else
          printf("C[%d] %s plnum: %d mode: %d socket: %d response: %d.\n",i,"UNK",cq->plnum,cq->mode,cq->socket,cq->response);
      }
      break;
    default:
      printf("Unknown command\n");
      sprintf(tclcommand, "MESSAGE\nUnknown command\n");
      sendcommand();
      print_menu();
      break;
  }
}

/***********************/
/* Install Mastertimer */
/***********************/

void start_signal(void)
{
  value.it_interval.tv_sec = 0;
  value.it_value.tv_sec =  1;
#ifdef ITIMERVAL
  value.it_value.tv_usec = 0;
  value.it_interval.tv_usec = DRAWTIME;
#else
  value.it_value.tv_nsec = 0;
  value.it_interval.tv_nsec = DRAWTIME;
#endif

  setitimer(ITIMER_REAL,&value,&ovalue);

#ifdef USE_SIGVEC
  vec1.sv_handler = (void (*)(int)) inter;
  vec1.sv_mask = 0;
  vec1.sv_flags = 0;
  if (sigvector(SIGALRM, &vec1, (struct sigvec *) 0) == -1) perror("SIGALRM\n");
#else
  vec1.sa_handler = (void (*)(int)) inter;
 #ifdef RS6000 /* ibm rs/6000 */
   sigemptyset(&vec1.sa_mask);
 #else
  vec1.sa_mask = 0;
 #endif
  vec1.sa_flags = 0;
  if ( sigaction(SIGALRM, &vec1, (struct sigaction *) 0) == -1) perror("SIGALRM\n");
#endif
}

void setup_sigchild(void) /* for external menu */
{
#ifdef USE_SIGVEC
  struct sigvec svec1;

  svec1.sv_handler = (void (*)(int)) handle_sigchild;
  svec1.sv_mask = 0;
  svec1.sv_flags = 0;
  if(sigvector(SIGCHLD, &svec1, (struct sigvec *) 0) == -1) perror("SIGCHLD\n");
#else
  struct sigaction svec1;

  svec1.sa_handler = (void (*)(int)) handle_sigchild;
 #ifdef RS6000 /* ibm rs/6000 */
   sigemptyset(&svec1.sa_mask);
 #else
  svec1.sa_mask = 0;
 #endif
  svec1.sa_flags = 0;
  if(sigaction(SIGCHLD,&svec1,(struct sigaction *)0) == -1) perror("SIGCHLD\n");
#endif
}

void handle_sigchild(int s)
{
  int stat,cpid;
  cpid = wait(&stat);
  fprintf(stderr,"Child (exmenu?): %d died.\n",cpid);
  exit(1); /* ok, that's the hard way */
}

/************************/
/* Timer-Signal-Handler */
/************************/

void inter(int sig)
{
  timerticks++; /* enough for about 6 years :-) */

  /* the timertick is for synchronisation */
  /* the main timer-function is called by the main loop */
  /* so we don't have to care about (very tricky) interrupt conditions. */
}

/********************************
 * the main timerfunction
 */

void do_timer(int nowaitgroup)
{
  struct gqueue *g;
  struct pqueue *p;
  unsigned int t;
  static unsigned int lasttick;
  int d;

  t = timerticks;
  if(!(t-lasttick) && !nowaitgroup) return;

  if((t - lasttick) > 0x1000)
  {
    fprintf(stderr,"Timerproblems?\n");
    lasttick = timerticks;
    return;
  }

  t -= lasttick;
  lasttick += t;

  for(g=gfirst;g!=NULL;g=g->next) /* all groups */
  {
    if(g->gameflag)
    {
      d = 1<<g->divider;
      if(g->dividercnt > d) /* sure is sure */
        g->dividercnt = d;
      else
        g->dividercnt -= t;
      if(g->dividercnt <= 0)
      {
        if(g->dividercnt > -d)
          g->dividercnt += d;
        else
          g->dividercnt = d;
      }
      else
        if(!(nowaitgroup == g->groupno)) continue;

      if(!g->response || nowait)
      {
        print_timerinfo(timerticks &(~(d-1)) );
        g->response = g->numwait;
        send_joydata(g);
        g->timeout = 0;
      }
      else
      {
        g->timeout+=d;
        if((g->timeout & 0x1f) == 0x1f)
        {
          for(p=g->first;p!=NULL;p=p->gnext)
          {
            if(p->playing && (p->mode == PLAYERMODE) && !p->connection->response)
            {
              fprintf(stderr,"Connection of Player %d doesn't response!\n",p->number);
              sprintf(tclcommand, "MESSAGE\nConnection of player %d\nMESSAGECAT\ndoesn't respond\n",
                      p->number);
              sendcommand();
            }
          }
        }
        if(g->timeout > 100)
          close_timeout_sockets(g);
      }
    }
  }

  for(g=gfirst;g!=NULL;g=g->next) /* all groups */
  {
    if( (g->playing == 1) && !g->gameflag)
    {
      if(!g->response)
      {
        g->playing++;
        g->gameflag = TRUE;
        printf("ok ... ACTION!!! ...[(3) stop game] \n");
      }
      else
      {
        if(timerticks - starttick > 100)
        {
          close_timeout_sockets(g);
        }
      }
    }
  }

}

/***********************************
 * I/O-Signal-Handler
 */

void io_cntl(void)
{
#ifdef HAVE_FDSET
  fd_set readmask1;
#else
  struct fd_mask readmask1;
#endif

  int count,len;
  int numfds;
  char buf[280];
  struct cqueue *q;

  for(;;)
  {
    readmask1 = readmask;

    /* we need max. 3+32 (+cams) filedescriptors
     * if select returns with a value < 0 we expect a SIGNAL not an error
     */
    if ((numfds = select(36,&readmask1,NULL,NULL,NULL)) < 0)
    {
      do_timer(FALSE);
      continue;
    }
    do_timer(FALSE);

    if(numfds == 0) break;

    /*
     * slave-sockets
     */
    for(q=cfirst;q!=NULL;)
    {
      if(FD_ISSET(q->socket,&readmask1))
      {
        if((count = recv(q->socket,buf,1,0)) != 1) /* nicht optimal */
        {
          q = close_socket(q);
          continue;
        }
        if((len = (int) nm_field[(int) *buf]) > 0) /* fixed length command */
        {
          if(len > 1)
          {
            count = recv(q->socket,buf+1,len-1,0);
            if(count != len-1)
            {
              fprintf(stderr,"IO: wrong length\n");
              return;
            }
          }
          work_input((unsigned char*)buf,len,q);
        }
        else
        {  /* getting length */
          if((count = recv(q->socket,buf+1,1,0)) != 1) /* nicht optimal */
          {
            fprintf(stderr,"IO: error reading blklen\n");
            return;
          }
          len = (int) buf[1];
          if(len > 2)
          {
            if((count = recv(q->socket,buf+2,len-2,0)) != (len-2) )
            {
              fprintf(stderr,"IO-1: wrong length\n");
              return;
            }
          }
          work_input((unsigned char*)buf,len,q);
        }
      }
      q=q->next;
    }

    /*
     * accept_socket
     */
    if (FD_ISSET(acc_socket, &readmask1))
    {
      accept_socket();
    }

    if(use_exmenu)
    {
      if(FD_ISSET(menu_in,&readmask1))
/* JG HACK        handle_exinput();    */
        handle_input();
    }
    else /* keyboard (stdin) */
    {
      if(FD_ISSET(0,&readmask1))
        handle_input();
    }

  }
} /* end io_handler */


/*************************************
 * (try to) accept a new socket
 */

void accept_socket(void)
{
  int addrlen = sizeof(struct sockaddr);
  struct cqueue *q;

  q = calloc(1,sizeof(struct cqueue)); /* alloc a new node */

  q->response = FALSE;
  q->socket   = accept(acc_socket,&(q->remoteaddr),&addrlen);
  if(q->socket == -1 )
  {
    perror("accept call failed"); exit(1);
  }

  q->next = cfirst;
  q->mode = 0;
  cfirst = q;

  get_hostname(q,q->hostname);

  printf("\n accepted a connection request from [%s].\n",q->hostname);
  sprintf(tclcommand, "MESSAGE\nNew connection request from\nMESSAGECAT\n%s\n",
          q->hostname);
  sendcommand();

  FD_SET(q->socket,&readmask);
}

/************************
 * resolve hostname
 */

char *get_hostname(struct cqueue *q,char *name)
{
  struct hostent *hp;
  char *saddr = (char *) &(((struct sockaddr_in *) &(q->remoteaddr))->sin_addr.s_addr);
  hp = gethostbyaddr(saddr,4,AF_INET);

  if(hp && hp->h_name && (strlen(hp->h_name) > 0))
    strcpy(name,hp->h_name);
  else
    sprintf(name,"%ud.%ud.%ud.%ud",(unsigned int) saddr[0],(unsigned int) saddr[1],(unsigned int) saddr[2],(unsigned int) saddr[3]);
  return name;
}

/**********************************
 * close a connection
 */

struct cqueue *close_socket(struct cqueue *q)
{
  struct cqueue *q2,*q1,*qr;
  struct pqueue *p,*p1;

  qr = q->next;

  FD_CLR(q->socket,&readmask); /* shutdown ? */
  close(q->socket);
  q->socket = -1;

  for(q2=cfirst,q1=NULL;q2 != NULL;q1=q2,q2=q2->next)
    if(q2 == q)
      break;

  if(q1 == NULL)
    cfirst = q->next;
  else
    q1->next = q->next;

  fprintf(stdout,"I've lost a connection ...\n");

  for(p=q->players;p!=NULL;p=p1)
  {
    p1=p->cnext;
    leave_group(p);
    remove_player(p,q);
  }
  free(q);

  return qr;
}

/*********************************
 * close not responding sockets
 */

void close_timeout_sockets(struct gqueue *g)
{
  struct pqueue *p;

  for(p=g->first;p!=NULL;)
  {
    if(p->playing)
    {
      if(!p->connection->response)
      {
        fprintf(stderr,"Connection of Player %d doesn't response .. -> shutdown\n",p->number);
        close_socket(p->connection);
        p=g->first;
        continue;
      }
    }
    p=p->gnext;
  }
}

/*******************************
 * handle an incoming packet
 * works only for SINGLE(PLAYER/CAMERA)-connections
 */

void work_input(unsigned char *buf,int len,struct cqueue *q)
{
  unsigned int lval;
  struct pqueue *pl;
  int cn;

/* ident?? */
  if( !(q->mode & (SINGLEPLAYER|SINGLECAMERA)) && (*buf != NM_SETMODE))
  {
    fprintf(stderr,"error: Junk on an unchecked connection.\n");
    return;
  }

  switch(*buf)
  {
    case NM_OWNDATA:
      if(q->mode & SINGLEPLAYER)
      {
        if(q->players == NULL) 
          break;

        q->players->joydata = (int) buf[1];
        if(q->players->playing && (q->players->mode == PLAYERMODE) )
        {
          q->players->group->response--;
          q->response = TRUE;
          if(q->players->group->timeout && !q->players->group->response
                                        && !nowait)
            do_timer(q->players->groupnumber);
        }
      }
      else
      {

      }
      break;
    case NM_MESSAGE:
        if(q->players == NULL)
          break;
        buf[(int) buf[1]] = 0;
        switch(buf[2])
        {
          case MSG_SERVER:
            printf("%s\n",buf+6); /* message to server */
            break;
          case MSG_ALL:
            break; /* shout all */
          case MSG_GROUP:
            send_mes((char*)buf+6,q->players,MSG_GROUP,NULL,q->players->group);
            break; /* group-message */
          case MSG_TEAM:
            break; /* team-message */
          case MSG_TO_NAME:
            if((pl=find_player((char*)buf+6)) != NULL)
              print_player((char*)buf+7+strlen((char*)buf),pl);
        }
	break;
    case NM_OWNNAME:
        fprintf(stderr,"CLIENT sent an old command.\n");
        send_message("Sorry, your running an old client-version.\n",q);
        close_socket(q);
	break;
    case NM_READY:
        if(q->players == NULL) 
          break;
        if(q->players->playing && (q->players->mode == PLAYERMODE))
        {
          q->players->group->response--;
          q->response = TRUE;
        }
	break;
    case NM_END:
        if(q->players == NULL) 
          break;
        fprintf(stderr,"Received END-Command by: %s.\n",q->players->name);
        if(q->players->group->gameflag && q->players->playing)
          if(q->players->master)
          {
            fprintf(stderr,"Accepted END-Command by: %s.\n",q->players->name);
            end_game(q->players->group);
          }
        break;
    case NM_SETMODE:
        if(!(q->mode & (SINGLEPLAYER)) )
        {
          lval = 0;
          lval |= ((unsigned int) buf[1]) << 24;
          lval |= ((unsigned int) buf[2]) << 16;
          lval |= ((unsigned int) buf[3]) << 8;
          lval |= ((unsigned int) buf[4]);

          switch(lval)
          {
            case PLAYERMAGIC:
              q->mode = SINGLEPLAYER;
              printf("It's a SINGPLEPLAYER connection!\n");
              break;
            case CAMMAGIC:
              q->mode = SINGLECAMERA;
              printf("It's a CAMERA connection! (not debugged!)\n");
              break;
            default:
              printf("Unknown Magic: Do you use different netprotocolls?\n");
              close_socket(q);
              break;
          }
        }
        break;
      case NM_OWNCOMMENT:
        if(q->players == NULL) 
          break;
        buf[(int)buf[1]] = 0;
        if(strlen((char*)buf+2) > MAXCOMMENT)
          buf[2+MAXCOMMENT] = 0;
        strcpy(q->players->comment,(char*)buf+2);
        if(q->players->group->playing)
          send_comment(q->players->group,q->players);
        break;
      case NM_DOCOMMAND:
        if(q->players == NULL) 
          break;
        fprintf(stderr,"-EXCOMMAND-\n");
        if(q->players->master)
          do_command(q->players->group,q->players,(char*)buf+2);
        break;
      case NM_JOIN:
        cn = ((int)buf[1]<<8)+buf[2];
        if((pl=find_player_by_cn(q,cn)) != NULL)
          join_group((int)buf[3],pl);
        else
          fprintf(stderr,"Join-Error!\n");
        break;
      case NM_ADDPLAYER:
        buf[(int)buf[1]] = 0; /* add \0 to playernamename */
        if(strlen((char*)buf+4) > MAXNAME)
          buf[4+MAXNAME] = 0;
        if(q->mode & SINGLEPLAYER)

          add_player(q,((int)buf[2]<<8)+buf[3],PLAYERMODE,(char*)buf+4);
        else if(q->mode & SINGLECAMERA)
          add_player(q,((int)buf[2]<<8)+buf[3],CAMMODE,(char*)buf+4);

/*          add_player(q,q->socket,PLAYERMODE,(char*)buf+4);
        else if(q->mode & SINGLECAMERA)
          add_player(q,q->socket,CAMMODE,(char*)buf+4); */
        break;
      case NM_REMOVEPLAYER:
        break;
      default:
        fprintf(stderr,"error: Unknown NET-Command!!\n");
        break;
  }
}

/***********************************/
/* Start Game  (master)            */
/***********************************/

void start_game(struct gqueue *g,int *teams)
{
  sprintf(tclcommand, "STARTGAME\n");
  sendcommand();

  g->playing = 1;
  g->numgamers = g->numplayers;

  init_slots(0,teams,g);

  send_names(g);
  send_comments(g);
  send_maze(g);
  send_start(g);
}

/**************************/
/* init_slots             */
/**************************/

void init_slots(int first,int *teams,struct gqueue *g)
{
  int j;
  struct pqueue *p;

  g->numwait=0;

  for(p=g->first,j=first;p!=NULL;p=p->gnext)
  {
    p->playing = 1;

    if(p->mode != PLAYERMODE)
      continue;

    p->joydata = 0;
    p->connection->response = FALSE;
    p->number = j;

    if(teams != NULL)
      p->team = teams[j];
    j++;
    g->numwait++;
  }
}

/***********************/
/* Spielende           */
/***********************/

void end_game(struct gqueue *g)
{

  
  char data[2];
  struct pqueue *p;

  sprintf(tclcommand, "STOPGAME\n");
  sendcommand();

  g->playing = 0;
  g->gameflag = FALSE;

  data[0] = NM_STOPGAME;
  send_group(g,0,data,1);

  for(p=g->first;p != NULL;p=p->gnext)
  {
    p->playing = FALSE;
  }
  fprintf(stdout,"Gameover in Group %s.\n",g->groupname);
}

/**********************************/
/* Sende Routinen                 */
/**********************************/

void send_maze(struct gqueue *g)
{
  int (*hfeld)[MAZEDIMENSION],(*vfeld)[MAZEDIMENSION];
  int dim,i,j;
  char buf[MAZEDIMENSION+10];

  hfeld = g->maze.hwalls;
  vfeld = g->maze.vwalls;
  dim = g->maze.xdim;

  for(i=0;i<=dim;i++)
  {
    for(j=0;j<dim;j++)
      buf[j+4] = (char) hfeld[i][j];
    buf[0] = NM_MAZEH;
    buf[1] = dim+4;
    buf[2] = (char) dim;
    buf[3] = (char) i;

    send_group(g,PLAYING,buf,dim+4);
  }

  for(i=0;i<=dim;i++)
  {
    for(j=0;j<dim;j++)
      buf[j+4] = (char) vfeld[j][i];
    buf[0] = NM_MAZEV;
    buf[1] = dim+4;
    buf[2] = (char) dim;
    buf[3] = (char) i;

    send_group(g,PLAYING,buf,dim+4);
  }
}

/*********************/
/* Send Names        */
/*********************/

void send_names(struct gqueue *g)
{
  char data[256];
  struct pqueue *p;

  for(p=g->first;p!=NULL;p=p->gnext)
  {
    if(p->playing && (p->mode == PLAYERMODE) )
    {
      data[0] = NM_ALLNAMES;
      data[1] = 3 + strlen(p->name);
      data[2] = (char) p->number;
      strcpy(data+3,p->name);
      send_group(g,PLAYING,data,data[1]);
    }
  }
}

/*********************/
/* Send Comments     */
/*********************/

void send_comments(struct gqueue *g)
{
  struct pqueue *p;

  for(p=g->first;p!=NULL;p=p->gnext)
    send_comment(g,p);
}

void send_comment(struct gqueue *g,struct pqueue *p)
{
  char data[256];

  if(p->playing && (p->mode == PLAYERMODE) )
  {
    data[0] = NM_ALLCOMMENTS;
    data[1] = 3 + strlen(p->comment);
    data[2] = (char) p->number;
    strcpy(data+3,p->comment);
    send_group(g,PLAYING,data,data[1]);
  }
}


/****************************************************
 * send start (init) data block
 * including: num. of players & playernumbers
 */

void send_start(struct gqueue *g)
{
  char data[MAXPLAYERS+10];
  int rndwert;
  struct pqueue *p;

  g->response = g->numwait;
  g->numjoy = g->numgamers;

  printf("Starting game with %d player(s) in group %s,\n",g->numgamers,g->groupname);

  data[0] = NM_STARTGAME;
  data[1] = g->numgamers + 16;
  data[2] = (char) g->numgamers;

  rndwert = rand();
  data[4] = rndwert >> 8;
  data[5] = rndwert & 0xff;

  if(g->numteams == 0)
    data[6] = (char) g->numgamers;
  else
    data[6] = g->numteams;

  data[7] = (unsigned char) (g->gamemode & 0xff);
  data[8] = (unsigned char) (g->gamemode>>8);
  data[9] = (unsigned char) g->divider;

  /* data 10 - 15 free for future changes */

  for(p=g->first;p!=NULL;p=p->gnext)
  {
    if(p->playing && (p->mode == PLAYERMODE))
    {
      if(g->numteams)
        data[16+p->number] = p->team;
      else
        data[16+p->number] = p->number;
    }
  }

  for(p=g->first;p!=NULL;p=p->gnext)
  {
    if(p->playing)
    {
      p->connection->response = FALSE; /* warning! */
      if(p->mode == PLAYERMODE)
        data[3] = (char) p->number;
      else
        data[3] = 0;
      send(p->connection->socket,data,data[1],0);
    }
  }
  starttick = timerticks;
}

/*************************************
 * send a (new) message
 */

void send_mes(char *m,struct pqueue *from,int type,struct pqueue *p,struct gqueue *g)
{
  unsigned char data[256];
  char mes[256];

  if(from != NULL)
  {
    if(from->checked)
      strcpy(mes,"<");
    else
      strcpy(mes,"!<");
    strcat(mes,from->name);
    strcat(mes,"> ");
  }
  else
    strcpy(mes,"[Server] ");

  strcat(mes,m);      

  data[0] = NM_MESSAGE;
  data[1] = 7 + strlen(mes);
  data[2] = type;
  data[3] = 0;
  data[4] = 0;
  data[5] = 0;
  strcpy((char *) data+6,mes);

  switch(type)
  {
    case MSG_ALL:
      send_command(cfirst,0,(char*)data,data[1]);
      break;
    case MSG_GROUP:
      data[4] = g->groupno;
      send_group(g,0,(char*)data,data[1]);
      break;
    case MSG_PLAYER:
      data[4] = (p->cnumber>>8) & 0xff;
      data[5] = p->cnumber & 0xff;
      send(p->connection->socket,(char *) data,(int) data[1],0);
      break;
    case MSG_TEAM:
      for(p=g->first;p!=NULL;p=p->gnext)
        if( (from->team == p->team) && (from != p) )
        {
          data[4] = (p->cnumber>>8) & 0xff;
          data[5] = p->cnumber & 0xff;
          send(p->connection->socket,(char *) data,(int) data[1],0);
        }
      break;
  }
}

/********************/
/* Send Message     */
/********************/

void send_message(char *str,struct cqueue *q)
{
  char data[256];

  data[0] = NM_MESSAGE;
  data[1] = strlen(str)+7;
  data[2] = 0;
  data[3] = 0;
  data[4] = 0;
  data[5] = 0;
  strcpy(data+6,str);
  send(q->socket,data,data[1],0);
}

/*************************
 * Send joystick data
 */

void send_joydata(struct gqueue *g)
{
  char data[MAXPLAYERS+2];
  struct pqueue *p;

  for(p=g->first;p!=NULL;p=p->gnext)
  {
    if(p->playing && (p->mode == PLAYERMODE))
    {
      data[p->number+1] = (char) p->joydata;
      p->connection->response = FALSE;
    }
  }

  data[0] = NM_ALLDATA;
  send_group(g,PLAYING,data,g->numjoy+1);
}

void send_inactive(struct gqueue *g,struct pqueue *p)
{
  char data[3];

  data[0] = NM_INACTIVATE;
  data[1] = g->groupno;
  data[2] = p->number;
  send_group(g,PLAYING,data,3);
}

/**************************************************************************
 *
 *   NEW STUFF:
 *
 */

/************************************
 * Send a command (new)
 */

void send_command(struct cqueue *q,int mask,char *data,int len)
{
  while(q != NULL)
  {
    if(q->mode & mask)
      send(q->socket,data,len,0);
    q = q->next;
  }
}

/************************************
 * send a command to all players of a group
 */

void send_group(struct gqueue *g,int mask,char *data,int len)
{
  struct pqueue *p=g->first;

  while(p != NULL)
  {
    if(!mask || ((mask == PLAYING) && p->playing))
      send(p->connection->socket,data,len,0);
    p = p->gnext;
  }
}

/************************************
 * add player (or camera)
 */

struct pqueue *add_player(struct cqueue *c,int cn,int mode,char *name)
{
  struct pqueue *p,*p1,*p2=NULL;

  p = calloc(1,sizeof(struct pqueue));

  if(find_player(name) == NULL)
  {
    p->checked = TRUE;
  }

  for(p1=c->players;p1!=NULL;p1=p1->cnext)
  {
    if(p1->cnumber == cn)
      fprintf(stderr,"Duplicated connection-numbers.\n");
    p2=p1;
  }
  if(p2 == NULL)
  {
    c->players = p;
  }
  else
  {
    p2->cnext = p;
    p->clast = p2;
  }

  p->connection = c;
  p->mode = mode;
  p->cnumber = cn;

  if(name != NULL)
  {
    if(strlen(name)>MAXNAME)
      name[MAXNAME] = 0;
    strcpy(p->name,name);
  }
  strcpy(p->comment,"Gotcha!!!!");

  if(p->checked)
    send_mes("OK, player added with a valid name.",NULL,MSG_PLAYER,p,NULL);
  else
    send_mes("OK, player added with an invalid name.",NULL,MSG_PLAYER,p,NULL);

  fprintf(stderr,"Added player %s : %d.\n",p->name,p->cnumber);
  list_connections();
  sprintf(tclcommand, "MESSAGECAT\nNew player: %s\n", p->name);
  sendcommand();

  return p;
}

void remove_player(struct pqueue *p,struct cqueue *c)
{
  list_connections();
  sprintf(tclcommand, "MESSAGE\nLost player: %s\n", p->name);
  sendcommand();
  free(p);
  /* if(c != NULL) -> silent on this connection */

}

/************************************
 * leave group
 */

int leave_group(struct pqueue *p)
{
  struct gqueue *g;

  g = p->group;

  if(!p->groupnumber || (g == NULL) )
    return 0;

  if(p->glast == NULL)
    p->group->first = p->gnext;
  else
    p->glast->gnext = p->gnext;

  if(p->gnext != NULL)
  {
    p->gnext->glast = p->glast;
  }

  p->gnext = NULL;
  p->glast = NULL;
  p->group = NULL;
  p->groupnumber = 0;

  if(p->mode == PLAYERMODE)
    g->numplayers--;

  if(p->master)
    if(g->first != NULL)
      g->first->master = TRUE;

  if(p->playing == TRUE)
  {
    p->playing = FALSE;
    if(p->mode == PLAYERMODE)
    {
      g->numgamers--;
      g->numwait--;
      if(!p->connection->response)
        g->response--;
    }

    if((g->numgamers == 0) || (g->numwait == 0)) /* double-check */
    {
      fprintf(stdout,"Last active player has left the game.\n");
      end_game(g);
    }
    else
      send_inactive(g,p);
  }

  return 1;
}

/************************************
 * join group
 */

int join_group(int no,struct pqueue *p)
{
  struct gqueue *g;
  struct pqueue *p1,*p2;

  if(p==NULL)
    return FALSE;

  if(p->groupnumber)
    leave_group(p);

  for(g=gfirst;g!=NULL;g=g->next)
    if(g->groupno == no)
      break;
  if(g == NULL)
    return 0;

  for(p1=g->first,p2=NULL;p1!=NULL;p1=p1->gnext) p2=p1;

  if(p2 == NULL)
  {
    g->first = p;
    p->glast = NULL;
    p->master = TRUE;
  }
  else
  {
    p2->gnext = p;
    p->glast = p2;
    p->master = FALSE;
  }
  p->gnext = NULL;
  p->groupnumber = no;
  p->group = g;
  p->playing = FALSE;

  if(p->mode == PLAYERMODE)
    g->numplayers++;

  send_mes("OK, here I am!",p,MSG_GROUP,NULL,g);
  if(g->playing)
    send_mes("Please wait .. another game is running!",NULL,MSG_PLAYER,p,NULL);
  else
    send_mes("OK, joined group!",NULL,MSG_PLAYER,p,NULL);

  return 1;
}

/***************************************
 * find a group (by name)
 */

struct gqueue *find_group(char *name)
{
  struct gqueue *g;

  for(g=gfirst;g!=NULL;g=g->next)
  {
    if(strcmp(g->groupname,name) == 0)
      return g;
  }
  return NULL;
}

/***************************************
 * find a player (by name) (later: hashtables)
 */

struct pqueue *find_player(char *name)
{
  struct cqueue *c;
  struct pqueue *p;

  if(name == NULL) return NULL;

  for(c=cfirst;c!=NULL;c=c->next)
    for(p=c->players;p!=NULL;p=p->cnext)
    {
      if(p->checked) /* only checked players */
        if(strcmp(name,p->name) == 0) return p;
    }

  return NULL;
}

/***************************************
 * find a player (by connectionnumber)
 */

struct pqueue *find_player_by_cn(struct cqueue *c,int cn)
{
  struct pqueue *p;

  if(c == NULL) return NULL;

  for(p=c->players;p!=NULL;p=p->cnext)
  {
    if(p->cnumber == cn)
      return p;
  }

  return NULL;
}

/***************************************
 * Create a new group
 */

struct gqueue *new_group(int no)
{
  struct gqueue *g,*g1,*g2=NULL;

  g = calloc(1,sizeof(struct gqueue));

  for(g1=gfirst;g1!=NULL;g1=g1->next) g2=g1;
  if(g2 == NULL)
    gfirst = g;
  else
    g2->next = g;

  g->groupno = no;
  strcpy(g->groupname,"NETMAZE");

  return g;
}

/*******************************
 * List connections
 */

void list_connections(void)
{
  struct cqueue *q;
  struct pqueue *p;
  struct gqueue *g=gfirst;
  int i;

  sprintf(tclcommand, "CONNECTIONLIST\n");
  sendcommand();  
  
  printf("No.: |     Player-Name: | hostname\n");
  printf("-----+------------------+----------------\n");

/*  for(p=g->first,i=0;p!=NULL;p=p->gnext,i++)  */
  for(q=cfirst,i=0;q!=NULL;q=q->next,i++) 
  {
    if(q->players->mode == PLAYERMODE) {
      printf("%3d  | %16s | %s\n",i,q->players->name,q->hostname);
      sprintf(tclcommand, "PLAYER\n%d\n%s\n%s\n", i,
              q->players->name, q->hostname);
      sendcommand();
    }
    else
      printf("%3d  | %16s | %s\n",i,"A Camera?!?",q->hostname);
  }
  printf("-----+------------------+----------------\n");
  sprintf(tclcommand, "ENDCONNECTIONLIST\n");
  sendcommand();
}

/***************************/
/* little help for newbies */
/***************************/

void usage(void)
{
  printf("Usage: netserv [-h|-help] [-exmenu <external-menu-program>] [-nowait]\n");
  printf("\t-h|-help: this message\n");
  printf("\t-exmenu: Control with an external menu\n");
  printf("\t-nowait: server shouldn't wait for clients (for very slow lines)\n");
  printf("\t-tclmenu: Uses tcl menu\n");
  exit(0);
}

void print_timerinfo(unsigned int t)
{
  switch(t & 0x3f)
  {
    case 0:
      fprintf(stderr,"\010|");
      break;
    case 0x10:
      fprintf(stderr,"\010/");
      break;
    case 0x20:
      fprintf(stderr,"\010-");
      break;
    case 0x30:
      fprintf(stderr,"\010\\");
      break;
  }
}

void print_menu(void)
{
  if(use_exmenu) return;

  printf("\t****************** MENU ******************\n");
  printf("\t1 [mazename] => Reinit/Load a maze ( rndmaze: 1 <size> <maxlen> )\n");
  printf("\t2 [teamlist] => Start game [with teams]\n");
  printf("\t3            => Stop a running game\n");
  printf("\t4            => List connections\n");
  printf("\t5 <No.>      => Shutdown a connection <No.>\n");
  switch(gfirst->divider)
  {
    case 0:
      printf("\t7            => Change 'beat' divider (current: 1)\n");
      break;
    case 1:
      printf("\t7            => Change 'beat' divider (current: 2)\n");
      break;
    case 2:
      printf("\t7            => Change 'beat' divider (current: 4)\n");
      break;
  }
  printf("\t21: %s, 22: %s, 23: %s, 24: %s\n",
         (gfirst->gamemode & GM_REFLECTINGSHOTS) ? "BOUNCE" : "bounce",
         (gfirst->gamemode & GM_DECAYINGSHOTS) ? "DECAY" : "decay",
         (gfirst->gamemode & GM_MULTIPLESHOTS) ? "MULTISHOT" : "multishot",
         (gfirst->gamemode & GM_WEAKINGSHOTS) ? "HURTS2SHOOT" : "hurts2shoot");
  printf("\t25: %s, 26:%s, 27: %s\n",
         (gfirst->gamemode & GM_REPOWERONKILL) ? "REPOWERONKILL" : "repoweronkill",
         (gfirst->gamemode & GM_FASTRECHARGE) ? "FASTHEAL" : "fastheal",
         (gfirst->gamemode & GM_FASTWALKING) ? "FASTWALK" : "fastwalk");
/*         (gfirst->gamemode & GM_SHOWGHOST) ? "GHOSTMODE" : "ghostmode"); */
  printf("\t29: %s, 30: %s, 32: %s\n",
         (gfirst->gamemode & GM_ALLOWHIDE) ? "CLOAK" : "cloak",
         (gfirst->gamemode & GM_ALLOWRADAR) ? "RADAR" : "radar",
/*         (gfirst->gamemode & GM_DESTRUCTSHOTS) ? "DESTRUCTSHOTS" : "destructshots", */
         (gfirst->gamemode & GM_DECSCORE) ? "DECSCORE" : "decscore");
  printf("\t34: %s\n",
/*         (gfirst->gamemode & GM_RANKSCORE) ? "RANKSCORE" : "rankscore"); */
         (gfirst->gamemode & GM_TEAMSHOTHURT) ? "TEAMSHOTHURT" : "teamshothurt");
     
  printf("\t------------------------------------------\n");
  printf("\t9            => Quit\n\n");
}

int reaper() {
  while(wait3(NULL, WNOHANG, NULL) > 0) {}
}

void sendcommand(void)
{
int len;
char *pts = tclcommand;
int status = 0, n, count;

  if (!use_tclmenu) return;
/* Sends the string in tclcommand to the external menu. */

  count = len = strlen(tclcommand);


  if (count < 0) exit(255);

   while (status != count) { 
        n = write(menu_out, pts+status, count-status); 
        if (n < 0) exit(255);
        status += n; 
    } 
    return;
} 

int read_buffer(int fd, char *buf, int count)
{
    char *pts = buf;
    int  status = 0, n; 

    if (count < 0) return (-1);

    while (status != count) { 
        n = read(fd, pts+status, count-status); 
        if (n < 0) return n;
        status += n; 
    }
    return (status);
}


int read_pipe(int socket, char *buf, int maxlen)
{
int status;
int count = 0;

  while (count <= maxlen) {
    if ((status = read_buffer(socket, buf+count, 1)) < 1) {
      printf("Error reading in function read_pipe\n");
      return 0;
    }
    if ((buf[count] == '\n') || buf[count] == '\r') {
      buf[count] = 0;
      return count;
    }
    count++;
  }
  buf[count] = 0;
  return count;
}
