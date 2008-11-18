#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>

#define JOY_LEFT 1
#define JOY_RIGHT 2
#define JOY_UP 4
#define JOY_DOWN 8
#define JOY_BUTTON 128

#define LOW_THRESHOLD 93   /* perhaps it's better to test x relativ to y */
#define HIGH_THRESHOLD 107

void main(void)
{
  Display *xd;
  Window r,own;
  int end=0,catch=0;
  int s,x,y,joy=0;
  GC gc;
  int redraw=0;

  xd = XOpenDisplay("");
  r = DefaultRootWindow(xd);
  s = DefaultScreen(xd);

  printf("This is a program to test a playable mousecontrol.\n");
  printf("  To start the test, press Button1 in the Window\n");
  printf("  To end the program, press Button3 in the Window\n");
  printf("While the test is running, Button1 = Fire, Button = StopWalking.\n");

  own = XCreateSimpleWindow(xd,r,100,100,200,200,5,0,BlackPixel(xd,s));
  gc = XCreateGC (xd,r,0,0);

  XSelectInput (xd,own,PointerMotionMask|ButtonPressMask|ButtonReleaseMask);

  XMapRaised (xd,own);
  XFlush(xd);

  for(;end==0;)
  {
    XEvent event1;
    XNextEvent(xd,&event1);
    switch(event1.type)
    {
      case ButtonPress:
        if(event1.xbutton.button == Button1)
        {
          if(!catch)
          {
            catch = 1;
            XGrabPointer(xd,own,False,
                 PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
                 GrabModeAsync,GrabModeAsync,own,None,event1.xbutton.time);
          }
          else
          {
            redraw = 1;
            joy |= JOY_BUTTON;
          }
        }
        else if(event1.xbutton.button == Button3)
        {
          XUngrabPointer(xd,event1.xbutton.time);
          end = 1;
        }
        else if(event1.xbutton.button == Button2)
          joy &= 0xf0;
        break;
      case ButtonRelease:
        if(event1.xbutton.button == Button1)
        {
          joy &= ~JOY_BUTTON;
          redraw = 1;
        }
        break;
      case MotionNotify:
        if(catch == 1)
        {
          if((event1.xmotion.x != 100) || (event1.xmotion.y != 100))
          {
            if(event1.xmotion.x < LOW_THRESHOLD)
            {
              joy &= 0xf0;
              joy |= JOY_LEFT;
              if(event1.xmotion.y < LOW_THRESHOLD+2)
                joy |= JOY_UP;
              else if(event1.xmotion.y > HIGH_THRESHOLD-2)
                joy |= JOY_DOWN;
            }
            else if(event1.xmotion.x > HIGH_THRESHOLD)
            {
              joy &= 0xf0;
              joy |= JOY_RIGHT;
              if(event1.xmotion.y < LOW_THRESHOLD+2)
                joy |= JOY_UP;
              else if(event1.xmotion.y > HIGH_THRESHOLD-2)
                joy |= JOY_DOWN;
            }
            else if(event1.xmotion.y < LOW_THRESHOLD)
            {
              joy &= 0xf0;
              joy |= JOY_UP;
            }
            else if(event1.xmotion.y > HIGH_THRESHOLD)
            {
              joy &= 0xf0;
              joy |= JOY_DOWN;
            }

            XClearWindow(xd,own);
            if(joy & JOY_LEFT) x = 0;
            else if(joy & JOY_RIGHT) x = 200;
            else x = 100;
            if(joy & JOY_UP) y = 0;
            else if(joy & JOY_DOWN) y = 200;
            else y = 100;
            XDrawLine(xd,own,gc,100,100,x,y);
            if(joy & JOY_BUTTON)
              XDrawArc(xd,own,gc,93,93,14,14,0,64*360);
            redraw = 0;

            XWarpPointer(xd,None,own,0,0,0,0,100,100);
          }
        }
        break;
    }
    if(redraw)
    {
      XClearWindow(xd,own);
      if(joy & JOY_LEFT) x = 0;
      else if(joy & JOY_RIGHT) x = 200;
      else x = 100;
      if(joy & JOY_UP) y = 0;
      else if(joy & JOY_DOWN) y = 200;
      else y = 100;
      XDrawLine(xd,own,gc,100,100,x,y);
      if(joy & JOY_BUTTON)
        XDrawArc(xd,own,gc,93,93,14,14,0,64*360);
      redraw = 0;
    }
  }

  XDestroyWindow(xd,own);
  XCloseDisplay(xd);
}

