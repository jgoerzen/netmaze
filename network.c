/*
 * slave-network-handler
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "netmaze.h"
#include "netcmds.h"
#include "network.h"

#define TRUE 1
#define FALSE 0

extern struct shared_struct *sm;

static void handle_packet(char *buf,int len);

static void send_owndata(void);
void send_owncomment(void);
static void send_ownready(void);
static void send_ident(void);
static void send_end(void);
static void send_addplayer(void);
static void send_join(void);

extern void move_all(PLAYER*,int*);
extern void run_game(MAZE*,PLAYER*);
extern void myrandominit(int);
extern void inactivate_player(int);
extern void activate_player(int);

extern int own_socket; /* streams socket descriptor */

struct robot_functions robot = { 0, } ;

/******************************************************
 * send the ident/add/join stuff (also used by robot.c)
 */

void ident_player(void)
{
  send_ident();
  send_addplayer();
  send_join();
  send_owncomment();
}

/**************************************************
 * handle the incoming data (also used by robot.c)
 */

void handle_socket(void)
{
  static char	buf[256];
  int	count,len;
  static int frag=0,fraglen;

      if(frag > 0)
      { /* we allow exact 1 fragmentation (our messages aren't int) */
        if((count = recv(own_socket,buf+fraglen,frag,0)) != frag)
	{
	  fprintf(stderr,"Major protocoll-error: %d!!\n",buf[0]);
	  exit(1);
	}
	fprintf(stderr,"Tried to handle fragmentation\n");
	handle_packet(buf,fraglen+frag);
	frag = 0;
	return;
      }
      else if( (count = recv(own_socket,buf,1,0)) != 1) /* nicht optimal */
      {
        if(count == 0)
        {
          fprintf(stderr,"I've lost the Server-Connection\n");
          exit(3);
        }
        fprintf(stderr,"IO: error reading ident\n"); return;
      }
      if((len = (int) nm_field[(unsigned char)*buf]) > 0)
      {
        if(len > 1)
        {
          count = recv(own_socket,buf+1,len-1,0);
          if(count != len-1)
          {
            fprintf(stderr,"IO: wrong length: %d %d\n",len-1,count);
            if(count >= 0)
            {
              frag = len - 2 - count;
              fraglen = count + 2;
              return;
            }
          }
        }
        handle_packet(buf,len);
      }
      else
      {
        if((count = recv(own_socket,buf+1,1,0)) != 1) /* nicht optimal */
        {
          fprintf(stderr,"IO: error reading blklen\n");  return;
        }
        len = (int) buf[1];
        if(len > 2)
        {
          if((count = recv(own_socket,buf+2,len-2,0)) != (len-2) )
          {
            fprintf(stderr,"IO-1: wrong length - possibly fragmented %d %d %d %d\n",len-2,count,(int) buf[0],(int) buf[1]);
            if(count >= 0)
	    {
	      frag = len - 2 - count;
              fraglen = count + 2;
	      return;
            }
	  }
        }
        handle_packet(buf,len);
      }
}

/*************************************
 * handle packets
 */

static void handle_packet(char *buf,int len)
{
  char data[1];
  int (*hfeld)[MAZEDIMENSION],(*vfeld)[MAZEDIMENSION];
  int i,j;
  int randbase;

  switch(*buf)
  {
    case NM_STARTGAME:
	sm->anzplayers = (int) buf[2];
        sm->shownumber = sm->shownumbertmp = (int) buf[3];
        sm->numteams = (int) buf[6];
        for(i=0;i<sm->anzplayers;i++)
        {
          sm->playfeld[i].team = buf[16+i];
        }
        randbase = (int) (unsigned char) buf[5];
        randbase += ((int) (unsigned char) buf[4]) << 8;
        myrandominit(randbase);
        sm->gamemode = (unsigned char) buf[7];
        sm->gamemode += ((int)(unsigned char)buf[8])<<8;
        sm->config.divider = (int) buf[9];
        nm_field[NM_ALLDATA] = (char) sm->anzplayers + 1;
        run_game(&(sm->std_maze),sm->playfeld);

        if(robot.valid)
          (robot.start)(sm->shownumber);

        if(!sm->camera)
	  send_ownready();
        sm->gameflag = TRUE;
	break;
    case NM_STOPGAME:
        if(sm->gameflag)
        {
          sm->gameflag   = FALSE;
          sm->winnerdraw = FALSE;
          sm->bgdraw     = TRUE;
          sm->screendraw = FALSE;
        }
	break;
    case NM_MAZEH:
        hfeld = sm->std_maze.hwalls;
        sm->std_maze.xdim = (int) buf[2];
        i = (int) buf[3];
        for(j=0;j<sm->std_maze.xdim;j++)
        {
          hfeld[i][j] = (int) buf[j+4];
        }
        break;
    case NM_MAZEV:
        vfeld = sm->std_maze.vwalls;
        sm->std_maze.ydim = (int) buf[2];
        i = (int) buf[3];
        for(j=0;j<sm->std_maze.ydim;j++)
        {
          vfeld[j][i] = (int) buf[j+4];
        }
        break;
    case NM_ALLDATA:
        for(i=0;i<sm->anzplayers;i++)
          sm->sticks[i] = (int) buf[i+1];

        move_all(sm->playfeld,sm->sticks);

        if(robot.valid)
          sm->ownstick = (robot.action)();

        if(sm->gameflag)
        {
          if(!sm->camera)
            send_owndata();
        }
        else
          send_end();
	break;
    case NM_MESSAGE:
        fprintf(stderr,"%s\n",buf + 6);
	break;
    case NM_ALLNAMES:
        strncpy(sm->playfeld[(unsigned char) buf[2]].name,buf+3,buf[1]-3);
        sm->playfeld[(unsigned char)buf[2]].name[(int)(unsigned char)buf[1]-3] = 0;
	break;
    case NM_PING:
        data[0] = NM_PONG;
        send(own_socket,&data,1,0);
	break;
    case NM_ALLCOMMENTS:
        strncpy(sm->playfeld[(int) (unsigned char) buf[2]].comment,buf+3,buf[1]-3);
        sm->playfeld[(int)(unsigned char)buf[2]].comment[(int)(unsigned char)buf[1]-3] = 0;
        break;
    case NM_ACTIVATE:
        activate_player((int) buf[2]);
        break;
    case NM_INACTIVATE:
        inactivate_player((int) buf[2]);
        break;
  }
}

static void send_owndata(void)
{
  char data[2];
  data[0] = NM_OWNDATA;
  data[1] = (char) sm->ownstick;

  send(own_socket,data,2,0);
}

static void send_ownready(void)
{
  char data = NM_READY;
  send(own_socket,&data,1,0);
}

void send_owncomment(void)
{
  char buf[100];

  buf[0] = NM_OWNCOMMENT;
  buf[1] = strlen(sm->owncomment)+2;
  strcpy(buf+2,sm->owncomment);
  send(own_socket,buf,buf[1],0);
}

static void send_ident(void)
{
  char buf[10];

  if(!sm->camera)
  {
    buf[1] = (char) ((PLAYERMAGIC>>24) & 0xff);
    buf[2] = (char) ((PLAYERMAGIC>>16) & 0xff);
    buf[3] = (char) ((PLAYERMAGIC>>8) & 0xff);
    buf[4] = (char) (PLAYERMAGIC & 0xff);
  }
  else
  {
    buf[1] = (char) ((CAMMAGIC>>24) & 0xff);
    buf[2] = (char) ((CAMMAGIC>>16) & 0xff);
    buf[3] = (char) ((CAMMAGIC>>8) & 0xff);
    buf[4] = (char) (CAMMAGIC & 0xff);
  }
  buf[0] = NM_SETMODE;
  send(own_socket,buf,5,0);
}
/*
static void send_docommand(char *c,int len)
{
  char data[256];

  data[0] = NM_DOCOMMAND;
  data[1] = len+2;
  memcpy(data+2,c,len);
  send(own_socket,data,data[1],0);
}
*/
static void send_end(void)
{
  char data[] = { NM_END };
  send(own_socket,data,1,0);
}

static void send_addplayer(void)
{
  char data[32];
  data[0] = NM_ADDPLAYER;
  data[1] = strlen(sm->ownname)+5;
  data[2] = 0;
  data[3] = 1;
  strcpy(data+4,sm->ownname);
  send(own_socket,data,data[1],0);
}

static void send_join(void)
{
  char data[4];
  data[0] = NM_JOIN;
  data[1] = 0;
  data[2] = 1;
  data[3] = 1;
  send(own_socket,data,4,0);
}


