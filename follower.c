/* 
 * follower bot
 * written and copyrights: roderick@ksu.ksu.edu (Mike Roderick)
 */

#include <string.h>
#include "netmaze.h"
#include <math.h>

#define PI 3.141592

extern struct shared_struct *sm;

/*******************
 * the init_robot is called one time right after the start
 */

void init_robot(void)
{
  strcpy(sm->ownname,"Follower2");
  strcpy(sm->owncomment,"Look Behind You");
}

void start_robot(void)
{
  printf("Here We Go Again\n");
  printf("The number of players in this game is:  %d\n",sm->anzplayers);
}

/*******************
 * the own_action is called every 'beat'
 *
 * this bot really is a dummy, I use this guy
 * to test my program with 31 'players'.
 */

int own_action(void)
{
  int ret=0;
  int chase;
  static int prtcntr = 0;
  int me;
  int angle;
  int mangle;
  int mag,dir;
  int updown;
  int shot;

  me = sm->shownumber;

  if(sm->playfeld[me].fitness < 800)
    updown = JOY_DOWN;
  else
    updown = JOY_UP;

  mangle = sm->playfeld[me].winkel;        /* get my angle */

  chase = whos_close(me);                  /* who should I chase */

  if (chase == -1)
    return 0;

  angle = what_ang(chase , me);            /* what is the target's agnle */

   if((sm->playfeld[me].fitness<800)&&sm->playfeld[me].team!=sm->playfeld[chase].team)
    updown = JOY_DOWN;
  else
    updown = JOY_UP; 

  if (!(abs(angle-mangle)<6) || sm->playfeld[me].team == sm->playfeld[chase].team)
    shot = 0;                  /* hold your fire */
  else
    shot = JOY_BUTTON;         /* shoot them */

  dir = angle - mangle;
  mag = abs(dir);
  
  if (((dir < 0) && (mag < 128)) || ((dir > 0) && (mag >= 128)))
    {
      ret = JOY_LEFT | updown | shot;
    }
  else if (((dir > 0) && (mag < 128)) || ((dir < 0) && (mag >=128)))
    {
      ret = JOY_RIGHT | updown | shot;
    }
  else
    {
      ret = updown | shot;
    }
/*
  if ( sm->playfeld[me].fitness < 800 )
    updown = JOY_DOWN;
  else
    updown = JOY_UP;

  if (!(( angle - mangle < 6 ) && ( angle - mangle > -6 )))
    shot = 0;
  else
    shot = JOY_BUTTON;

  if ( angle > mangle )
    {
      ret = JOY_RIGHT | updown | shot;
    }
  else if ( angle < mangle )
    {
      ret = JOY_LEFT | updown | shot;
    }
  else
    {
      ret = updown | JOY_BUTTON;
    }
*/
  if(prtcntr++ > 10&&0)
    {
      prtcntr = 0;
      printf("chasing opponent: %d at angle: %d My angle: %d\n",chase,angle,sm->playfeld[me].winkel);
    }

  return ret;
}


int whos_close(int me)
{
  int i;
  int dist=999999999;
  int temp;
  int close=-1;


  for( i = 0 ; i < (sm->anzplayers) ; i++ )
    {
      if( i == me )
	continue;
      temp = get_dist( i , me );
      if((dist > temp) && (sm->playfeld[i].alive) && (sm->playfeld[i].team != sm->playfeld[me].team))
	{
	  dist = temp;
	  close = i;
	}
/*      else if (dist == -1)
	{
	  dist = 999999999;
	  close = i;
	}
*/
    }
  return close;
}

int get_dist(int a, int b)
{
  int tmp1;
  int tmp2;
  int difx;
  int dify;
  int dist;
  double dx,dy,di;

  tmp1 = sm->playfeld[a].x;
  tmp2 = sm->playfeld[b].x;
  difx = tmp1 - tmp2;
  tmp1 = sm->playfeld[a].y;
  tmp2 = sm->playfeld[b].y;
  dify = tmp1 - tmp2;
  dx   = (double) difx;
  dy   = (double) dify;
  dx  *= dx;
  dy  *= dy;
  di   = sqrt(dx+dy);
  dist = (int) di;
  return dist;
}

int what_ang(int target , int me)
{
  int ang;
  int difx;
  int dify;
  double an;
  double dx;
  double dy;
  int tmp1;
  int tmp2;
  double tmp3;

  tmp1 = sm->playfeld[me].x;
  tmp2 = sm->playfeld[target].x;
  difx = tmp1 - tmp2;
  dx   = (double) difx;
  if (dx == 0.0)
    dx = 1.0;
  tmp1 = sm->playfeld[me].y;
  tmp2 = sm->playfeld[target].y;
  dify = tmp1 - tmp2;
  dy   = (double) dify;
  tmp3 = dy/dx;
  an   = -atan(tmp3);
  an   = (an/(2.0*PI))*256.0;
  ang  = (int) an;
  if ( difx < 0 )
    ang += 63;
  else
    ang += 191;
  if ( ang > 255 )
    ang -= 256;
  return ang;
}






















