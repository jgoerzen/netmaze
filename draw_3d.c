/*
 * Calculate the gameview (optimizations are welcome)
 */

#ifdef ALL_PERFORMANCE_TEST
 #include <time.h>
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "trigtab.h"
#include "netmaze.h"

extern struct shared_struct *sm;

static int  wall_3d(long,long,long,long,int,WALL*);
static void sort_walls(WALL*,int);
static int  comp(WALL*,WALL*);
static int  clip_walls(WALL*,int);
static int  calc_players(int,WALL*,PLAYER*,int);
static int  calc_shoots(int,WALL*,PLAYER*,int);
static int  calc_walls(PLAYER*,WALL*,MAZE*,int);
static int  stest(WALL *w1,WALL *w2);
static void resort(WALL *f,int n);

/* X11-Draw-stuff: */
extern void draw_maze(WALL*,PLAYER*,int,int);
extern void draw_texture_maze(WALL *walls,PLAYER *play,int anzahl,int nr);
extern void draw_rmap(PLAYER*,XSegment*,int);
extern void draw_kill(PLAYER*,int);
extern void draw_status(int,PLAYER*);
extern void draw_kills(int,PLAYER*);

static WALL wallbuff[300]; /* better would be a pointerfield to WALL-struct */

/************************************
 * mainfunction: calls all important
 */

void draw_screen(void)
{
  int anz=0,killer,i;

  memcpy(sm->playfeld1,sm->playfeld,sm->anzplayers*sizeof(PLAYER));
  sm->statchg = sm->statchg1;
  sm->statchg1 = FALSE;

  sm->shownumber = sm->shownumbertmp;
  sm->playfeld[sm->shownumber].statchg = FALSE;
  sm->playfeld[sm->shownumber].killchg = FALSE;

  sm->anzlines = 0; /* maplines */

  if(sm->playfeld1[sm->shownumber].alive)
  {
    anz = calc_players(sm->shownumber,wallbuff,sm->playfeld1,anz);
    anz = calc_shoots(sm->shownumber,wallbuff,sm->playfeld1,anz);
    anz = calc_walls(sm->playfeld1,wallbuff,&(sm->std_maze),anz);

    sort_walls(wallbuff,anz);
    if(sm->debug == 0)
      anz = clip_walls(wallbuff,anz);
    resort(wallbuff,anz);

    if(sm->debug)
      for(i=anz-1;i>=0;i--)
        printf("%d: %x %lx %lx %lx %lx\n",i,wallbuff[i].ident,wallbuff[i].rmax,
               wallbuff[i].rmin,wallbuff[i].xd,wallbuff[i].yd);

    if(sm->debug) sm->debug--;

    draw_status(sm->shownumber,sm->playfeld1);
    draw_kills(sm->shownumber,sm->playfeld1);
    if(sm->mapdraw)
      draw_rmap(sm->playfeld1,sm->maplines,sm->anzlines);
#ifdef ALL_PERFORMANCE_TEST
    { long a=clock();
#endif
    if(!sm->texturemode)
      draw_maze(wallbuff,sm->playfeld1,anz,sm->shownumber); /* <-does XSync()*/
    else
      draw_texture_maze(wallbuff,sm->playfeld1,anz,sm->shownumber); 
#ifdef ALL_PERFORMANCE_TEST
    a=clock()-a; fprintf(stderr,"%ld ",a); }
#endif
  }
  else
  {
    killer = sm->playfeld1[sm->shownumber].ownkiller;
    draw_status(sm->shownumber,sm->playfeld1);
    draw_kills(sm->shownumber,sm->playfeld1);
    draw_kill(sm->playfeld1,killer); /* <- does XSync() */
  }
}


/****************************
 * Clipping of invisible walls
 */

static int clip_walls(WALL *walls,int anzahl)
{
  int desti=0,i,j;
  register int xr,xl,rclip,lclip;
  WALL *wall1;
  struct { int r,l; } cliplist[300];
  int cliplen = 0;
  int update,drawflag;

  wall1 = walls;

  for(j=0;j<anzahl;j++)
  {
    if(walls[j].ident < 0x100) /* smiley-clip */
    {
      update = FALSE;
      drawflag = TRUE;

      xl = walls[j].x1-(walls[j].h1>>1);
      xr = walls[j].x2+(walls[j].h1>>1);
      if(xl < XMIN)
      {
        if(xr < XMIN) 
          continue;
        else
          xl = XMIN;
      }
      if(xr > XMAX)
      { 
        if(xl > XMAX)
          continue;
        else
          xr = XMAX;
      }

      for(i=0;i<cliplen;i++)
      {
        if(xl < (lclip = cliplist[i].l))
        {
          if(xr < lclip-1) continue;
          xr = lclip-1;
        }
        else
        {
          if(xl > (rclip = cliplist[i].r)+1) continue;
          if(xr <= rclip)
          {
            drawflag = FALSE;
            i = cliplen; /* no 2nd loop */
            break;
          }
          xl = rclip+1;
        }
      }
    }
    else /* wallclip */
    {
      if( (xl = walls[j].lclip = walls[j].x1) < XMIN)
        xl = walls[j].lclip = XMIN;
      if( (xr = walls[j].rclip = walls[j].x2) > XMAX)
        xr = walls[j].rclip = XMAX;

      update = TRUE;
      drawflag = TRUE;

      for(i=0;i<cliplen;i++)
      {
        if(xl < (lclip = cliplist[i].l))
        {
          if(xr < lclip-1) continue;
          if(xr > cliplist[i].r)
          {
             cliplist[i].l = xl;
             cliplist[i].r = xr;
             update = FALSE;
             break;
          }
          xr = walls[j].rclip = lclip-1; /*right wallside is behind another*/
          cliplist[i].l = xl;
          update = FALSE;
          break;
        }
        else
        {
          if(xl > (rclip = cliplist[i].r)+1) continue;
          if(xr <= rclip)
          {
            update = FALSE; /* whole wall is behind another wall */
            drawflag = FALSE;
            i = cliplen; /* no second loop */
            break;
          }
          xl = walls[j].lclip = rclip+1;
          cliplist[i].r = xr;
          update = FALSE;
          break;
        }
      }
    }

    if(!update)
    {
      for(i++;i<cliplen;i++)
      {
        if(xl < (lclip = cliplist[i].l))
        {
          if(xr < lclip-1) continue;
          xr = walls[j].rclip = lclip-1;
        }
        else
        {
          if(xl > (rclip = cliplist[i].r)+1) continue;
          if(xr <= rclip)
          {
            drawflag = FALSE;
            break;
          }
          xl = walls[j].lclip = rclip+1;
        }
      }
    }
    else
    {
      cliplist[cliplen].l = xl;
      cliplist[cliplen++].r = xr;
    }
    if(drawflag)
      walls[desti++] = walls[j];
  }
  return(desti);
}


/***************************/
/* Z-Pos-Sorting           */
/***************************/

static void sort_walls(WALL *walls,int anzahl)
{
   qsort(walls,(size_t) anzahl,sizeof(WALL),(int (*)(const void*,const void*)) comp);
}

static int comp(WALL *wall1,WALL *wall2)
{
 /* return (wall1->r) - (wall2->r); */
  return (wall1->rmax+wall1->rmin) - (wall2->rmax+wall2->rmin);
}

/**************************
 * calculate the walls
 */

static int calc_walls(PLAYER *players,WALL *walls,MAZE *maze,int anzahl)
{
  long xpos,ypos;
  int xloop,yloop,xfield,yfield,tnr;
  int xdim,ydim,xdist,ydist,istart,jstart,iend,jend;
  int winkel;
  long xrot,yrot,x1rot,y1rot,tsin,tcos;
  double xdrot,ydrot,dsin,dcos /* xd1rot,yd1rot */ ;
  int i,j,p;
  int (*hwalls)[MAZEDIMENSION],(*vwalls)[MAZEDIMENSION];
  long xd,yd,xd1;
  PLAYER *player;

  player = players+sm->shownumber;

  xpos = player->x;
  ypos = player->y;
  winkel = player->winkel;
  hwalls = maze->hwalls;
  vwalls = maze->vwalls;

  dsin = (double) (tsin = trigtab[winkel]);
  dcos = (double) (tcos = trigtab[winkel+64]);

  xfield = (int) (xpos>>24);
  yfield = (int) (ypos>>24);

  xdim = maze->xdim;
  ydim = maze->ydim;

  if(xfield < SICHTWEITE-1)
    xloop = xfield+1;
  else
    xloop = SICHTWEITE;

  xdist = xloop-1;

  if(xfield > xdim - SICHTWEITE)
    xloop += (xdim-xfield);
  else
    xloop += SICHTWEITE;

  if(yfield < SICHTWEITE-1)
    yloop = yfield+1;
  else
    yloop = SICHTWEITE;

  ydist = yloop-1;

  if(yfield > ydim - SICHTWEITE)
    yloop += (ydim-yfield);
  else
    yloop += SICHTWEITE;

  istart = yfield-ydist;
  jstart = xfield-xdist;
  iend = yloop+istart;
  jend = xloop+jstart;

  xdrot = (double) (xd1 = ((long)(-xdist)<<24) - (xpos & 0x00ffffff));
  ydrot = (double) (yd  = ((long)(-ydist)<<24) - (ypos & 0x00ffffff));
/*
  xd1rot = xdrot*dcos - ydrot*dsin;
  yd1rot = ydrot*dcos + xdrot*dsin;
*/
  x1rot = xrot = (long) ( (xdrot*dcos - ydrot*dsin) / 0x1000000 );
  y1rot = yrot = (long) ( (ydrot*dcos + xdrot*dsin) / 0x1000000 );

  sm->marks=0;

  for(i=istart;i<iend;i++,yd += 0x1000000)
  {
    for(xd=xd1,j=jstart;j<jend;j++,xd += 0x1000000)
    {
      if(sm->newmap)
      {
        if (sm->mapdraw && ((i==istart) || (i == iend-1)))
        {
          sm->maplines[sm->anzlines].x1 = (short) (x1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].y1 = (short) (-y1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].x2 = (short) ((x1rot+tcos)>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].y2 = (short) (-(y1rot+tsin)>>21)+MAPSIZE/2;
          sm->anzlines++;
        }
        if (sm->mapdraw && (j==jstart || j==jend-1) )
        {
          sm->maplines[sm->anzlines].x1 = (short) (x1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].y1 = (short) (-y1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].x2 = (short) ((x1rot-tsin)>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].y2 = (short) (-(y1rot+tcos)>>21)+MAPSIZE/2;
          sm->anzlines++;
        }
        for(p=0; p<sm->anzplayers; p++)
        {
          int  tx=((players+p)->x)>>24;
          int  ty=((players+p)->y)>>24;

          /*
           * If they can see you, you can see them...
           */
          if (player->radar || players[p].radar)
            if (i == ty && j == tx && p!=sm->shownumber)
            {
              int x2 = ((x1rot+tcos)>>21)+(MAPSIZE>>1);
              int y2 = (-(y1rot+tsin)>>21)+(MAPSIZE>>1);
              int x3 = ((x1rot-tsin)>>21)+(MAPSIZE>>1);
              int y3 = (-(y1rot+tcos)>>21)+(MAPSIZE>>1);

              sm->markers[sm->marks].x = (x3-((x3-x2)>>1));
              sm->markers[sm->marks].y = (y3-((y3-y2)>>1));
              sm->markers[sm->marks++].player = p;
            }
        }
      }

      if(tnr=(hwalls[i][j] & MAZE_TYPEMASK))
      {
        if(sm->mapdraw && !sm->newmap)
        {
          sm->maplines[sm->anzlines].x1 = (short) (x1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].y1 = (short) (-y1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].x2 = (short) ((x1rot+tcos)>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines++].y2 =(short)(-(y1rot+tsin)>>21)+MAPSIZE/2;
        }
        if(wall_3d(x1rot,y1rot,x1rot+tcos,y1rot+tsin,0x100|(tnr<<10),&walls[anzahl]))
        {
          walls[anzahl].xd = (xd > -0x800000)?xd+0x800000:-xd-0x800000;
          walls[anzahl++].yd = (yd>0)?yd:-yd;
        }
      }
      if(tnr=(vwalls[i][j] & MAZE_TYPEMASK))
      {
        if(sm->mapdraw && !sm->newmap)
        {
          sm->maplines[sm->anzlines].x1 = (short) (x1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].y1 = (short) (-y1rot>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines].x2 = (short) ((x1rot-tsin)>>21)+MAPSIZE/2;
          sm->maplines[sm->anzlines++].y2=(short)(-(y1rot+tcos)>>21)+MAPSIZE/2;
        }
        if(wall_3d(x1rot,y1rot,x1rot-tsin,y1rot+tcos,0x200|(tnr<<10),&walls[anzahl]))
        {
          walls[anzahl].xd = (xd>0)?xd:-xd;
          walls[anzahl++].yd = (yd > -0x800000)?yd+0x800000:-yd-0x800000;
        }
      }
      x1rot += tcos;
      y1rot += tsin;
    }
    x1rot = (xrot -= tsin);
    y1rot = (yrot += tcos);
  }

  return anzahl;
}

static int calc_players(int number,WALL *walls,PLAYER *players,int anz)
{
  int i,wink;
  long x,y,xd,yd,rmax,rmin,hor1,h1;
  double xdrot,ydrot,dsin,dcos;

  x = players[number].x;
  y = players[number].y;
  wink = players[number].winkel;
  dsin = (double) trigtab[wink];
  dcos = (double) trigtab[wink+64];

  for(i=0;i<sm->anzplayers;i++)
  {
    if (((players[i].x != x) || (players[i].y != y)) &&
          players[i].alive && (!players[i].hide))
    {
      xdrot = (double) (xd = players[i].x - x);
      ydrot = (double) (yd = players[i].y - y);

      walls[anz].xd = ((xd>0) ? xd : -xd);
      walls[anz].yd = ((yd>0) ? yd : -yd);

      if( (yd = (long) ((ydrot*dcos + xdrot*dsin) / 0x1000000) >>16) > 0)
      {
        xd = (long) ((xdrot*dcos - ydrot*dsin) / 0x1000000) >>16;

        if(xd > 0)
        {
          rmax = ((xd+PRADIUS)*(xd+PRADIUS) + (yd+PRADIUS)*(yd+PRADIUS))<<2;
          rmin = ((xd-PRADIUS)*(xd-PRADIUS) + (yd-PRADIUS)*(yd-PRADIUS))<<2;
        }
        else
        {
          rmax = ((xd-PRADIUS)*(xd-PRADIUS) + (yd+PRADIUS)*(yd+PRADIUS))<<2;
          rmin = ((xd+PRADIUS)*(xd+PRADIUS) + (yd-PRADIUS)*(yd-PRADIUS))<<2;
        }

        hor1 = xd * (SCREEN - FLUCHT);
        hor1 /= (yd - FLUCHT);
        h1 = (SCREEN - FLUCHT) * SMILEYHEIGHT;
        h1 /= (yd - FLUCHT);

        hor1<<=12; h1<<=12; /* BRKPNT */

        walls[anz].x2 = walls[anz].x1 = (int) hor1;
        walls[anz].h2 = (atan((double)xd/(double)(yd-FLUCHT))*128.0/M_PI)
                          +128-players[i].winkel+wink;
        while(walls[anz].h2 < -128) walls[anz].h2 += 256;
        while(walls[anz].h2 >= 128) walls[anz].h2 -= 256;
        walls[anz].h1 = (int) h1;
        walls[anz].rmax = rmax; walls[anz].rmin = rmin;
        walls[anz++].ident = i;
      }
    }
  }

  return anz;
}

static int calc_shoots(int number,WALL *walls,PLAYER *players,int anz)
{
  int i,j,wink;
  long x,y,xd,yd,rmax,rmin,h1;
  double xdrot,ydrot,dsin,dcos;

  x = players[number].x;
  y = players[number].y;
  wink = players[number].winkel;
  dsin  = (double) trigtab[wink];
  dcos  = (double) trigtab[wink+64];

  for(i=0;i<sm->anzplayers;i++)
  {
    for(j=players[i].shotqueue2;j >= 0;j=players[i].shots[j].next)
    {
      xdrot = (double) (xd = players[i].shots[j].sx - x);
      ydrot = (double) (yd = players[i].shots[j].sy - y);

      walls[anz].xd = ((xd>0) ? xd : -xd);
      walls[anz].yd = ((yd>0) ? yd : -yd);

      if( (yd = (long) ((ydrot*dcos + xdrot*dsin) / 0x1000000) >>16) > 0)
      {
        if( (xd = (long) ((xdrot*dcos - ydrot*dsin) / 0x1000000) >> 16) > 0)
        {
          rmax = ((xd+PRADIUS)*(xd+PRADIUS) + (yd+PRADIUS)*(yd+PRADIUS))<<2;
          rmin = ((xd-PRADIUS)*(xd-PRADIUS) + (yd-PRADIUS)*(yd-PRADIUS))<<2;
        }
        else
        {
          rmin = ((xd+PRADIUS)*(xd+PRADIUS) + (yd-PRADIUS)*(yd-PRADIUS))<<2;
          rmax = ((xd-PRADIUS)*(xd-PRADIUS) + (yd+PRADIUS)*(yd+PRADIUS))<<2;
        }

        xd *=  SCREEN - FLUCHT;
        xd /= (yd - FLUCHT);
        h1 = (SCREEN - FLUCHT) * SHOTHEIGHT;
        h1 /= (yd - FLUCHT);

        xd<<=12; h1<<=12;

        walls[anz].x2 = walls[anz].x1 = (int) xd;
        walls[anz].h2 = walls[anz].h1 = (int) h1;
        walls[anz].rmax = rmax;
        walls[anz].rmin = rmin;
        walls[anz++].ident = i+32; /* 32=MAXPLAYER*/
      }
    }
  }

  return anz;
}

static int wall_3d(long x1,long y1,long x2,long y2,int ident,WALL *wall)
{
  long rmax,rmin,x,r,h1,h2;

  x1 >>= 16; x2 >>= 16; y1 >>= 16; y2 >>= 16;

  /* wrong in texturemode */

  wall->clipped = 0;
  if ((y1 > 0) && (y2 < 0))
  {
     wall->clipped = 2;
     x  =  y1 * (x2 - x1);
     x2 = x1 + x / (y1 - y2);
     y2 = 0;
  }
  else if ((y2 > 0) && (y1 < 0))
  {
     wall->clipped = 1;
     x = y2 * (x1 - x2);
     x1 = x2 + x / (y2 - y1);
     y1 = 0;
  }

  if( (rmax=(x1*x1+y1*y1)<<2) < (rmin=(x2*x2+y2*y2)<<2) )
  {
    r = rmax; rmax = rmin; rmin = r;
  }

  if((y1 >= 0) && (y2 >= 0))
  {
    x1 *= (SCREEN - FLUCHT)<<8;
    x2 *= (SCREEN - FLUCHT)<<8;
    x1 /= (y1 - FLUCHT);
    x2 /= (y2 - FLUCHT);
    h1 = (SCREEN - FLUCHT) * WALLHEIGHT * 4096;
    h2 = (SCREEN - FLUCHT) * WALLHEIGHT * 4096;
    h1 /= (y1 - FLUCHT);
    h2 /= (y2 - FLUCHT);

    x1<<=4; x2<<=4;

    if(x1 < x2)
    {
      if(wall->clipped == 2)
        wall->clipped = 0;
      if ((x1 < XMAX) && (x2 > XMIN))
      {
        wall->x1 = (int) x1; wall->x2 = (int) x2;
        wall->h1 = (int) h1; wall->h2 = (int) h2;
        wall->rmax = rmax;     wall->rmin = rmin;
        wall->ident = ident;
        return TRUE;
      }
    }
    else if ((x2 < XMAX) && (x1 > XMIN))
    {
      if(wall->clipped == 1)
        wall->clipped = 0;
      wall->x2 = (int) x1; wall->x1 = (int) x2;
      wall->h2 = (int) h1; wall->h1 = (int) h2;
      wall->rmax = rmax;     wall->rmin = rmin;
      wall->ident = ident;
      return TRUE;
    }
  }

  return FALSE;
}

/***************************************
 * Resort Walls/Smilies
 *
 * warning: xd,yd are absolut; this COULD cause problems
 */

static void resort(WALL *f,int n)
{
  int i,j,c;
  WALL s;

  for(i=n-1;i>=0;i--)
  {
    for(j=i-1,c=0;j>=0;j--,c++)
    {
      if(c > 100)
      {
        fprintf(stderr,"R?");
        return;
      }

      if(f[i].rmin > f[j].rmax) break;
      if(stest(&f[i],&f[j]))
      {
        s = f[i]; f[i] = f[j] ; f[j] = s; /* swappen */
        if((f[i].xd == f[j].xd) && (f[i].yd == f[j].yd))
        {
         /* sm->debug = 2; */
          printf("E?\n");
          break;
        }
        j = i;
        continue; /* second loop again */
      }
    }
  }
}

/******************************
 * Priotity-Test (very ugly and buggy)
 */

static int stest(WALL *w1,WALL *w2)
{
  long d;
  int xr1,xl1,xr2,xl2;

  if(w1->ident < 0x100)
  {
    xl1 = w1->x1 - (w1->h1>>1);
    xr1 = w1->x1 + (w1->h1>>1);
  }
  else
  {
    xl1 = w1->x1;
    xr1 = w1->x2;
  }
  if(w2->ident < 0x100)
  {
    xl2 = w2->x1 - (w2->h1>>1);
    xr2 = w2->x1 + (w2->h1>>1);
  }
  else
  {
    xl2 = w2->x1;
    xr2 = w2->x2;
  }

  if((xl1 >= xr2) || (xr1 <= xl2)) return(FALSE);

  if( ((w1->ident < 0x100) && (w2->ident < 0x100)) || ((w1->ident >= 0x100) && (w2->ident >= 0x100)) )
  {
    return(w1->rmax < w2->rmax);
  }

  if(sm->debug)
  {
    printf("%x %lx %lx - %x %lx %lx\n",w1->ident,w1->xd,w1->yd,w2->ident,w2->xd,w2->yd);
    printf("%d %d %d %d %d %d\n",w1->x1,w1->x2,w1->h1,w2->x1,w2->x2,w2->h1);
    printf("* %d %d %d %d\n",xl1,xr1,xl2,xr2);
  }

  if((w1->ident & 0x100) || (w2->ident & 0x100))
  {
    if(w1->yd > w2->yd)
    {  /* 2 in front of 1 */
      d = w2->xd - w1->xd;
      if(d >= 0xc00000) return TRUE;
      return FALSE;
    }
    else
    {  /* 1 in front of 2 */
      d = w1->xd - w2->xd;
      if(d >= 0xc00000) return FALSE;
      return TRUE;
    }
  }
  else
  {
    if(w1->xd > w2->xd)
    {  /* 2 in front of 1 */
      d = w2->yd - w1->yd;
      if(d >= 0xc00000) return TRUE;
      return FALSE;
    }
    else
    {  /* 1 in front of 2 */
      d = w1->yd - w2->yd;
      if(d >= 0xc00000) return FALSE;
      return TRUE;
    }
  }
}

