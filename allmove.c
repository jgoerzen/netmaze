/*
 * movement-handler
 */

#include <stdio.h>
#include <string.h>

#include "netmaze.h"

#define REBOUNCE_PERC	0.85

extern void play_sound(int);
extern int random_maze(MAZE*,int,int);

extern long trigtab[];
extern struct shared_struct *sm;

static void enemy_colision(long,long,PLAYER*,PLAYER*);
static int  enemy_touch(PLAYER *player,PLAYER *players);
static void wall_pcoll(long,long,PLAYER*);
static int  wall_scoll(PLAYER*,int nr);
static int  player_hit(int,long,long,PLAYER *players);
static void set_player_pos(PLAYER*,int,MAZE *mazeadd);
static int  add_shot(PLAYER*);
static void remove_shot(PLAYER*,int);
static int  ball_bounce(PLAYER *p,int i,int xc,int yc,long x,long y);
static void convert_trigtabs(int divider);
void myrandominit(long s);
static int myrandom(void);
static void reset_player(PLAYER *players,int i);

long walktab[320],shoottab[320];

/*
 in diesem Programmteil sollten moeglichst keine
 Veraenderungen vorgenommen werden, weil eine
 Aenderung sehr leicht zu Inkompatibilitaeten
 zwischen den Rechnern fuehren kann.
 Auch sollte man auf Berechnungen mit Floatingpoint
 und Divisionen allgemein verzichten. Statt Divisionen
 mit Shifts arbeiten.

 You shouldn't do changes in this file .. because changes
 can cause incompatibility between two different compilations.
 You should also avoid floatingpointcalculations and divisions.
 (Use shifts instead of divisions!)
*/

/***********************************/
/* this is the 'heart' of the game */
/* moving player,shots             */
/* check collisions ....           */
/***********************************/

void move_all(PLAYER* players,int *joywerte)
{
  int i,joy,wink,plynum,j,next;
  PLAYER *player;
  long plx,ply;
  int count;

  count = 1<<sm->config.divider;
RERUN:

  if(sm->armageddon > 0)
  {
    sm->armageddon--;
    if(!sm->armageddon)
    {
      random_maze(&sm->std_maze,10,0); /* 10x10 arena */ 
      for(i=0;i<sm->anzplayers;i++)
        set_player_pos(players,i,&sm->std_maze);
    }
  }

  player = players;

  for(i=0;i<sm->anzplayers;i++,player++)
  {
    if(player->alive) /* is the player alive? */
    {
      joy = joywerte[i]; /* get players joy/mouse/keyboard-movement-value */
      if(sm->gamemode & GM_ALLOWHIDE)
      {
        if(joy & JOY_HIDE) player->hide = TRUE;
        else player->hide = FALSE;
      }
      if(joy & JOY_UP) /* press stick UP? */
      {
        player->hide = FALSE;
        wink = player->winkel;
        plx = player->x; /* copy old position */
        ply = player->y;
        if(sm->gamemode & GM_FASTWALKING)
        {
          player->x += walktab[wink]<<1;
          player->y += walktab[wink+64]<<1;
        }
        else
        {
          player->x += walktab[wink]; /* add step to position */
          player->y += walktab[wink+64];
        }
        if (player->x < 0) player->x = 0;
        if (player->y < 0) player->y = 0;

        wall_pcoll(plx,ply,player); /* coll? yes => set old value */
        enemy_colision(plx,ply,player,players);
      }
      else if(joy & JOY_DOWN)
      {
        player->hide = FALSE;
        wink = player->winkel;
        plx = player->x;
        ply = player->y;
        if(sm->gamemode & GM_FASTWALKING)
        {
          player->x -= walktab[wink]<<1;
          player->y -= walktab[wink+64]<<1;
        }
        else
        {
          player->x -= walktab[wink];
          player->y -= walktab[wink+64];
        }
        if (player->x < 0) player->x = 0;
        if (player->y < 0) player->y = 0;
        wall_pcoll(plx,ply,player);
        enemy_colision(plx,ply,player,players);
      }
      if(joy & JOY_RIGHT)
      {
        if(joy & JOY_SLOW)
          player->winkel += sm->config.angle_step>>2;
        else
        {
          player->winkel += sm->config.angle_step; /* step angle right */
          player->winkel &= ~(sm->config.angle_step-1);
        }
        if(player->winkel >= MAX_WINKEL) player->winkel -= MAX_WINKEL;
      }
      else if(joy & JOY_LEFT)
      {
        if(joy & JOY_SLOW)
          player->winkel -= sm->config.angle_step>>2;
        else
        {
          player->winkel += sm->config.angle_step-1;
          player->winkel &= ~(sm->config.angle_step-1);
          player->winkel -= sm->config.angle_step; /* step angle left */
        }
        if(player->winkel < 0) player->winkel += MAX_WINKEL;
      }

      if(joy & JOY_BUTTON)
      {
        if(player->recharge == 0) /* gun is recharged? */
        {
          if(i == sm->shownumber)
            play_sound(1);

          player->stat.shots++; /* statistics */
          player->hide = FALSE;
          if((!(sm->gamemode & GM_WEAKINGSHOTS)) || (player->fitness >= 80))
          {
            if(sm->gamemode & GM_MULTIPLESHOTS)
            {
              j = add_shot(player);
              if(j >= 0)
              {
                player->shots[j].sx = player->x; /* set start-position */
                player->shots[j].sy = player->y;
                wink = player->winkel;
                player->shots[j].sxd = shoottab[wink]; /* set shot-step */
                player->shots[j].syd = shoottab[wink+64];
                player->shots[j].power = 320;
                player->recharge = sm->config.rechargetime;
              }
            }
            else
            {
              if(player->numofshots > 0)
                remove_shot(player,player->shotqueue2);
              j = add_shot(player);
              player->shots[j].sx = player->x; /* set start-position */
              player->shots[j].sy = player->y;
              wink = player->winkel;
              player->shots[j].sxd = shoottab[wink]; /* set shot-step */
              player->shots[j].syd = shoottab[wink+64];
              player->shots[j].power = SHOTPOWER;
              player->recharge = sm->config.rechargetime; /* disable gun */
            }
            if(sm->gamemode & GM_WEAKINGSHOTS)
              player->fitness -= 80;
          }
        }
      }
      if(sm->gamemode & GM_ALLOWRADAR)
      {
        if(joy & JOY_RADAR)
          player->radar = TRUE;
        else
          player->radar = FALSE;
      }
      if((player->fitness < sm->config.startfitness) || player->radar )
      {
        if(player->radar)
          player->fitness -= sm->config.repower>>1;
        else
          player->fitness += sm->config.repower; /* repower the player */
        player->statchg = TRUE; /* power-bar changes (redraw) */
        if(player->fitness > sm->config.startfitness)
          player->fitness = sm->config.startfitness;
        else if(player->fitness < 0)
          player->fitness = 0;
      }

    } /* end: player is alive */
    else
    {
       /* player is dead/or has left the game */
      if( (++player->fitness > 0) && player->active)
      {
        set_player_pos(players,i,&(sm->std_maze)); /*reincarnate player */
        player->fitness = sm->config.startfitness; /*give him the start-power */
        player->alive = TRUE;   /* make him alive */
        player->statchg = TRUE; /* and set the redraw-flag */
        if(i == sm->shownumber)
          play_sound(2);
      }
    }

    /*
     * move shots
     */
    for(j=player->shotqueue2;j >= 0;j=next)
    {
      next = player->shots[j].next;
      player->shots[j].sx += player->shots[j].sxd; /* move the shot */
      player->shots[j].sy += player->shots[j].syd;
      if(wall_scoll(player,j)) /*shot hits wall?*/
      {
        if(sm->gamemode & GM_REFLECTINGSHOTS)
        {
          player->shots[j].power *= REBOUNCE_PERC; /* lose power */
          if(player->shots[j].power <= 0)
            remove_shot(player,j);
        }
        else
        {
          remove_shot(player,j);
        }
      }
      else if( (plynum = player_hit(i,player->shots[j].sx,player->shots[j].sy,players)) >= 0) /*hits an enemy? */
      {
        remove_shot(player,j);
        if(i == sm->shownumber)
          play_sound(5);
        else if(plynum == sm->shownumber)
          play_sound(4);


        if(player->team != players[plynum].team) /* friendly-fire-protect */
        {
          player->stat.hits++;
          players[plynum].stat.ownhits++;
          players[plynum].fitness -= player->shots[j].power;
          players[plynum].statchg  = TRUE;
          player->follow = plynum;
        }
        players[plynum].hitbycnt = 8; /* recolour enemy for 8 beats */
        players[plynum].hitby    = player->team;
        players[plynum].hide     = FALSE;

        if(players[plynum].fitness < 0) /* enemy is out of power? */
        {
          if(i == sm->shownumber)
            play_sound(0);
          else if(plynum == sm->shownumber)
            play_sound(3);

          player->stat.kills++;
          players[plynum].stat.ownkills++;

          player->follow = -1;
          players[plynum].alive     = FALSE;  /* make him dead */
          players[plynum].fitness   = -sm->config.deadtime;/*reincarnate-delay*/
          players[plynum].ownkiller = i;      /* set the killer */
          players[plynum].hitbycnt  = 0;      /* set colour */
          player->killtable[player->killanz] = plynum;
          player->killanz++;
          player->killchg = TRUE;
          sm->teams[player->team].kills++; /* inc kills */
          if(sm->teams[player->team].kills >= sm->config.kills2win) /*enough kills?*/
          {
            sm->gameflag   = FALSE;
            sm->winner     = i;
            sm->winnerdraw = TRUE;
            sm->screendraw = FALSE;
            sm->statchg2   = TRUE;
            sm->killchg    = TRUE;
            sm->redraw = TRUE;
            return;
          }
          sm->statchg1 = TRUE; /* state-change for every player */
        }
        else
        {
            /* for statistics */
        }
      }
      else
      {
        if(sm->gamemode & GM_DECAYINGSHOTS)
        {
          player->shots[j].power -= sm->config.shot_decay;
          if(player->shots[j].power <= 0)
          {
            remove_shot(player,j);
          }
        }
      }
    } /* end for(j...) */

    if(player->recharge) player->recharge--;
    if(player->hitbycnt > 0) player->hitbycnt -= sm->config.recolour_delay;
  }

  sm->drawwait++;      /* internal 'beat' counter */
  sm->redraw = TRUE;

  count--;
  if(count > 0) goto RERUN;   /* for divider */

}

/*
 * Shot-queue-helper:
 */

static int add_shot(PLAYER *p)
{
  int i,old,new;

  if(p->shotqueue1 == -1) return -1;
  p->numofshots++;
  old = p->shotqueue2;
  new = p->shotqueue1;
  i = p->shots[new].next;
  p->shots[new].next = old;
  p->shots[new].last = -1;
  p->shotqueue2 = p->shotqueue1;
  p->shotqueue1 = i;
  if(old >= 0)
  {
    p->shots[old].last = p->shotqueue2;
  }
  return new;
}

static void remove_shot(PLAYER *p,int i)
{
  int last,next;

  last = p->shots[i].last;
  next = p->shots[i].next;
  if(last >= 0)
    p->shots[last].next = p->shots[i].next;
  else
    p->shotqueue2 = p->shots[i].next;
  if(next >= 0)
    p->shots[next].last = last;
  p->numofshots--;
  p->shots[i].next = p->shotqueue1;
  p->shotqueue1 = i;
}

/******************************/
/* Player <-> Wall Collision  */
/******************************/

static void wall_pcoll(long xold,long yold,PLAYER *player)
{
  long x,y;
  int  xc,yc;
  int  xflag=-1;
  int  yflag=-1;

  x = (player->x) & 0x00ffffff;
  y = (player->y) & 0x00ffffff;
  xc = (int) ((player->x) >> 24);
  yc = (int) ((player->y) >> 24);

  if(x < 0x00400000)
  {
    xflag = 0;
    if(sm->std_maze.vwalls[yc][xc] & MAZE_BLOCK_PLAYER)
    {
      player->x = (player->x & 0xff000000) | 0x00400000;
      xflag = -1;
    }
  }
  else if(x > 0x00c00000)
  {
    xflag = 1;
    if(sm->std_maze.vwalls[yc][xc+1] & MAZE_BLOCK_PLAYER)
    {
      player->x = (player->x & 0xff000000) | 0x00c00000;
      xflag = -1;
    }
  }

  if(y < 0x00400000)
  {
    yflag = 0;
    if(sm->std_maze.hwalls[yc][xc] & MAZE_BLOCK_PLAYER)
    {
      player->y = (player->y & 0xff000000) | 0x00400000;
      yflag = -1;
    }
  }
  else if(y > 0x00c00000)
  {
    yflag = 2;
    if(sm->std_maze.hwalls[yc+1][xc] & MAZE_BLOCK_PLAYER)
    {
      player->y = (player->y & 0xff000000) | 0x00c00000;
      yflag = -1;
    }
  }

  if((xflag != -1) && (yflag != -1))
  {
     switch(xflag | yflag)
     {
       case 0:
         if( (sm->std_maze.vwalls[yc-1][xc] & MAZE_BLOCK_PLAYER) ||
             (sm->std_maze.hwalls[yc][xc-1] & MAZE_BLOCK_PLAYER) )
         {
           if(player->x > xold)
           {
             player->y = (player->y & 0xff000000) | 0x00400000;
           }
           else if(player->y > yold)
           {
             player->x = (player->x & 0xff000000) | 0x00400000;
           }
           else
           {
             player->x = xold; /* very simple (and not good) */
             player->y = yold;
           }
         }
         break;
       case 1:
         if( (sm->std_maze.vwalls[yc-1][xc+1] & MAZE_BLOCK_PLAYER) ||
             (sm->std_maze.hwalls[yc][xc+1] & MAZE_BLOCK_PLAYER) )
         {
           if(player->x < xold)
           {
             player->y = (player->y & 0xff000000) | 0x00400000;
           }
           else if(player->y > yold)
           {
             player->x = (player->x & 0xff000000) | 0x00c00000;
           }
           else
           {
             player->x = xold; /* very simple (and not good) */
             player->y = yold;
           }
         }
         break;
       case 2:
         if( (sm->std_maze.vwalls[yc+1][xc] & MAZE_BLOCK_PLAYER) ||
             (sm->std_maze.hwalls[yc+1][xc-1] & MAZE_BLOCK_PLAYER) )
         {
           if(player->x > xold)
           {
             player->y = (player->y & 0xff000000) | 0x00c00000;
           }
           else if(player->y < yold)
           {
             player->x = (player->x & 0xff000000) | 0x00400000;
           }
           else
           {
             player->x = xold; /* very simple (and not good) */
             player->y = yold;
           }
         }
         break;
       case 3:
         if( (sm->std_maze.vwalls[yc+1][xc+1] & MAZE_BLOCK_PLAYER) ||
             (sm->std_maze.hwalls[yc+1][xc+1] & MAZE_BLOCK_PLAYER) )
         {
           if(player->x < xold)
           {
             player->y = (player->y & 0xff000000) | 0x00c00000;
           }
           else if(player->y < yold)
           {
             player->x = (player->x & 0xff000000) | 0x00c00000;
           }
           else
           {
             player->x = xold; /* very simple (and not good) */
             player->y = yold;
           }
         }
         break;
     }
  }
}

/******************************/
/* Shot <-> Wall Collission   */
/******************************/

static int wall_scoll(PLAYER *p,int i)
{
  long x,y;
  int  xc,yc,flag=0;
  long sx,sy;

  sx = p->shots[i].sx;
  sy = p->shots[i].sy;

  x = sx & 0x00ffffff;
  y = sy & 0x00ffffff;
  xc = (int) (sx >> 24);
  yc = (int) (sy >> 24);

  if(x < 0x001e0000)
  {
    flag |= 1;
  }
  else if(x > 0x00e20000)
  {
    flag |= 4;
  }

  if(y < 0x001e0000)
  {
    flag |= 2;
  }
  else if(y > 0x00e20000)
  {
    flag |= 8;
  }

  switch(flag)
  {
    case 1:
      if(sm->std_maze.vwalls[yc][xc] & MAZE_BLOCK_SHOT)
      {
        if(p->shots[i].sxd >= 0)
        {
          p->shots[i].sx -= 0x001e0000 + x;
        }
        else
        {
          p->shots[i].sx += 0x001e0000 - x;
        }
        p->shots[i].sxd = -p->shots[i].sxd;
        return(TRUE);
      }
      break;
    case 2:
      if(sm->std_maze.hwalls[yc][xc] & MAZE_BLOCK_SHOT)
      {
        if(p->shots[i].syd >= 0)
        {
          p->shots[i].sy -= 0x001e0000 + y;
        }
        else
        {
          p->shots[i].sy += 0x001e0000 - y;
        }
        p->shots[i].syd = -p->shots[i].syd;
        return(TRUE);
      }
      break;
    case 3:
      return ball_bounce(p,i,xc,yc,x,y);
    case 4:
      if(sm->std_maze.vwalls[yc][xc+1] & MAZE_BLOCK_SHOT)
      {
        if(p->shots[i].sxd >= 0)
        {
          p->shots[i].sx -= x - 0x00e20000;
        }
        else
        {
          p->shots[i].sx += 0x011e0000 - x;
        }
        p->shots[i].sxd = -p->shots[i].sxd;
        return(TRUE);
      }
      break;
    case 6:
      return ball_bounce(p,i,xc+1,yc,x,y);
    case 8:
      if(sm->std_maze.hwalls[yc+1][xc] & MAZE_BLOCK_SHOT)
      {
        if(p->shots[i].syd >= 0)
        {
          p->shots[i].sy -= y - 0x00e20000;
        }
        else
        {
          p->shots[i].sy += 0x011e0000 - y;
        }
        p->shots[i].syd = -p->shots[i].syd;
        return(TRUE);
      }
      break;
    case 9:
      return ball_bounce(p,i,xc,yc+1,x,y);
    case 12:
      return ball_bounce(p,i,xc+1,yc+1,x,y);
  }
  return(FALSE);
}

/*
 * wall_scoll-helper (not complete yet)
 */

static int ball_bounce(PLAYER *p,int i,int xc,int yc,long x,long y)
{
  int f = 0,w = 0;

  w = (sm->std_maze.vwalls[yc][xc]<<2) + (sm->std_maze.hwalls[yc][xc]<<1) +
       sm->std_maze.vwalls[yc-1][xc] + (sm->std_maze.hwalls[yc][xc-1]<<3);

  if(p->shots[i].sxd >= 0) f = 1;
  if(p->shots[i].syd >= 0) f |= 2;

  switch(f)
  {
    case 0:
      if( ((w & 0x6) == 0x6) || ( !(w & 0x6) && ((w & 0x9) == 0x9)))
      {
        p->shots[i].sxd = -p->shots[i].sxd;
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w & 0x2)
      {
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w)
      {
        p->shots[i].sxd = -p->shots[i].sxd;
      }
      break;
    case 1:
      if( ((w & 0xc) == 0xc) || ( !(w & 0xc) && ((w & 0x3) == 0x3)))
      {
        p->shots[i].sxd = -p->shots[i].sxd;
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w & 0x8)
      {
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w)
      {
        p->shots[i].sxd = -p->shots[i].sxd;
      }
      break;
    case 2:
      if( ((w & 0x3) == 0x3) || ( !(w & 0x3) && ((w & 0xc) == 0xc)))
      {
        p->shots[i].sxd = -p->shots[i].sxd;
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w & 0x2)
      {
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w)
      {
        p->shots[i].sxd = -p->shots[i].sxd;
      }
      break;
    case 3:
      if( ((w & 0x9) == 0x9) || ( !(w & 0x9) && ((w & 0x6) == 0x6)))
      {
        p->shots[i].sxd = -p->shots[i].sxd;
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w & 0x8)
      {
        p->shots[i].syd = -p->shots[i].syd;
      }
      else if(w)
      {
        p->shots[i].sxd = -p->shots[i].sxd;
      }
      break;
  }

  return(w?TRUE:FALSE);
/*
  if(w)
    return TRUE;
  else
    return FALSE;
*/
}



/********************************/
/* Player <-> Player Collision  */
/********************************/

static void enemy_colision(long xold,long yold,PLAYER *player,PLAYER *players)
{
  if(enemy_touch(player,players))
  {
    player->x = xold;
    player->y = yold;
  }
}

static int enemy_touch(PLAYER *player,PLAYER *players)
{
  int i;
  long xd,yd;

  for(i=0;i<sm->anzplayers;i++,players++)
  {
    if((player != players) && players->alive )
    {
      xd = (player->x - players->x);
      yd = (player->y - players->y);

      if(xd < 0) xd = -xd;
      if(yd < 0) yd = -yd;

      if((xd > 0x800000) || (yd > 0x800000)) continue;

      xd>>=12; yd>>=12;

      if( (xd*xd + yd*yd) < 0x280000)
        return TRUE;
    }
  }
  return FALSE;
}


/********************************/
/* Player <-> Shot Collision    */
/* -1: no hit / >= 0: playernr. */
/********************************/

static int player_hit(int plnr,long sx,long sy,PLAYER *plys)
{
  int i;
  long xd,yd;

  for(i=0;i<sm->anzplayers;i++,plys++)
  {
    if((i != plnr) && plys->alive )
    {
      if( (xd = (sx - plys->x)) < 0) xd = -xd;
      if( (yd = (sy - plys->y)) < 0) yd = -yd;

      if((xd > 0x800000) || (yd > 0x800000)) continue;

      xd>>=12; yd>>=12;

      if( (xd*xd + yd*yd) < 0x180000)
        return(i);
    }
  }
  return(-1);
}

/************************/
/* Game starts          */
/* Setup Start-Settings */
/************************/

void run_game(MAZE *mazeadd,PLAYER *players)
{
  int i;

  sm->winnerdraw = FALSE;
  sm->screendraw = TRUE;

  sm->ownstick = 0;
  memset(sm->sticks,0,MAXPLAYERS);

  if(sm->sologame)
  {
/*
    sm->gamemode   = GM_MULTIPLESHOTS | GM_WEAKINGSHOTS | GM_REFLECTINGSHOTS |
                     GM_DECAYINGSHOTS | GM_ALLOWHIDE | GM_ALLOWRADAR;
*/
    sm->gamemode   = GM_CLASSIC;
    sm->numteams = 4;
    for(i=0;i<sm->anzplayers;i++)
    {
      players[i].team = i % sm->numteams;
    }
    myrandominit(10000);
  }
  for(i=0;i<sm->numteams;i++)
  {
    sm->teams[i].kills = 0;
  }

  /* setup the configure-stuff */
  sm->config.angle_step = WINKEL_STEP;
  sm->config.recolour_delay = 1;
  sm->config.repower = REFITSPEED;
  sm->config.shot_decay = 2;
  sm->config.deadtime = DEADTIME;
  sm->config.startfitness = STARTFITNESS;
  sm->config.kills2win = WINNERANZ;
  sm->config.armageddon = 1000000; /* armageddon after 1.000.000 beats */
  sm->armageddon = sm->config.armageddon;

  if(sm->gamemode & GM_MULTIPLESHOTS)
  {
    if(sm->gamemode & GM_FASTRECHARGE)
      sm->config.rechargetime = 4;
    else
      sm->config.rechargetime = 8;
  }
  else
    sm->config.rechargetime = 36;
  convert_trigtabs(0);

  fprintf(stderr,"*** New game with %d player(s) starts! ***\n",sm->anzplayers);

  for(i=0;i<sm->anzplayers;i++)
  {
    reset_player(players,i);
    fprintf(stderr,"%16s: Number: %d\tTeam: %d\n",players[i].name,i,players[i].team);
  }

  for(i=0;i<sm->anzplayers;i++)
    set_player_pos(players,i,mazeadd);

  sm->statchg1 = TRUE;
  sm->killchg = TRUE;
}

/****************************************
 * reset player data
 */

static void reset_player(PLAYER *players,int i)
{
  int j;

  memset(&players[i].stat,0,sizeof(STATISTIC));
  players[i].stat.gamemode = sm->gamemode;

  players[i].x = players[i].y = 0;
  players[i].fitness  = sm->config.startfitness;
  players[i].killanz  = 0;
  players[i].ownkills = 0;
  players[i].alive    = TRUE;
  players[i].radar    = FALSE;
  players[i].hide     = FALSE;
  players[i].recharge = 0;
  players[i].hitbycnt = 0;
  players[i].follow   = -1;
  for(j=0;j<MAXSHOTS-1;j++)
  {
    players[i].shots[j].next = j+1;
  }
  players[i].shots[j].next = -1;
  players[i].shotqueue1 = 0;  /* all shots in the free-queue */
  players[i].shotqueue2 = -1; /* no flying shots */
  players[i].numofshots = 0;
  players[i].active = TRUE;

  if(strlen(players[i].name) == 0)
  {
    strcpy(players[i].name,"Player Nr. ");
    sprintf(players[i].name+strlen(players[i].name),"%d",i);
  }
  if(strlen(players[i].comment) == 0)
  {
    strcpy(players[i].comment,"Have a nice day!");
  }
}

/********************************
 * place a player on the field
 */

static void set_player_pos(PLAYER *players,int num,MAZE *mazeadd)
{
  int i;
  int xc,yc;
  PLAYER *data = players+num;

  data->winkel = myrandom() & 0xf0;
  data->alive  = TRUE;

  for(i=0;i<32;i++) /* try 32 times */
  {
    data->x = ( (xc = myrandom() % mazeadd->xdim) << 24) + 0x800000;
    data->y = ( (yc = myrandom() % mazeadd->ydim) << 24) + 0x800000;
    if((sm->std_maze.vwalls[yc][xc] & MAZE_BLOCK_PLAYER) &&
       (sm->std_maze.vwalls[yc][xc+1] & MAZE_BLOCK_PLAYER) &&
       (sm->std_maze.hwalls[yc][xc] & MAZE_BLOCK_PLAYER) &&
       (sm->std_maze.hwalls[yc+1][xc] & MAZE_BLOCK_PLAYER)) continue;
    if(!enemy_touch(data,players)) break;
  }
}

/**********************************
 * activate
 */

void activate_player(int pl)
{
  if(pl >= sm->anzplayers)
    sm->anzplayers++;

  reset_player(sm->playfeld,pl);
}

/**********************************
 * inactivate 
 */

void inactivate_player(int pl)
{
  PLAYER *p=&sm->playfeld[pl];

  p->active = FALSE;
  p->x = p->y = 0;
  p->killanz  = 0;
  p->ownkills = 0;
  p->alive    = FALSE;
}

/******************************************
 * "Random" from: r.sedgewick/algorithms
 */

void myrandominit(long s)
{
  int j;
  sm->rndshiftpos = 10;

  for( sm->rndshifttab[0]=s,j=1;j<55;j++)
    sm->rndshifttab[j] = (sm->rndshifttab[j-1] * 31415821 + 1) % 100000000;
}

static int myrandom(void)
{
  int i,j;

  if(++sm->rndshiftpos > 54) sm->rndshiftpos = 0;

  if( (i=(sm->rndshiftpos + 23)) > 54) i=sm->rndshiftpos-22;
  if( (j=(sm->rndshiftpos + 54)) > 54) j=sm->rndshiftpos-1;

  sm->rndshifttab[sm->rndshiftpos] =
                     (sm->rndshifttab[i] + sm->rndshifttab[j]) % 100000000;
  return (sm->rndshifttab[sm->rndshiftpos] & 0x7fff);
}

/**********************/
/* converting sin/cos */
/**********************/

static void convert_trigtabs(int divider)
{
  long *tab1 = trigtab,*tab2 = walktab,*tab3 = shoottab;
  int i;
  long s;
  static int t = -1;

  if(divider == t) return;
  t = divider;

  switch(t)
  {
    case 0:    /* DIVIDER_1 */
      for(i=0;i<320;i++)
      {
        *tab2 = (s = *(tab1++)>>3)>>1;  /* tab2 =  W/16 */
        *tab3++ = *tab2++ + s;      /* tab3 =  W/8 + W/16 = W/5.33333 */
      }
      break;
#if 0
    case 1:   /* DIVIDER_2 */
      for(i=0;i<320;i++)
      {
        *tab2 = (s = *(tab1++)>>2)>>1;  /* tab2 =  W/8 */
        *tab3++ = *tab2++ + s;      /* tab3 =  W/4 + W/8 = W/2.66667 */
      }
      break;
    case 2:   /* DIVIDER_4 */
      for(i=0;i<320;i++)
      {
        *tab2 = (s = *(tab1++)>>1)>>1;  /* tab2 =  W/4 */
        *tab3++ = *tab2++ + s;      /* tab3 =  W/2 + W/4 = W/1.33333 */
      }
      break;
#endif
  }
}

