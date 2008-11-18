/*
 * x11: draw smileyface
 */

#include <math.h>
#include <stdio.h>

/*
#include <X11/Xlib.h>
#include <X11/Xutil.h>
*/

#include "netmaze.h"
#include "x11smiley.h"

#define MAXSMHIGH 1024
#define SMBASE 128
#define MAXFACEANZ 768

extern struct shared_struct *sm;

struct data {
  double w1;
  double w2;
  double y;
  short    step;
} datas[512];

struct sdata {
  short x1;
  short x2;
  short step;
} winkel[128][512];

short lineanz[128];
XSegment facelines[MAXFACEANZ];

/*
extern GRAFIX grafix;
*/
extern GC whitegc;

void init_smiley(void)
{
  short i,j=0,k,l,d;
  short *data = smiley;
  double w,s;

  for(i=0,j=0;i<256;j++) /* Linewerte in Winkelwerte */
  {
    if(*data == 0x7fff)
    {
      datas[j].step = 1;
      i++; data++;
    }
    else
    {
      datas[j].y  = 128.0 * cos(asin((double) (128-i) / 128.0));
      datas[j].w1 = asin((double) (*data++ - 128) / datas[j].y);
      datas[j].w2 = asin((double) (*data++ - 128) / datas[j].y);
      datas[j].step = 0;
    }
  }

  for(i=0;i<128;i++) /* Winkelwerte in Winkel-Line-Werte */
  {
    w = M_PI * i / 128.0;

    for(j=0,k=0,d=0,l=0;j<256;k++)
    {
      if(datas[k].step == 1)
      {
        winkel[i][l].step = 1; j++; l++;
      }
      else if((s = datas[k].w1 + w) < M_PI_2)
      {
        winkel[i][l].x1 = datas[k].y * sin(s);
        if((s = datas[k].w2 + w) < M_PI_2)
          winkel[i][l].x2 = datas[k].y * sin(s);
        else
          winkel[i][l].x2 = datas[k].y;
        winkel[i][l].step = 0; d++; l++;
      }
    }
    lineanz[i] = d;
  }
}

void draw_smiley(Drawable wind,short x,short y,short win,short r)
{
  short j,k,k1;
  double scale,scale1,jmp=0.5;
  int faceanz = 0,lclip,rclip;

  lclip=(IMAGEWIDTH-WXSIZE)>>1;
  rclip=lclip+WXSIZE-1;
  scale1 = scale = (double) r / 128.0;

  y -= r;

  if(win < 0)
  {
    win = - win;
    scale1 = -scale1;
  }

  if(lineanz[win] == 0) return;

  for(j=0,k=0;j<256;j++) /* very simple test-drawroutine */
  {
    jmp += scale;
    if(jmp >= 1)
    {
      k1 = k;
      while(jmp >= 1)
      {
        k = k1;
        while(winkel[win][k].step == 0)
        {
          if(faceanz < MAXFACEANZ)
          {
            facelines[faceanz].x1 = x+winkel[win][k].x1*scale1;
            facelines[faceanz].x2 = x+winkel[win][k].x2*scale1;
            facelines[faceanz].y1 = facelines[faceanz].y2 = y;
            faceanz++;
          }
          k++;
        }
        y++; k++;
        jmp -= 1;
      }
    }
    else
    {
      while(winkel[win][k].step == 0)  k++;
      k++;
    }
  }
  XDrawSegments(sm->grafix.display,wind,whitegc,facelines,faceanz);

}

void image_face(long x,long r,int win,int col)
{
  int j,k,k1,y;
  double scale,scale1,jmp=0.5;
  extern void image_hline(int x1,int y1,int x2,int val);
  int lclip=(IMAGEWIDTH-WXSIZE)>>1;
  int rclip=((IMAGEWIDTH+WXSIZE)>>1)-1;

  scale1 = scale = (double) r / 128.0;

  if(win < 0)
  {
    win = - win;
    scale1 = -scale1;
  }

  if(lineanz[win] == 0) return;

  x += (IMAGEWIDTH>>1);
  y = (IMAGEHEIGHT>>1) - r;

  for(j=0,k=0;j<256;j++) /* very simple test-drawroutine */
  {
    jmp += scale;
    if(jmp >= 1)
    {
      k1 = k;
      while(jmp >= 1)
      {
        k = k1;
        while(winkel[win][k].step == 0)
        {
          int xi1,xi2;
          int x1=x+winkel[win][k].x1*scale1;
          int x2=x+winkel[win][k].x2*scale1;
          k++;

          if(x1>x2) 
          {
            if(x2<lclip)
            {
              if(x1<lclip)
                continue;
              x2=lclip;
            }
            if(x1>rclip)
            { 
              if(x2>rclip)
                continue;
              x1=rclip;
            }
            image_hline(x2,y,x1,col);
          }
          else
          {
            if(x1<lclip)
            {
              if(x2<lclip)
                continue;
              x1=lclip;
            }
            if(x2>rclip)
            { 
              if(x1>rclip)
                continue;
              x2=rclip;
            }
            image_hline(x1,y,x2,col);
          }
        }
        y++; k++;
        jmp -= 1;
      }
    }
    else
    {
      while(winkel[win][k].step == 0)  k++;
      k++;
    }
  }
}

