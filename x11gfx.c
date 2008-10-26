/*
 * X11: init and draw
 */

#ifdef PERFORMANCE_TEST
 #include <time.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef SH_MEM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
XShmSegmentInfo shminfo;
#endif

#include "netmaze.h"
#include "bitmap.h"

#include "bitmaps/unilogo.xbm"
#include "bitmaps/smiley.xbm"

extern struct shared_struct *sm;

GC hwallgc,vwallgc,blackgc,whitegc,topgc,bottomgc,unilogogc;
static Pixmap unilogo;
static XColor blackpix,whitepix,hwallpix,vwallpix,toppix,bottompix,yellpix;

static char gamename[] = "Netmaze - The Multiplayercombatgame";
static char mapname1[] = "Rotate-Map";
static char iconname[] = "Netmaze";
static char statusname[] = "Status";
static char killname[] = "Kills";

static int smileyrad;

static void set_colors(PLAYER*);
static void gamebgtofg(void);
static GC mkmonomap(char *bitmap);
static GC mkunilogo(void);
static GC mkcolormap(char *name,XColor*);
static GC mkdithermap(char *bitmap,char *fg,char *bg);
unsigned int get_best_color(XColor *col);
static int calc_pos(int num,int len);
static int calc_fitlen(int fit,int height);
/*
static int clipit(int x1,int h1,int x2,int h2,int c);
*/

extern void init_smiley(void);
extern void draw_smiley(Drawable,short,short,short,short);

/* extern: texture.c */
extern void image_bg(int ctop,int cbottom);
extern void image_circle(int x1,int y1,int h1,int h2,int col);
extern void image_sym_vline(int x1,int h1,int col,int);
extern void image_hline(int x1,int y1,int x2,int val);
extern void image_face(int x,int  r,int win,int col);
extern void image_floor(int x,int y,int angle,struct texture *tx);
extern void image_top(int ctop);

extern struct texture *load_texture(char *);
extern void make_tabs(void);
extern void texture_wall(int x1,int h1,int x2,int h2,struct texture *tex,int,int,int,int,int);

static struct texture *textures[16];
extern int texturemem;

static int XErrorNewHandler(Display*,XErrorEvent*);
static int XErrorFlag=0;

char *infotext[] = { "Welcome to Netmaze by MH! Pre-Version V0.81-April94" , ""};

/************************************************
 * Init X11-datas, (open Display,setup gcs, ..)
 */

void x11_init(int argc,char **argv)
{
  Pixmap px;
  int i;

  sm->grafix.display /*= sm->grafix.display1*/ = XOpenDisplay ("");
  sm->grafix.display1 = XOpenDisplay ("");

  if((sm->grafix.display == NULL) || (sm->grafix.display1 == NULL))
  {
    fprintf(stderr,"%s: Can't open Display\n",argv[0]);
    exit(-1);
  }

  sm->grafix.root = DefaultRootWindow(sm->grafix.display);
  sm->grafix.screen = DefaultScreen(sm->grafix.display);
  sm->grafix.cmap = DefaultColormap(sm->grafix.display,sm->grafix.screen);
  XGetWindowAttributes(sm->grafix.display,sm->grafix.root,&sm->grafix.attribute);
  sm->grafix.gc = XCreateGC (sm->grafix.display,sm->grafix.root,0,0);

  if(sm->privatecmap)
  {
    Visual *vis;
    XColor dummy;

    vis  = DefaultVisual(sm->grafix.display,sm->grafix.screen);
    sm->grafix.cmap = XCreateColormap(sm->grafix.display,sm->grafix.root,                                        vis,AllocNone );
    XAllocNamedColor(sm->grafix.display,sm->grafix.cmap,
                     "White",&dummy,&whitepix);
    XAllocNamedColor(sm->grafix.display,sm->grafix.cmap,
                     "Black",&dummy,&blackpix);
  }
  else
  {
    blackpix.pixel = BlackPixel(sm->grafix.display,sm->grafix.screen);
    whitepix.pixel = WhitePixel(sm->grafix.display,sm->grafix.screen);
  }

  switch(sm->outputsize)
  {
    case 12:
      sm->grafix.gamewidth = sm->grafix.shint.width = 640;
      sm->grafix.gameheight = sm->grafix.shint.height = 400;
      smileyrad = 128;
      break;
    case 13:
      sm->grafix.gamewidth = sm->grafix.shint.width = 320;
      sm->grafix.gameheight = sm->grafix.shint.height = 200;
      smileyrad = 64;
      break;
    case 14:
      sm->grafix.gamewidth = sm->grafix.shint.width = 160;
      sm->grafix.gameheight = sm->grafix.shint.height = 100;
      smileyrad = 32;
      break;
    case 11:
      sm->grafix.gamewidth = sm->grafix.shint.width = 1280;
      sm->grafix.gameheight = sm->grafix.shint.height = 800;
      smileyrad = 256;
      break;
    case -1:
      sm->grafix.gamewidth = sm->grafix.shint.width = 960;
      sm->grafix.gameheight = sm->grafix.shint.height = 600;
      smileyrad = 192;
      break;
  }
  sm->grafix.shint.flags = PSize;

  sm->grafix.gamefg = XCreateSimpleWindow(sm->grafix.display,sm->grafix.root,
                      100,100,sm->grafix.shint.width,sm->grafix.shint.height,
                      5,0,blackpix.pixel);

  if(!sm->directdraw)
    sm->grafix.gamebg = XCreatePixmap (sm->grafix.display,sm->grafix.gamefg,
                        sm->grafix.gamewidth,sm->grafix.gameheight,
                        sm->grafix.attribute.depth);
  else
    sm->grafix.gamebg = sm->grafix.gamefg;

  sm->grafix.mapfg  = XCreateSimpleWindow(sm->grafix.display,sm->grafix.root,
                      100,100,MAPSIZE,MAPSIZE,5,0,blackpix.pixel);

  sm->grafix.mapbg  = XCreatePixmap(sm->grafix.display,sm->grafix.mapfg,
                      MAPSIZE,MAPSIZE,sm->grafix.attribute.depth);

  sm->grafix.sthint.width  = 200;
  sm->grafix.sthint.height = 100;
  sm->grafix.sthint.flags  = PSize;
  sm->grafix.statusfg = XCreateSimpleWindow(sm->grafix.display,sm->grafix.root,
                       100,100,sm->grafix.sthint.width,sm->grafix.sthint.height,
                        5,0,blackpix.pixel);

  sm->grafix.khint.width  = 204;
  sm->grafix.khint.height = 84;
  sm->grafix.khint.flags  = PSize;
  sm->grafix.killfg = XCreateSimpleWindow(sm->grafix.display,sm->grafix.root,
                       100,100,sm->grafix.khint.width,sm->grafix.khint.height,
                        5,0,blackpix.pixel);

  px = XCreateBitmapFromData(sm->grafix.display,sm->grafix.root,smiley_bits,
                             smiley_width,smiley_height);
  XSetStandardProperties (sm->grafix.display,sm->grafix.mapfg,mapname1,
                          mapname1,px,argv,argc,NULL);
  XSetStandardProperties (sm->grafix.display,sm->grafix.gamefg,gamename,
                          iconname,px,argv,argc,&sm->grafix.shint);
  XSetStandardProperties (sm->grafix.display,sm->grafix.statusfg,statusname,
                          statusname,px,argv,argc,&sm->grafix.sthint);
  XSetStandardProperties (sm->grafix.display,sm->grafix.killfg,killname,
                          killname,px,argv,argc,&sm->grafix.khint);

  unilogo = XCreateBitmapFromData(sm->grafix.display,sm->grafix.root,
            unilogo_bits,unilogo_width,unilogo_height);

  if(sm->privatecmap)
  {
    XSetWindowColormap(sm->grafix.display,sm->grafix.gamefg,sm->grafix.cmap);
    XSetWindowColormap(sm->grafix.display,sm->grafix.statusfg,sm->grafix.cmap);
    XSetWindowColormap(sm->grafix.display,sm->grafix.killfg,sm->grafix.cmap);
  }

  if(sm->texturemode && sm->grafix.attribute.depth < 8)
  {
    fprintf(stderr,"Sorry, but you need at least an 8bit graphic to run texturemode.\n");
    sm->texturemode = FALSE;
  }

  set_colors(sm->playfeld);

  if(sm->texturemode)
  {
    Visual *vis;
    vis  = DefaultVisual(sm->grafix.display,sm->grafix.screen);

    if(sm->outputsize< 0)
    {
      fprintf(stderr,"Sorry, in texturemode is this screensize impossible.\n");
      exit(1);
    }

    if( ((textures[0xf] = load_texture("hwall.bmp")) == NULL) ||
        ((textures[0xe] = load_texture("vwall.bmp")) == NULL) ||
        ((textures[0xd] = load_texture("floor.bmp")) == NULL) )
    {
      fprintf(stderr,"Can't load texturedata!.\n");
      sm->texturemode = FALSE;
    }
    else
    {
      for(i=1;i<10;i++)
      {
        char name[16];
        sprintf(name,"texture%d.bmp",i);
        textures[i] = load_texture(name);
        if(textures[i] == NULL)
          textures[i] = textures[0xf];
      }
      printf("Used Texturememory: %ld\n",texturemem);

#ifdef SH_MEM
      if(!sm->noshmem && !XShmQueryExtension(sm->grafix.display))
      {
        fprintf(stderr,"You're terminal doesn't support the X11 shared memory extension.\n");
        sm->noshmem = TRUE;
      }
      if(!sm->noshmem)
      {
        XSetErrorHandler(XErrorNewHandler);
        XFlush(sm->grafix.display);
        sm->grafix.ximg = XShmCreateImage(sm->grafix.display,vis,8,
                               ZPixmap,NULL,&shminfo,IMAGEWIDTH,IMAGEHEIGHT);

        if(sm->grafix.ximg == NULL)
          sm->noshmem = TRUE;
        else
        {
          int len = sm->grafix.ximg->bytes_per_line*sm->grafix.ximg->height;
          shminfo.shmid = shmget(IPC_PRIVATE,len,IPC_CREAT | 0777);
          if(shminfo.shmid < 0)
            sm->noshmem = TRUE;
          else
          {
            sm->grafix.ximg->data = sm->grafix.imagebuf = shminfo.shmaddr =
                                (char *)shmat(shminfo.shmid,0,0);
            XShmAttach(sm->grafix.display, &shminfo);
            XSync(sm->grafix.display,1); 
            if(XErrorFlag)
            {
              fprintf(stderr,"You're terminal doesn't support the X11 shared memor extension.\n");
              sm->noshmem = TRUE;
            }
            shmctl(shminfo.shmid, IPC_RMID, 0);
          }
        }
        XSetErrorHandler(NULL);
        XFlush(sm->grafix.display);
      }
#endif
      if(sm->noshmem)
      {
        sm->grafix.imagebuf = (char *) malloc(IMAGEWIDTH*IMAGEHEIGHT);
        sm->grafix.ximg = XCreateImage(sm->grafix.display,vis,8,ZPixmap,0,
                     sm->grafix.imagebuf,IMAGEWIDTH,IMAGEHEIGHT,32,IMAGEWIDTH);
        printf("Ok, using simple PutImage\n");

      }
      else printf("Ok, using faster shared memory PutImage.\n");

      make_tabs();
    }
  }

  XSelectInput (sm->grafix.display1,sm->grafix.gamefg,
                ExposureMask | KeyPressMask | ButtonReleaseMask |
                ButtonPressMask | KeyReleaseMask | StructureNotifyMask );
  if(sm->mapdraw)
    XSelectInput (sm->grafix.display1,sm->grafix.mapfg,ExposureMask);

  XSelectInput(sm->grafix.display1,sm->grafix.statusfg,
                 ExposureMask | StructureNotifyMask);
  XSelectInput(sm->grafix.display1,sm->grafix.killfg,
                 ExposureMask | StructureNotifyMask);

  if(sm->mapdraw)
    XMapRaised (sm->grafix.display,sm->grafix.mapfg);
  XMapRaised (sm->grafix.display,sm->grafix.statusfg);
  XMapRaised (sm->grafix.display,sm->grafix.gamefg);
  XMapRaised (sm->grafix.display,sm->grafix.killfg);
  XFlush(sm->grafix.display1);
  XFlush(sm->grafix.display);

  init_smiley();
}

static int XErrorNewHandler(Display *d,XErrorEvent *e)
{
  XErrorFlag = 1;
  return 0;
}


/********************************+++
 * Draw the map
 */

void draw_map(MAZE *mazeadd)
{
  int (*vfeld)[MAZEDIMENSION],(*hfeld)[MAZEDIMENSION];
  int i,j,dim;

  hfeld = mazeadd->hwalls;
  vfeld = mazeadd->vwalls;
  dim = mazeadd->xdim;

  for(i=0;i<dim+1;i++)
    for(j=0;j<dim;j++)
    {
      if(hfeld[j][i] & MAZE_TYPEMASK)
      {
        XDrawLine(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
                  j*SCALE+5,i*SCALE+5,j*SCALE+5+SCALE,i*SCALE+5);
      }
      if(vfeld[j][i] & MAZE_TYPEMASK)
      {
        XDrawLine(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
                  i*SCALE+5,j*SCALE+5,i*SCALE+5,j*SCALE+5+SCALE);
      }
    }
}

/****************************+
 * draw rotate-map
 */

void draw_rmap(PLAYER *play,XSegment *lines,int anzahl)
{
  int x;
  XFillRectangle(sm->grafix.display,sm->grafix.mapbg,blackgc,0,0,MAPSIZE,MAPSIZE);
  XDrawSegments(sm->grafix.display,sm->grafix.mapbg,sm->grafix.gc,lines,anzahl);
  /* Targets */
  for(x=0; x<sm->marks; x++)
  {
    int nr = sm->markers[x].player;
    XFillArc(sm->grafix.display,sm->grafix.mapbg,sm->pgfx[play[nr].team].xgc,
             sm->markers[x].x-2, sm->markers[x].y-2,5,5,0,360*64);
  }
  XDrawArc(sm->grafix.display,sm->grafix.mapbg,sm->grafix.gc,
           MAPSIZE/2-2,MAPSIZE/2-2,5,5,0,360*64);
  XCopyArea(sm->grafix.display,sm->grafix.mapbg,sm->grafix.mapfg,sm->grafix.gc,
            0,0,MAPSIZE,MAPSIZE,0,0);
}

/******************************
 * Draw 'game-over' Screen
 */

void draw_end(PLAYER *play,int nr)
{
  int r,i,j;

  r = smileyrad;

  XFillRectangle(sm->grafix.display,sm->grafix.gamebg,blackgc,
                  0,0,sm->grafix.gamewidth,sm->grafix.gameheight);
  XFillArc(sm->grafix.display,sm->grafix.gamebg,sm->pgfx[play[nr].team].xgc,
           (sm->grafix.gamewidth>>1)-r,(sm->grafix.gameheight>>1)-r,
           r<<1,r<<1,0,360*64);
  draw_smiley(sm->grafix.gamebg,sm->grafix.gamewidth>>1,sm->grafix.gameheight>>1,0,r);

  for(i=0,j=0;i<sm->anzplayers;i++)
  {
    if(play[nr].team == play[i].team)
    {
      XDrawImageString(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
                       5,32+j*16,play[i].name,strlen(play[i].name));
      j++;
    }
  }
  if(j == 1)
  {
    XDrawImageString(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
                   0,16,"The Winner is:",14);
  }
  else
  {
    XDrawImageString(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
                   0,16,"The Winners are:",16);
  }

  gamebgtofg();
}

/***************************************
 * Draw maze (3D)
 */

double deg(double x,double y)		/* degrees 0..256 */
{
  return (x != 0.0 ? 256.0 - 256.0/4.0 - atan(y/x)/(M_PI*2.0) * 256.0 : 0.0);
}

void draw_maze(WALL *walls,PLAYER *play,int anzahl,int nr)
{
  int id,i,r,r2,s;
  unsigned int width,high,hwidth,hhigh,x1;
  XPoint points[4];
  GC gc;

  width = sm->grafix.gamewidth;
  high  = sm->grafix.gameheight;

  if(play[nr].hitbycnt == 0)
  {
    XFillRectangle (sm->grafix.display,sm->grafix.gamebg,topgc,
                      0,0,width,high>>1);
    XFillRectangle (sm->grafix.display,sm->grafix.gamebg,bottomgc,
                      0,high>>1,width,high>>1);
  }
  else
    XFillRectangle(sm->grafix.display,sm->grafix.gamebg,
                   sm->pgfx[play[nr].hitby].xgc,0,0,width,high);

  hwidth = width >> 1;
  hhigh  = high  >> 1;

  for(i=anzahl-1;i>=0;i--)
  {
    id = walls[i].ident;

    if(id >= 0x100)
    {
      if(id & 0x100)
        gc = hwallgc;
      else
        gc = vwallgc;

/*
      if(sm->clipsides)
      {
        if(walls[i].x1+2 < walls[i].lclip)
        {
          walls[i].h1 = clipit(walls[i].x1,walls[i].h1,
                               walls[i].x2,walls[i].h2,walls[i].lclip);
          walls[i].x1 = walls[i].lclip;
        }
        if(walls[i].x2-2 > walls[i].rclip)
        {
          walls[i].h2 = clipit(walls[i].x1,walls[i].h1,
                               walls[i].x2,walls[i].h2,walls[i].rclip);
          walls[i].x2 = walls[i].rclip;
        }
      }
      static int clipit(int x1,int h1,int x2,int h2,int c)
      {
        double t;

        t = (double) (x2 - c) / (double) (x2 - x1);
        h1 = h2 - (int) ((double) (h2 - h1) * t);
        return h1;
      }
*/
      if((s=sm->outputsize) > 0)
      {
         points[0].x=hwidth+(walls[i].x1>>s);points[0].y=hhigh+(walls[i].h1>>s);
         points[1].x=hwidth+(walls[i].x2>>s);points[1].y=hhigh+(walls[i].h2>>s);
         points[2].x=hwidth+(walls[i].x2>>s);points[2].y=hhigh-(walls[i].h2>>s);
         points[3].x=hwidth+(walls[i].x1>>s);points[3].y=hhigh-(walls[i].h1>>s);
      }
      else /* big */
      {
         points[0].x=hwidth+(walls[i].x1>>13)+(walls[i].x1>>12);
         points[0].y=hhigh+(walls[i].h1>>13)+(walls[i].h1>>12);
         points[1].x=hwidth+(walls[i].x2>>13)+(walls[i].x2>>12);
         points[1].y=hhigh+(walls[i].h2>>13)+(walls[i].h2>>12);
         points[2].x=hwidth+(walls[i].x2>>13)+(walls[i].x2>>12);
         points[2].y=hhigh-(walls[i].h2>>13)-(walls[i].h2>>12);
         points[3].x=hwidth+(walls[i].x1>>13)+(walls[i].x1>>12);
         points[3].y=hhigh-(walls[i].h1>>13)-(walls[i].h1>>12);
      }

      XFillPolygon (sm->grafix.display,sm->grafix.gamebg,gc,
                    points,4,Convex,CoordModeOrigin);

      if(walls[i].x1 == walls[i].lclip)
        XDrawLine(sm->grafix.display,sm->grafix.gamebg,whitegc,
                  points[0].x,points[0].y,points[3].x,points[3].y);
      if(walls[i].x2 == walls[i].rclip)
        XDrawLine(sm->grafix.display,sm->grafix.gamebg,whitegc,
                  points[1].x,points[1].y,points[2].x,points[2].y);
    }
    else if(id < 64)
    {
      if((s=sm->outputsize)>0)
      {
        r2 = (r = (walls[i].h1>>s))>>1;
        x1 = walls[i].x1>>s;
      }
      else
      {
        r2 = (r = (walls[i].h1>>13)+walls[i].h1)>>13;
        x1 = (walls[i].x1>>13)+(walls[i].x1>>12);
      }

      if(id < 32)
      {
        if(play[id].hitbycnt == 0)
        {
          XFillArc(sm->grafix.display,sm->grafix.gamebg,
                   sm->pgfx[play[id].team].xgc,
                   x1+hwidth-r2,hhigh-r2,r,r,0,360*64);
        }
        else
        {
          XFillArc(sm->grafix.display,sm->grafix.gamebg,
                   sm->pgfx[play[id].hitby].xgc,
                   x1+hwidth-r2,hhigh-r2,r,r,0,360*64);
        }
        XFillArc(sm->grafix.display,sm->grafix.gamebg,blackgc, /* shadow */
                 x1+hwidth-r2+(r2>>2),hhigh+r2*SMSCALE-(r2>>2),
                 r-(r2>>1),r2>>1,0,360*64);
        draw_smiley(sm->grafix.gamebg,x1+hwidth,hhigh,walls[i].h2,r2);
      }
      else if(id < 64)
      {
        XFillArc(sm->grafix.display,sm->grafix.gamebg, /* shot */
                 sm->pgfx[play[id-32].team].xgc,
                 x1+hwidth-r2,hhigh-r2,r,r,0,360*64);
        XFillArc(sm->grafix.display,sm->grafix.gamebg,blackgc, /* shadow */
                 x1+hwidth-r2+(r2>>2),
                 hhigh+r2*STSCALE,r-(r2>>1),r2>>1,0,360*64);
      }
    }
  }

  if(play[nr].hide)
  {
    XDrawLine(sm->grafix.display,sm->grafix.gamebg,blackgc,
              hwidth-5,hhigh,hwidth+5,hhigh);
    XDrawLine(sm->grafix.display,sm->grafix.gamebg,blackgc,
              hwidth,hhigh-5,hwidth,hhigh+5);
  }
  else
  {
    XDrawLine(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
              hwidth-5,hhigh,hwidth+5,hhigh);
    XDrawLine(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
              hwidth,hhigh-5,hwidth,hhigh+5);
  }

/*
	Follow arrow.
	look at winkel, and player follow winkel,
	and distance to player.
*/
  if((nr != play[nr].follow) && (play[nr].follow != -1) && sm->locator)
  {
    PLAYER *me   = play+nr;
    PLAYER *them = play+me->follow;
    int mx,my;
    int tx,ty;
    int dx,dy;
    static int count=0;
    int angle, diff;

    if (me && them)
    {
      mx = (int) ((me->x) >> 24);
      my = (int) ((me->y) >> 24);
      tx = (int) ((them->x) >> 24);
      ty = (int) ((them->y) >> 24);
      dx = mx-tx;
      dy = my-ty;

      count++;
      /*
       ** Angle from me to target,
       ** map-based angle.
       */
      angle = (int)deg((double)dy,(double)dx);

      /*
       ** Now its me-based
       */
      diff = angle;
      diff -= 128;
#define max(a,b) ((a)>(b)?(a):(b))

      /*
       * update telltale.
       */
      {
        int dist = sqrt((double)(dx*dx + dy*dy))*6.0;
        if ((count%8) < 4)
        {
          int radius = max(1,(50 - dist));
          XDrawArc(sm->grafix.display,sm->grafix.gamebg,
                   sm->pgfx[play[me->follow].team].xgc,
                   hwidth-radius/2,hhigh-radius/2,radius, radius, 0, 360*64);
        }
      }
    }
  }
  gamebgtofg();
}

void draw_texture_maze(WALL *walls,PLAYER *play,int anzahl,int nr)
{
  int id,i,r,r2,s;
  unsigned int width,height,x1;
  int x,y;
  unsigned int pixel;
#ifdef PERFORMANCE_TEST
  int a;
#endif

/*
  if(play[nr].hitbycnt == 0)
    image_bg(toppix.pixel,hwallpix.pixel);
  else
    image_bg(sm->pgfx[play[nr].hitby].col.pixel,
             sm->pgfx[play[nr].hitby].col.pixel);
*/
  if(play[nr].hitbycnt == 0)
    image_top(toppix.pixel);
  else
    image_top(sm->pgfx[play[nr].hitby].col.pixel);
  image_floor(play[nr].x,play[nr].y,play[nr].winkel,textures[0xd]);


#ifdef PERFORMANCE_TEST
  a = clock();
#endif
  for(i=anzahl-1;i>=0;i--)
  {
    if((id = walls[i].ident) >= 0x100)
    {
      int tnr=(id>>10)&0xf;
      texture_wall(walls[i].x1,walls[i].h1,walls[i].x2,walls[i].h2,
                   textures[tnr],walls[i].lclip,walls[i].rclip,
                   sm->outputsize,walls[i].clipped,0);

      if(walls[i].x1 == walls[i].lclip)
        image_sym_vline(walls[i].x1,walls[i].h1,
                          whitepix.pixel,sm->outputsize);
      if(walls[i].x2 == walls[i].rclip)
        image_sym_vline(walls[i].x2,walls[i].h2,
                          whitepix.pixel,sm->outputsize);
    }
    else if(id < 64)
    {
      if((s=sm->outputsize)>0)
      {
        r2 = (r = (walls[i].h1>>s))>>1;
        x1 = walls[i].x1>>s;
      }
      else
      {
        r2 = (r = (walls[i].h1>>13)+walls[i].h1)>>13;
        x1 = (walls[i].x1>>13)+(walls[i].x1>>12);
      }
      if(id < 32)
      {
        if(play[id].hitbycnt == 0)
          image_circle(x1,0,r,r,sm->pgfx[play[id].team].col.pixel);
        else 
          image_circle(x1,0,r,r,sm->pgfx[play[id].hitby].col.pixel);
        image_circle(x1,r2*SMSCALE,r2>>1,r-(r2>>1),1);
        image_face(x1,r2,walls[i].h2,whitepix.pixel);
      }
      else if(id < 64)
      {
        image_circle(x1,0,r,r,sm->pgfx[play[id-32].team].col.pixel);
        image_circle(x1,r2*STSCALE,r2>>1,r-(r2>>1),1);
      }
    }
  }
#ifdef PERFORMANCE_TEST
  a = clock()-a; fprintf(stderr,"%ld ",a);
#endif

  /* bgtofg: */
  width  = sm->grafix.shint.width;
  height = sm->grafix.shint.height;
  x = (width-sm->grafix.gamewidth)>>1; y = (height-sm->grafix.gameheight)>>1;

  if((x > 0) || (y > 0))
    XDrawRectangle(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
                   x-1,y-1,sm->grafix.gamewidth+1,sm->grafix.gameheight+1);

  if(play[nr].hide)
    pixel = blackpix.pixel;
  else
    pixel = yellpix.pixel;

  image_hline((IMAGEWIDTH>>1)-5,(IMAGEHEIGHT>>1)-1,(IMAGEWIDTH>>1)+6,blackpix.pixel);
  image_hline((IMAGEWIDTH>>1)-5,(IMAGEHEIGHT>>1)+1,(IMAGEWIDTH>>1)+6,blackpix.pixel);
  image_sym_vline(-1,5,blackpix.pixel,0);
  image_sym_vline(1,5,blackpix.pixel,0);

  image_hline((IMAGEWIDTH>>1)-5,(IMAGEHEIGHT>>1),(IMAGEWIDTH>>1)+6,pixel);
  image_sym_vline(0,5,pixel,0);

#ifdef SH_MEM
  if(!sm->noshmem)
  {
    XShmPutImage(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
       sm->grafix.ximg,(IMAGEWIDTH-WXSIZE)>>1,(IMAGEHEIGHT-WYSIZE)>>1,
       x,y,sm->grafix.gamewidth,sm->grafix.gameheight,True);
  }
  else
#endif
  {
    XPutImage(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
       sm->grafix.ximg,(IMAGEWIDTH-WXSIZE)>>1,(IMAGEHEIGHT-WYSIZE)>>1,
       x,y,sm->grafix.gamewidth,sm->grafix.gameheight);
  }

  XSync(sm->grafix.display,1);
}

/*********************************************
 * Copy background-pixmap to the foreground
 */

static void gamebgtofg(void)
{
  int x,y;
  unsigned int height,width;

  if(sm->directdraw)
  {
    XSync(sm->grafix.display,1);
    return;
  }

  width  = sm->grafix.shint.width;
  height = sm->grafix.shint.height;

  x = (width-sm->grafix.gamewidth) >> 1; y = (height-sm->grafix.gameheight) >> 1;
  if((x > 0) || (y > 0))
  {
    XDrawRectangle(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
                   x-1,y-1,sm->grafix.gamewidth+1,sm->grafix.gameheight+1);
  }
  XCopyArea(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gamefg,blackgc,
            0,0,sm->grafix.gamewidth,sm->grafix.gameheight,x,y);

  XSync(sm->grafix.display,1);
}

/***********************************
 * Draw 'Killed by' -Screen
 */

void draw_kill(PLAYER *play,int k)
{
  int r;
  char string[32];

  strcpy(string,play[k].name);
  strcpy(string+strlen(string)," says:");

  r = smileyrad;

  XFillRectangle(sm->grafix.display,sm->grafix.gamebg,blackgc,
                 0,0,sm->grafix.gamewidth,sm->grafix.gameheight);
  XFillArc(sm->grafix.display,sm->grafix.gamebg,sm->pgfx[play[k].team].xgc,
                 (sm->grafix.gamewidth>>1)-r,(sm->grafix.gameheight>>1)-r,
                 r<<1,r<<1,0,360*64);
  draw_smiley(sm->grafix.gamebg,sm->grafix.gamewidth>>1,sm->grafix.gameheight>>1,0,r);
  XDrawImageString(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
                   0,16,string,strlen(string));
  XDrawImageString(sm->grafix.display,sm->grafix.gamebg,sm->grafix.gc,
                   0,32,play[k].comment,strlen(play[k].comment));
  gamebgtofg();
}

/***************************+++
 * Draw Status
 */

void draw_status(int nr,PLAYER *play)
{
  int i,j,k;
  int len,fitpos,step,len1;
  int width = sm->grafix.sthint.width;
  int height = sm->grafix.sthint.height;
  XPoint points[1024];

  if(nr >= 0)
  {
    if(!play[nr].statchg && !sm->statchg) return;
  }
  else if(!sm->statchg) return;

  fitpos = width - 15;
  if(fitpos < 0) fitpos = 0;
  step = height / sm->numteams;
  if(step > 16) step = 16;
  len = width - 30;
  if(len < 0) len = 0;

  if(sm->statchg)
  {
    XFillRectangle(sm->grafix.display,sm->grafix.statusfg,blackgc,0,0,width,height);

    for(i=1,k=0;i<=sm->numteams;i++)
      for(j=1;j<=sm->config.kills2win;j++)
        if(k<1024)
        {
          points[k].x = calc_pos(j,len);
          points[k].y = i*step-1;
          k++;
        }
    XDrawPoints(sm->grafix.display,sm->grafix.statusfg,whitegc,
                points,k,CoordModeOrigin);

    for(i=0;i<sm->numteams;i++) /* numteams */
    {
      XFillRectangle(sm->grafix.display,sm->grafix.statusfg,sm->pgfx[i].xgc,
                     0,i*step,calc_pos(sm->teams[i].kills,len),step-1);
    }
    if(nr >= 0)
    {
      len1 = calc_fitlen(play[nr].fitness,height);
      XFillRectangle(sm->grafix.display,sm->grafix.statusfg,
                     sm->pgfx[play[nr].team].xgc,fitpos,height-len1,10,len1);
    }
 /*   sm->statchg = FALSE; */
  }
  else
  {
    len1 = calc_fitlen(play[nr].fitness,height);
    XFillRectangle(sm->grafix.display,sm->grafix.statusfg,blackgc,
                   fitpos,0,10,height-len1);
    XFillRectangle(sm->grafix.display,sm->grafix.statusfg,
                   sm->pgfx[play[nr].team].xgc,fitpos,height-len1,10,len1);
  }
}

static int calc_pos(int num,int len)
{
  return ((int) ( (double) num / (double) (sm->config.kills2win?sm->config.kills2win:1) * (double) len));
}

static int calc_fitlen(int fit,int height)
{
  int i;
  i = (int) (( (int) fit * (int) height) / 2000) ;
  return (i<0)?0:i;
}

/******************************
 * Draw Kills
 */

void draw_kills(int nr,PLAYER *play)
{
  int i;

  if(sm->killchg)
  {
    /* JG Hack....  10/24/96... Was:
    XFillRectangle(sm->grafix.display,sm->grafix.killfg,blackgc,0,0,200,80);
    */
    XFillRectangle(sm->grafix.display,sm->grafix.killfg,blackgc,0,0,200,800);
    for(i=0;i<play[nr].killanz;i++)
    {
      XFillArc(sm->grafix.display,sm->grafix.killfg,
               sm->pgfx[play[play[nr].killtable[i]].team].xgc,
               (i%5)*40+4,(i/5)*40+4,36,36,0,360*64);
      draw_smiley(sm->grafix.killfg,(i%5)*40+22,(i/5)*40+22,0,18);
    }
    sm->killchg = FALSE;
  }
  else if(play[nr].killchg)
  {
    if( (i = play[nr].killanz-1) < 0) return;
      XFillArc(sm->grafix.display,sm->grafix.killfg,
               sm->pgfx[play[play[nr].killtable[i]].team].xgc,
               (i%5)*40+4,(i/5)*40+4,36,36,0,360*64);
      draw_smiley(sm->grafix.killfg,(i%5)*40+22,(i/5)*40+22,0,18);
  }
}


/******************************
 * Draw Info
 */

void draw_info(void)
{
  int i;
  int x,y;
  unsigned int height,width;

  width  = sm->grafix.shint.width;
  height = sm->grafix.shint.height;

  if((sm->screendraw) || (sm->winnerdraw))
  {
    x = (width-sm->grafix.gamewidth)>>1; y = (height-sm->grafix.gameheight)>>1;
    if((x <= 0) && (y <= 0)) return;

    XFillRectangle(sm->grafix.display,sm->grafix.gamefg,unilogogc,
                   0,0,sm->grafix.shint.width,y);
    XFillRectangle(sm->grafix.display,sm->grafix.gamefg,unilogogc,
                   0,y,x,sm->grafix.gameheight);
    XFillRectangle(sm->grafix.display,sm->grafix.gamefg,unilogogc,
                   x+sm->grafix.gamewidth,y,x,sm->grafix.gameheight);
    XFillRectangle(sm->grafix.display,sm->grafix.gamefg,unilogogc,
                   0,y+sm->grafix.gameheight,width,y);
  }
  else
  {
    XFillRectangle(sm->grafix.display,sm->grafix.gamefg,unilogogc,
                   0,0,sm->grafix.shint.width,sm->grafix.shint.height);

  }

  for(i=0;strlen(infotext[i])!=0;i++)
  {
    XDrawImageString(sm->grafix.display,sm->grafix.gamefg,sm->grafix.gc,
                     5,16*(i+1),infotext[i],strlen(infotext[i]));
  }
}

/******************************
 * Setup Colors
 */

static void set_colors(PLAYER *players)
{
  XColor dummy;
  int i,j;

  char *colornames[] = { "Yellow" ,"Red" , "Green" , "Blue" , "SandyBrown" ,
                         "DarkOliveGreen", "Orange" , "DarkOrchid" , "Pink",
                         "VioletRed" , "LightBlue" , "LightCyan" ,
                         "RosyBrown" , "IndianRed" , "DeepPink" , "LightPink" ,
                         "yellowGreen" ,"orchid" , "lavender" , "lemonchiffon" ,
                         "YellowGreen" , "khaki" , "DarkKhaki" , "violet" ,
                         "plum" , "DarkOrchid" , "DarkViolet" , "PaleTurquoise",
                         "Turquoise" , "darkturquoise" , "coral" , "Black" ,NULL};


  char *dithercolors[] = {  "Yellow", "Red", "Green", "Blue", "cyan",
                             "magenta", "white", "black", NULL };
  char **colorpoint = colornames;
  char **bitmappoint = player_bits;
  char **ditherpoint = dithercolors;

  if(sm->graymode && (sm->grafix.attribute.depth >= 4))
  {
    for(j=0,i=0;i<MAXPLAYERS;i++,bitmappoint++,j++)
    {
      if(*bitmappoint == NULL) bitmappoint--;
      sm->pgfx[i].xgc = mkmonomap(*bitmappoint);
    }
    hwallgc  = mkcolormap("DimGray",&hwallpix);
    vwallgc  = mkcolormap("Gray",&vwallpix);
    topgc    = mkcolormap("LightGray",&toppix);
    bottomgc = mkcolormap("DarkGrey",&bottompix);

    XSetForeground(sm->grafix.display,sm->grafix.gc,whitepix.pixel);
    yellpix.pixel = whitepix.pixel;
  }
  else if((sm->grafix.attribute.depth >= 8) && !sm->monomode && !sm->dithermode)
  {
    hwallgc  = mkcolormap("DimGray",&hwallpix);
    vwallgc  = mkcolormap("Gray",&vwallpix);
    topgc    = mkcolormap("cornflowerblue",&toppix);
    bottomgc = mkcolormap("steelblue",&bottompix);
    for(j=0,i=0;i<MAXPLAYERS;i++,colorpoint++,j++)
    {
      if(*colorpoint == NULL) colorpoint--;
      sm->pgfx[i].xgc = mkcolormap(*colorpoint,&sm->pgfx[i].col);
    }
    XAllocNamedColor(sm->grafix.display,sm->grafix.cmap,"Yellow",&dummy,&yellpix);
    XSetForeground(sm->grafix.display,sm->grafix.gc,yellpix.pixel);
  }
  else if((sm->grafix.attribute.depth >= 6) && !sm->monomode)
  {
    hwallgc = mkcolormap("DimGray",&hwallpix);
    vwallgc = mkcolormap("Gray",&vwallpix);
    topgc   = mkcolormap("cornflowerblue",&toppix);
    bottomgc = mkcolormap("steelblue",&bottompix);

    colorpoint = dithercolors;

    for(j=0,i=0;i<MAXPLAYERS;i++,colorpoint++,j++)
    {
      if(*colorpoint == NULL)
      {
        colorpoint = dithercolors;
        ditherpoint++;
        if (ditherpoint == NULL)
          ditherpoint--;
      }
      sm->pgfx[i].xgc = mkdithermap(hwall_bits, *colorpoint, *ditherpoint);
    }

    XAllocNamedColor(sm->grafix.display,sm->grafix.cmap,"Yellow",&dummy,&yellpix);
    XSetForeground(sm->grafix.display,sm->grafix.gc,yellpix.pixel);
  }
  else
  {
    for(j=0,i=0;i<MAXPLAYERS;i++,bitmappoint++,j++)
    {
      if(*bitmappoint == NULL) bitmappoint--;
      sm->pgfx[i].xgc = mkmonomap(*bitmappoint);
    }
    hwallgc  = mkmonomap(hwall_bits);
    vwallgc  = mkmonomap(vwall_bits);
    topgc    = mkmonomap(bgtop_bits);
    bottomgc = mkmonomap(bgbottom_bits);

    XSetForeground(sm->grafix.display,sm->grafix.gc,whitepix.pixel);
  }

  unilogogc = mkunilogo();

  blackgc = XCreateGC(sm->grafix.display,sm->grafix.root,0,0);
  XSetForeground(sm->grafix.display,blackgc,blackpix.pixel);
  whitegc = XCreateGC(sm->grafix.display,sm->grafix.root,0,0);
  XSetForeground(sm->grafix.display,whitegc,whitepix.pixel);

  XSetBackground(sm->grafix.display,sm->grafix.gc,blackpix.pixel);
}

/****************************
 * Setup unilogo-gc
 */

static GC mkunilogo(void)
{
  Pixmap bitmp;
  GC gc;

  bitmp = XCreateBitmapFromData(sm->grafix.display,sm->grafix.root,unilogo_bits,
                                unilogo_width,unilogo_height);
  gc = XCreateGC (sm->grafix.display,sm->grafix.root,0,0);
  XSetStipple(sm->grafix.display,gc,bitmp);
  XSetFillStyle(sm->grafix.display,gc,FillOpaqueStippled);
  XSetForeground(sm->grafix.display,gc,whitepix.pixel);
  XSetBackground(sm->grafix.display,gc,blackpix.pixel);
  return gc;
}

/*********************************++
 * Setup Bitmap-Mono-gcs
 */

static GC mkmonomap(char *bitmap)
{
  Pixmap bitmp;
  GC gc;

  bitmp = XCreateBitmapFromData(sm->grafix.display,sm->grafix.root,bitmap,16,16);
  gc = XCreateGC (sm->grafix.display,sm->grafix.root,0,0);
  XSetStipple(sm->grafix.display,gc,bitmp);
  XSetFillStyle(sm->grafix.display,gc,FillOpaqueStippled);
  XSetForeground(sm->grafix.display,gc,whitepix.pixel);
  XSetBackground(sm->grafix.display,gc,blackpix.pixel);
  return gc;
}

/***********************************
 * Setup color-gcs
 */

static GC mkcolormap(char *name,XColor *col)
{
  XColor dummy,color;
  GC gc;
  Status s;

  if(!sm->texturemode) 
  {
    s=XAllocNamedColor(sm->grafix.display,sm->grafix.cmap,name,&dummy,&color);
    if(!s)
      fprintf(stderr, "Warning: Cannot allocate colormap entry for \"%s\"\n",name);
  }
  else
  {
    XLookupColor(sm->grafix.display,sm->grafix.cmap,name,&color,&dummy);
    get_best_color(&color);
  }

  gc = XCreateGC (sm->grafix.display,sm->grafix.root,0,0);
  XSetForeground (sm->grafix.display,gc,color.pixel);
  if(col)
    *col = color;

  return gc;
}

/********************************
 * dither-gcs
 */

static GC mkdithermap(char *bitmap, char *fg, char *bg)
{
  Pixmap bitmp;
  XColor dummy,color;
  GC gc;
  Status s;

  bitmp = XCreateBitmapFromData(sm->grafix.display,sm->grafix.root,bitmap,16,16);
  gc = XCreateGC (sm->grafix.display,sm->grafix.root,0,0);
  XSetStipple(sm->grafix.display,gc,bitmp);
  XSetFillStyle(sm->grafix.display,gc,FillOpaqueStippled);

  s=XAllocNamedColor (sm->grafix.display, sm->grafix.cmap,fg,&dummy,&color);
  if (s == 0)
    fprintf(stderr, "Warning: Cannot allocate colormap entry for \"%s\"\n",fg);
  XSetForeground(sm->grafix.display,gc, color.pixel);

  s=XAllocNamedColor (sm->grafix.display, sm->grafix.cmap,bg,&dummy,&color);
  if (s == 0)
    fprintf(stderr, "Warning: Cannot allocate colormap entry for \"%s\"\n",bg);
  XSetBackground(sm->grafix.display,gc, color.pixel);
  return gc;
}

/***************************************** 
 * find a good color: slow and ugly but works ... 
 */

unsigned int get_best_color(XColor *col)
{
#define MAX_COLORS 1024
#define SHFT 12
  static unsigned char cfield[MAX_COLORS][4];
  static unsigned int pfield[MAX_COLORS];
  static int outofcol=0;
  static int num=0;
  int i,best=0;
  unsigned char r,g,b;
  int d,diff=0x7fffffff;

  r=(col->red>>SHFT)&0xff; g=(col->green>>SHFT)&0xff; b=(col->blue>>SHFT)&0xff;
  
  if(!outofcol) 
  {
    for(i=0;i<num;i++)
      if((cfield[i][0] == r) && (cfield[i][1] == g) && (cfield[i][2] == b))
        return pfield[i];

    col->flags = DoRed | DoGreen | DoBlue;
    if(!XAllocColor(sm->grafix.display,sm->grafix.cmap,col))
      outofcol=TRUE;
    else
    {
      r=(col->red>>SHFT)&0xff; 
      g=(col->green>>SHFT)&0xff; 
      b=(col->blue>>SHFT)&0xff;
      cfield[num][0] = r; cfield[num][1] = g; cfield[num][2] = b;
      pfield[num] = col->pixel;
      num++;
      if(num == MAX_COLORS)
        outofcol=1;
      return col->pixel;
    }
  }

  for(i=0;i<num;i++)
  {
    d = ((int) r - (int) cfield[i][0])*((int) r - (int) cfield[i][0]) + 
        ((int) g - (int) cfield[i][1])*((int) g - (int) cfield[i][1]) + 
        ((int) b - (int) cfield[i][2])*((int) b - (int) cfield[i][2]);
    if(d == 0)
      break;
    if(d < diff)
    {      
      diff = d;
      best = i;
    }
  }

  return pfield[best];
}

