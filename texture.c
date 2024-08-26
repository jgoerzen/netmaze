/*
 * Simple Texture test, Needs an (n^2)*(m^2) BMP (8bit) as texture-base
 * maxwidth = 256, 
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "netmaze.h"

static char *make_jump_tab(int h);
struct texture *load_texture(char *name);
void image_hline(int x1,int y1,int x2,int val);
void image_circle(int x1,int y1,int h1,int h2,int);

extern unsigned int get_best_color(XColor *col);

extern struct shared_struct *sm;
/*
static unsigned char *sqrttab;
*/
static double *circletab;
static int *floortab;
int texturemem;
extern struct texture *vtex;
extern int trigtab[];

/*
 * Draw a texture-wall.. really not fast
 * I have to optimize the shift-stuff, most of them was a quick hack.
 * that's really not the best texture methode, but it was the best way
 * to add it to the old code.
 * type: 0=full, !0=mirrored
 */

void texture_wall(int x1,int hn1,int x2,int hn2,struct texture *tex,int lclip,int rclip,int size,int clipped,int type)
{
  int j,xp,hi,k,ln,d,i1,istep;
  char *jmpt,*imgbuf,*t1,*t2;
  int h,xn1,yn1;
  char *ia1,*ia2,pixval1,pixval2;
  int shift=size-4,offset=0;

  hn1>>=4; hn2>>=4;

  if(!(d = (x2-x1)>>size) || hn1 > 0x400000 || hn2 > 0x400000)
    return;

  h = ((double) (hn2 - hn1) / d);

  xn1 = x1*WALLHEIGHT / hn1;
  yn1 = (((SCREEN-FLUCHT)*WALLHEIGHT)<<12)/hn1+(FLUCHT<<4);

  if(x1 < lclip)
  {
    hn1 += ((lclip-x1)>>size)*h;
    x1 = lclip;
  }
  if(x2 > rclip )
    x2=rclip;

  i1=x1*WALLHEIGHT;
  istep=WALLHEIGHT<<size;
  d=((x2-x1)>>size)+1;
  if(clipped)
  {
    double xd0,yd0;
    yd0 = (((SCREEN-FLUCHT)*WALLHEIGHT)<<12) / hn2+(FLUCHT<<4)-yn1;
    xd0 = (x2*WALLHEIGHT) / hn2 - xn1;
    offset = 256-(((int) sqrt(xd0*xd0+yd0*yd0))>>4);
  }

  imgbuf = sm->grafix.imagebuf + (x1>>size) 
                               + (IMAGEWIDTH>>1) + (IMAGEHEIGHT>>1)*IMAGEWIDTH;

  for(;d;d--,i1+=istep,hn1 += h)
  {
    double xd0,yd0;
    yd0 = (((SCREEN-FLUCHT)*WALLHEIGHT)<<12) / hn1+(FLUCHT<<4)-yn1;
    xd0 = i1 / hn1 - xn1;
    xp = ((((int) sqrt(xd0*xd0+yd0*yd0))>>4)+offset) & tex->wmask; 

    hi = hn1>>shift;
    ia1 = ia2 = imgbuf++;

    if(!type)
    {
      t2 = t1 = tex->data + (xp<<tex->hshift) + tex->hhalf;

      jmpt = tex->jmptabh + ((hi & tex->hmaskh)<<tex->jshifth);
      ln   = hi>>tex->hshifth;

      for(j=tex->divtabh[hi][0];j;j--)
      {
        pixval1 = *t1++;
        pixval2 = *--t2;
#if 1
        for(k=ln;(--k)>=0;)
        {
          ia1 -= IMAGEWIDTH;
          *ia1 = pixval1;
          *ia2 = pixval2;
          ia2 += IMAGEWIDTH;
        }
        if(*jmpt++)
        {
          ia1 -= IMAGEWIDTH;
          *ia1  = pixval1;
          *ia2  = pixval2;
          ia2 += IMAGEWIDTH;
        }
#else
  #include "xtests/texpix.c"
#endif
      }
      if(j=tex->divtabh[hi][1])
      {
        pixval1 = *t1++;
        pixval2 = *--t2;
        for(;j;j--)
        {
          ia1 -= IMAGEWIDTH;
          *ia1  = pixval1;
          *ia2  = pixval2;
          ia2 += IMAGEWIDTH;
        }       
      } 
    }
    else if(!tex->precalc || hi>tex->height)
    {
      t1   = tex->data + (xp<<tex->hshift);
      jmpt = tex->jmptab + ((hi & tex->hmask)<<tex->jshift);
      ln   = hi>>tex->hshift;

      for(j=tex->divtab[hi][0];j;j--)
      {
        pixval1 = *t1++;
        for(k=ln;(--k)>=0;)
        {
          ia1 -= IMAGEWIDTH;
          *ia1 = pixval1;
          *ia2 = pixval1;
          ia2 += IMAGEWIDTH;
        }
        if(*jmpt++)
        {
          ia1 -= IMAGEWIDTH;
          *ia1  = pixval1;
          *ia2  = pixval1;
          ia2 += IMAGEWIDTH;
        }
      }
      if(j=tex->divtab[hi][1])
      {
        pixval1 = *t1++;
        for(;j;j--)
        {
          ia1 -= IMAGEWIDTH;
          *ia1  = pixval1;
          *ia2  = pixval1;
          ia2 += IMAGEWIDTH;
        }       
      }

    }
    else
    {
      t1 = tex->datatab[hi] + xp*hi;
      for(j=(hi>WYHALF)?WYHALF:hi;j;j--)
      {
        ia1 -= IMAGEWIDTH;
        *ia1 = *ia2 = *t1++;
        ia2 += IMAGEWIDTH;
      }
    }

  }
}

static char *make_jump_tab(int h)
{
  int d;
  int d1;
  int i,j,h1,shft;
  char *t1,*t;
  static char *jmptabsave[32] = { NULL, };

  if(!h) 
    return NULL;

  for(h1=h,shft=0;h1;h1>>=1,shft++);
  if(jmptabsave[shft] != NULL)
    return jmptabsave[shft];

  if((t1 = t = malloc(h*h)) == NULL)
  {
    fprintf(stderr,"Out of Memory!\n");
    exit(1);
  }
  jmptabsave[shft] = t;

  for(i=0;i<h;i++) /* fullheight */
  {
    d1 = (i<<16) / h;
    d = 0x7fff;
    for(j=0;j<h;j++)
    {
      d += d1;
      if(d & 0xffff0000)
      {
        t[j] = 1;
        d &= 0x0000ffff;
      }
      else
        t[j] = 0;
    }
    t += h;
  }

  return t1;
}

static char *make_div_tab(int h)
{
  int i,h1,shft;
  int h2;
  char *t;
  static char *divtabsave[32] = { NULL, };
  short (*t1)[2];

  if(!h) 
    return NULL;

  for(h1=h,shft=0;h1;h1>>=1,shft++);
  if(divtabsave[shft] != NULL)
    return divtabsave[shft];

  if((t = malloc(4096*2*sizeof(short))) == NULL)
  {
    fprintf(stderr,"Out of Memory!\n");
    exit(1);
  }
  divtabsave[shft] = t;

  t1 = (short (*)[2]) t;

  for(i=0;i<WYHALF;i++)
  {
    t1[i][0] = h;
    t1[i][1] = 0;
  }

  h2=WYHALF*h;
  for(i=WYHALF;i<4096;i++)
  {
    int w;
    w = h2 / i;
    t1[i][0] = w;
    w = ((int)i<<16) / h;
    w *= (int) t1[i][0];
    w += 0x7fff;
    t1[i][1] = WYHALF - (w>>16);
  }

  return t;
}

void make_tabs(void)
{
  int i,j;
  int *fltab;
  double m[] = { 0,0,0,0,0,0,0,0,0,0,0, 0.5 , 1.0 , 2.0 , 4.0 , 8.0 };

/*
  sqrttab = malloc(66049);
  for(i=0;i<66049;i++)
    sqrttab[i] = (unsigned char) sqrt((double) i);
*/

  circletab = malloc(520*sizeof(double));
  if(circletab == NULL)
  {
    fprintf(stderr,"Not enough memory for circletab.\n");
    exit(1);
  }
  for(i=0;i<512;i++)
  {
    circletab[i] = sqrt(1 - ((double) i)*((double) i)/(512*512));
  }

  fltab = floortab = (int *) malloc(sizeof(int)*4*(WYSIZE>>1)*128);
  if(floortab == NULL)
  {
    fprintf(stderr,"No memory for floortab.\n");
    exit(1);
  }

  for(i=0;i<128;i++)
  {
    double dsin,dcos;
    int i1;

    i1 = i?i:256; 
    
    dsin = (double) trigtab[256-i1] / 0x1000000;
    dcos = (double) trigtab[320-i1] / 0x1000000;
 
    for(j=0;j<(WYSIZE>>1);j++)
    {
      double ydrot,d1cos,d1sin;
      double xo,yo;
      int h;

      h = (j>3)?j:4+j;

      d1cos = WALLHEIGHT / ((double) h * m[sm->outputsize]);
      ydrot = d1cos * (SCREEN-FLUCHT) * 4096 + FLUCHT * 4096;
      d1sin = dsin * d1cos;
      d1cos *= dcos;

      xo = d1cos*(XMIN) - ydrot*dsin;
      yo = ydrot*dcos + d1sin*(XMIN);

      *fltab++ = (int) (xo*0x10);
      *fltab++ = (int) (yo*0x10);
      *fltab++ = (int) (d1sin*0x10000*m[sm->outputsize]);
      *fltab++ = (int) (d1cos*0x10000*m[sm->outputsize]);
    }
  }
}

struct texture *load_texture(char *name)
{
  struct texture *tex;
  int i,j,k,precalc=0,h,w;
  FILE *f;
  char *b,*t,*t1;
  int d,d1;
  unsigned char buf[32];
  int map[256];
  char fn[1024];

  strcpy(fn,TEXTUREPATH);
  strcat(fn,"/");
  strcat(fn,name);

  f=fopen(fn,"r");
  if(f)
  {
    fread(buf,1,32,f);
    fclose(f);
  }
  else
  {
    fprintf(stderr,"Can't load texture: %s\n",fn);
    return NULL;
  }

  w = buf[18] + (buf[19]<<8);
  h = buf[22] + (buf[23]<<8);

/*  if(w <= 32 && h <= 128)
    precalc = 1;*/

  tex = malloc(sizeof(struct texture));
  if(tex == NULL)
  {
    fprintf(stderr,"Not enough memory for texturestruct %s.\n",fn);
    exit(1);
  }

  tex->height = h;
  tex->width = w;
  tex->hhalf = h>>1;
  tex->whalf = w>>1;
  tex->jmptab = make_jump_tab(h);
  tex->jmptabh = make_jump_tab(tex->hhalf);
  tex->divtab = (short (*)[2]) make_div_tab(h);
  tex->divtabh = (short (*)[2]) make_div_tab(tex->hhalf);
  tex->hmask = h-1;
  tex->hmaskh = (h>>1)-1;
  tex->wmask = w-1;
  tex->precalc = precalc;

  if((h & (h-1)) || (w & (w-1)))
  {
    fprintf(stderr,"Illegal texture size.\n");
    return NULL;
  }

  if(precalc)
  {
    tex->datatab = malloc( sizeof(char *) * (h+1) );
    texturemem += (h>>1)*w*h+((w*h)>>1)+4;
    //tex->data = (char *) (((int) malloc((h>>1)*w*h+((w*h)>>1)+4) + 3) & 0xfffffffc);
    void *memptr = NULL;
    posix_memalign(&memptr, 32, (h>>1)*w*h+((w*h)>>1)+4);
    tex->data = memptr;
  }
  else
  {
    //tex->data = (char *) (((int) malloc(w*h+4) + 3) & 0xfffffffc);
    void *memptr = NULL;
    posix_memalign(&memptr, 32, w*h+4 );
    tex->data = memptr;
    texturemem += w*h+4;
  }
  if( (tex->data == NULL) || (precalc && (tex->datatab == NULL)) )
  {
    fprintf(stderr,"Not enough memory for texturedata %s.\n",fn);
    exit(1);
  }

  b = (char *) malloc(tex->width*tex->height);
  if(b == NULL)
  {
    fprintf(stderr,"Not enough memory for tmp-texturedata %s.\n",fn);
    exit(1);
  }
  
 
  for(tex->hshift=-1,i=h;i;i>>=1)
    tex->hshift++;
  for(tex->wshift=-1,i=h;i;i>>=1)
    tex->wshift++;
  tex->jshift = tex->hshift;
  tex->hshifth = tex->hshift-1;
  tex->jshifth = tex->hshifth;

  f=fopen(fn,"r");
  if(f)
  {
    XColor c;
    unsigned char a[4];

    fread(tex->data,1,54,f);
    for(i=0;i<256;i++)
    {
      fread((char *) a,1,4,f);
      c.blue = ((unsigned short) a[0])<<8;
      c.green = ((unsigned short) a[1])<<8;
      c.red = ((unsigned short) a[2])<<8;
      c.flags = DoRed | DoGreen | DoBlue;
      map[i] = get_best_color(&c);
    }
    fread(b,1,tex->width*tex->height,f);
    fclose(f);
  }
  else
    exit(1);

  /* rotate and remap colors */
  for(i=0;i<w;i++)
    for(j=0;j<h;j++)
    {
      tex->data[i*h+j] = map[(int)(unsigned char)b[j*w+i]];
    }

  if(precalc)
  {
    t = tex->data + h*w;
    tex->datatab[h] = tex->data;

    for(k=1;k<h;k++)
    {
      tex->datatab[k] = t;
      d1 = (k<<16) / h;
      for(i=0;i<w;i++)
      {
        d = 0x7fff;
        t1 = tex->data+i*h;
        for(j=h;(--j)>=0;)
        {
          d += d1;
          if(d & 0xffff0000)
          {
            *t++ = *t1++;
            d &= 0x0000ffff;
          }
          else
            t1++;
        }
      }      
    }
  }

  free(b);

  return tex;
}

void image_circle(int x1,int y1,int h1,int h2,int col)
{
  int i,j,k,xi1,xi2;
  int d;
  int d1=0x7fff;
  int r;
  int lclip,rclip,xmid;

  if(h1 < 2)
    return;

  d = (512<<16)/(h1>>1);

  lclip=(IMAGEWIDTH-WXSIZE)>>1;
  rclip=lclip+WXSIZE-1;
  xmid = x1+(IMAGEWIDTH>>1);

  h2>>=1;

  for(k=(h1>>1),i=j=(IMAGEHEIGHT>>1)+y1;k;k--,d1+=d)
  {
    r=(int) (h2*circletab[d1>>16]);
    xi1=xmid-r;
    xi2=xmid+r;
    if(xi1<lclip)
    {
      if(xi2<lclip)
        break;
      xi1=lclip;
    }
    if(xi2>rclip)
    {
      if(xi1>rclip)
        break;
      xi2=rclip;
    }
    image_hline(xi1,i++,xi2,col);
    image_hline(xi1,--j,xi2,col);
  }
}

/*********************************
 * draw textures on the floor 
 * (not optimized)
 */

void image_floor(int x,int y,int angle,struct texture *tx)
{
  char *image = sm->grafix.imagebuf + (IMAGEHEIGHT>>1)*IMAGEWIDTH + ((IMAGEWIDTH-WXSIZE)>>1);
  char *t = tx->data;
  int *fltab;
  int i,j;
  int mask = 0x00ff0000;
  int step=IMAGEWIDTH-WXSIZE;

  switch(angle & 0x80)
  {
    case 0:
      fltab = floortab + angle*4*(WYSIZE>>1);

      for(j=(WYSIZE>>1);j;j--,image+=step)
      {
        int xo,yo,tsin,tcos;
        xo   = x + *fltab++;
        yo   = y + *fltab++;
        tsin = *fltab++;
        tcos = *fltab++;

        for(i=WXSIZE;i;i--)
        {
          *image++ = *(t + (((xo & mask) + ((yo & mask)>>8))>>8) );
    /*
          *image++ = *(t+ ((xo>>8)&0xff00) + ((yo>>16)&0xff) ); */
          xo += tcos; yo += tsin;
        }
      }
      break;
    default:
      angle &= 0x7f;
      fltab = floortab + angle*4*(WYSIZE>>1);

      for(j=(WYSIZE>>1);j;j--,image+=step)
      {
        int xo,yo,tsin,tcos;
        xo   = x - *fltab++;
        yo   = y - *fltab++;
        tsin = - *fltab++;
        tcos = - *fltab++;

        for(i=WXSIZE;i;i--)
        {
          *image++ = *(t + (((xo & mask) + ((yo & mask)>>8))>>8) );
          xo += tcos; yo += tsin;
        }
      }
  }
}

void image_top(int c1)
{
  int i,j,k;

  unsigned int *ia1;
  unsigned int val1=c1 +((int) c1<<8) +((int) c1<<16) +((int) c1<<24);

  ia1 = (unsigned int *) (sm->grafix.imagebuf + 
                       ((IMAGEWIDTH-WXSIZE)>>1) + (IMAGEHEIGHT>>1)*IMAGEWIDTH);
  k = WXSIZE>>2;

  for(i=(WYSIZE>>1);i;i--)
  {
    ia1 -= (IMAGEWIDTH-WXSIZE)>>2;
    for(j=k;j;j--)
      *--ia1 = val1;
  }
}

void image_bg(int c1,int c2)
{
  int i,j,k;

  unsigned int *ia1,*ia2;
  unsigned int val1=c1 +((int) c1<<8) +((int) c1<<16) +((int) c1<<24);
  unsigned int val2=c2 +((int) c2<<8) +((int) c2<<16) +((int) c2<<24);

  ia1 = ia2 = (unsigned int *) (sm->grafix.imagebuf + 
                       ((IMAGEWIDTH-WXSIZE)>>1) + (IMAGEHEIGHT>>1)*IMAGEWIDTH);
  k = WXSIZE>>2;

  for(i=(WYSIZE>>1);i;i--)
  {
    ia1 -= (IMAGEWIDTH-WXSIZE)>>2;
    for(j=k;j;j--)
    {
      *--ia1 = val1;
      *ia2++ = val2;
    }
    ia2 += (IMAGEWIDTH-WXSIZE)>>2;
  }
}

void image_hline(int x1,int y1,int x2,int val)
{
  char *img = sm->grafix.imagebuf + y1*IMAGEWIDTH + x1;
  int d1,d2,d3;
  unsigned int val1;

  if(x2 < x1)
  {
    fprintf(stderr,"argl: image_hline: x2<x1\n");
    return;
  }

  if((x1 & 0xfffc) == (x2 & 0xfffc))
  {
    d1=x2-x1+1;
    for(;d1;d1--)
      *img++ = val;
  }
  else
  {
    val1=val +((int) val<<8) +((int) val<<16) +((int) val<<24);
    d1=4-(x1 & 0x3);
    d2=x2 & 0x3;
    d3=(x2-d2-x1-d1)>>2;
    if(d3 < 0)
    {
      fprintf(stderr,"argl: image_hline: d3<0! \n");
      return;
    }
    for(;d1;d1--)
      *img++ = val;
    for(;d3;d3--)
    { 
         /* *(((unsigned int*)img)++) = val1; */
      *(((unsigned int*)img)) = val1;
      img+=4;
    }
    for(;d2;d2--)
      *img++ = val;
  }
}

void image_sym_vline(int x1,int h1,int col,int size)
{
  char *ia1,*ia2;

  h1 >>= size;
  if(h1 > WYHALF)
    h1 = WYHALF;

  ia1 = ia2 = sm->grafix.imagebuf + (x1>>size) + (IMAGEWIDTH>>1) + (IMAGEHEIGHT>>1)*IMAGEWIDTH;

  for(;h1;h1--)
  {
    ia2 -= IMAGEWIDTH;
    *ia2 = col;
    *ia1 = col;
    ia1 += IMAGEWIDTH;
  }
}
