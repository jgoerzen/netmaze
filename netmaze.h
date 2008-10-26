#include <sys/param.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/*********************/

#include "config.h"   /* check out config.h for configuration */

/*********************/

#define NETMAZEPORT 12346

#define RS6000 1

#define TRUE  1
#define FALSE 0

#define DRAWWAIT 1       /* 1 = Draw 3D-view every 'heart-beat' (if possible)*/
#define DRAWTIME 36000L  /* time per 'heart-beat' in microsec */

#define WINKEL_STEP 4    /* Rotate-Step (don't change) */
#define MAX_WINKEL 256   /* maximum-angel (don't change) */

#define XMAX 320*4096	/* internal x-resolution (right-border)  */
#define XMIN -320*4096	/* internal X-resolution (left-border)*/

#define IMAGEWIDTH 1280
#define IMAGEHEIGHT 810

#define WXSIZE (sm->grafix.gamewidth)
#define WYSIZE (sm->grafix.gameheight)
#define WXHALF (WXSIZE>>1)
#define WYHALF (WYSIZE>>1)

#define SCREEN 360	/* relativ position of the Screen (3D) */
#define FLUCHT (-80)	/*  */
#define WALLHEIGHT 80   /* wall-height / 2  */
#define SMILEYHEIGHT 100 /* Smileyheight */
#define SHOTHEIGHT 40   /* shotheight */
#define SMSCALE 1.6
#define STSCALE 4.0

#define MAXPLAYERS 32	 /* max. players (don't change) */
#define MAXSHOTS 16
#define MAZEDIMENSION 64 /* Groesse eines Mazes (maximal) */
#define BPL 128		 /* Bytes per Line (Maze) */
#define SCALE 20	 /* Vergroesserungsfaktor (Screen) */

#define SICHTWEITE 11	/* visual range in fields */

#define JOY_UP     0x01	/* OR-Codes for Joy-Values */
#define JOY_DOWN   0x02
#define JOY_LEFT   0x04
#define JOY_RIGHT  0x08
#define JOY_RADAR  0x10
#define JOY_HIDE   0x20
#define JOY_SLOW   0x40
#define JOY_BUTTON 0x80

#define PRADIUS 0x40
#define SRADIUS 0x20

#define SHOTPOWER 1000 /* classic */
#define STARTFITNESS 2000
#define RECHARGETIME 36 /* classic */
#define REFITSPEED  2
#define DEADTIME 200

#define WINNERANZ 10

#define MAXNAME 16
#define MAXCOMMENT 70

#define MAPSIZE 100

/* GAME-MODES ('-' = not used) */
#define GM_CLASSIC         0x0000 /* no options selectd */
#define GM_REFLECTINGSHOTS 0x0001 /* shots can bounce */
#define GM_DECAYINGSHOTS   0x0002 /* shots lose power */
#define GM_MULTIPLESHOTS   0x0004 /* allow more than one shot */
#define GM_WEAKINGSHOTS    0x0008 /* shooting weaks the player */
#define GM_REPOWERONKILL   0x0010 /* - repower player, if he kills an enemy */
#define GM_FASTRECHARGE    0x0020 /* faster recharg */
#define GM_FASTWALKING     0x0040 /* double-speed-walking */
#define GM_SHOWGHOST       0x0080 /* - ghostmode */
#define GM_ALLOWHIDE       0x0100 /* allow hideing */
#define GM_ALLOWRADAR      0x0200 /* allow radar */
#define GM_DESTRUCTSHOTS   0x1000 /* - destruct shots on kill */
#define GM_DECSCORE        0x2000 /* - decrease score of a killed player */
#define GM_RANKSCORE       0x4000 /* - rankingdependend score-bonus */
#define GM_TEAMSHOTHURT   0x8000 /* Shots from others on team hurt */

/* maze-data defines: */
#define MAZE_TYPEMASK     0x000f
#define MAZE_BLOCK_PLAYER 0x0010
#define MAZE_BLOCK_SHOT   0x0020


struct fd_mask
{
  u_int fds_bits[NOFILE/32+1];
};

/* Structur auf MAZE. Here is all important maze-stuff */

typedef struct {
 /*
  MAZE *prev_maze;
  MAZE *next_maze;
 */
  int hwalls[MAZEDIMENSION][MAZEDIMENSION];
  int vwalls[MAZEDIMENSION][MAZEDIMENSION];

  int xdim;
  int ydim;
  char *setlist;
  int *bitlist;
} MAZE;

/* PLAYER-Struct */

typedef struct {
  int sx;
  int sy;
  int sxd;
  int syd;
  int  salive;
  int power;
  int  next; /* next shot in chain */
  int  last; /* last shot in chain */
} SHOT;

typedef struct {
  int gamemode;
  int shots;
  int hits;
  int kills;
  int ownhits;
  int ownkills;
  int hitpnt;
  int hitby[64];
  int hitpower[64];
} STATISTIC;

typedef struct {
  short shot_power;
  short shot_weaking;
  short bounce_decay; /* 16384 = 1.0 */
  short shot_decay;   /* ! */
  short startfitness; /* ! */
  short deadtime;     /* ! */
  short max_shots;
  short repower; /* ! */
  short rechargetime; /* ! */
  short recolour_delay; /* ! */
  short angle_step; /* ! */
  short kills2win; /* ! */
  int   divider; /* ! */
  int   armageddon; /* time before armageddon is activated */
} CONFIG;

typedef struct {
  char name[MAXNAME+1];
  char comment[MAXCOMMENT+1];
  int team;
  int x;
  int y;
  int winkel;
  int fitness;
  int follow;
  int hide,radar;
  int ownkills;       /* number of deaths */
  int ownkiller;      /* playernumber of latest killer */
  int killanz;        /* number of kills */
  int killtable[256]; /* table of killed players */
  int alive;          /* alive? */

  SHOT shots[MAXSHOTS];
  int shotqueue1;
  int shotqueue2;
  int numofshots;

  int  recharge;
  int  hitbycnt;
  int  hitby;
  int  statchg; /* Status changed .. (redraw) */
  int  killchg;
  int  active;

  STATISTIC stat;
} PLAYER;

typedef struct {
  int kills;
  char teamname[MAXNAME+1];
} TEAM;

typedef struct {
  int x1,h1;
  int x2,h2;
  int ident;
  int rclip,lclip;
  int xd,yd;
  int rmax,rmin;
  int  clipped; /* need for texture */
} WALL;

typedef struct {
  Display    *display;
  Display    *display1;
  int        screen;
  Window     root;
  Drawable   gamefg;
  Drawable   gamebg;
  int        gamewidth;
  int        gameheight;
  Drawable   statusfg;
  Drawable   killfg;
  Drawable   mapfg;
  Drawable   mapbg;
  Colormap   cmap;
  GC         gc;
  XImage    *ximg;
  char      *imagebuf;
  XWindowAttributes attribute;
  XSizeHints shint;
  XSizeHints sthint;
  XSizeHints khint;
} GRAFIX;

typedef struct {
 GC xgc;
 GC facegc;
 XColor col;
} PGRAFIX;

struct texture 
{
  char *data;     /* buffer */
  char **datatab; /* pointertab */
  char *jmptab;   /* jumptab */
  char *jmptabh;  /* jmptab , halfheight */
  short (*divtab)[2];
  short (*divtabh)[2];
  int width;      /* texturewidth */
  int height;     /* tetxureheight */
  int hshift;     /* textureheight shift (*height) */
  int hshifth;    /* hshift, halfheight */
  int wshift;     /* widthshift */
  int jshift;     /* jumptableshift */
  int jshifth;    /* jumptableshifthalf */
  int hhalf;      /* half height */
  int whalf;      /* half width */
  int hmask;      /* height mask (height-1) */
  int wmask;      /* width mask */
  int hmaskh;     /* hmask half height */
  int precalc;    /* have precalculated datas? */
};

typedef struct mapmark {
        int     x,y;
        int     player;
} mapmark;

struct shared_struct /* the most should be volatile */
{
  int shmid;                       /* Shared Memory id */
  char ownname[MAXNAME+1];         /* Playername */
  char owncomment[MAXCOMMENT+1];   /* Comment, if you kill another one */
  char hostname[256];              /* Remotehostname */

  volatile int winner;             /* Winner-Number */
  int anzplayers;                  /* Number of Players */
  int numteams;                    /* Number of Teams */
  int shownumber,shownumbertmp;    /* Player-View-Number */
  int joynumber;                   /* Player-Joy-Number (Solo) */
  int gamemode;			   /* GAME-MODES: not supported yet */
  CONFIG config;                   /* important configurevalues */
  int armageddon;
  PLAYER playfeld[MAXPLAYERS];     /* Player-Data-Field */
  PLAYER playfeld1[MAXPLAYERS];    /* Copy */
  TEAM teams[MAXPLAYERS];          /* team-data */
  PGRAFIX pgfx[MAXPLAYERS];        /* Player-Grafix-datas */
  int sticks[MAXPLAYERS];          /* Stick/Cursor/Mouse-Data */
  int ownstick;                    /* Joystick-Data on this machine */
  MAZE std_maze;                   /* MAZE-DATA */
  GRAFIX grafix;                   /* Grafix-(X11)-Data */
  XSegment maplines[1000];         /* Map-Line-Buffer */
  int anzlines;
  int marks;                       /* # markers */
  mapmark markers[32];             /* Map markers */
  int rndshiftpos;                 /* Random */
  int rndshifttab[55];            /* more random-stuff */
  volatile unsigned int drawwait; /* delay Draw .. */

  /* flags */
  volatile int gameflag:1;             /* Game-is-running-flag */
  int exitprg:1;                       /* Exitflag */
  int bgdraw:1,redraw:1;               /* Mainwindowredrawflag */
  int screendraw:1,winnerdraw:1;       /* more Mainwindowredrawflags */
  int statchg:1,statchg1:1,statchg2:1; /* Statusredrawflags */
  int killchg:1;                       /* Killsredrawflags */
  int sologame:1,mapdraw:1,debug:1;    /* Solomode-,Mapdraw-,Debugflag */
  int newmap:1,locator:1;
  int monomode:1,graymode:1,dithermode:1; /* Screenoutputflags */
  int camera:1,directdraw:1,usesound:1;	  /* client in camera-mode,no bg,sound */
  int texturemode:1,privatecmap:1;
  int noshmem:1;
  int outputsize;                      /* Outputsize */
};

