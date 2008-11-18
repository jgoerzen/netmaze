/*
 * X11: (event) - control
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "netmaze.h"

extern struct shared_struct *sm;

extern void run_game(MAZE*,PLAYER*);
extern void draw_info(void);


void x11_flush(void)
{
  XFlush(sm->grafix.display);
}

void x11_cntrl(void)
{
  int i;
  XEvent event1;
  KeySym key;
  char text[10];

#ifdef USE_IPC
  for(;;)
  {
    if(sm->sologame)
      if(!(XEventsQueued (sm->grafix.display1,QueuedAfterReading) > 0))
/*      if(!(XEventsQueued (sm->grafix.display1,QueuedAlready) > 0)) */
        break;
#else
  while(XEventsQueued (sm->grafix.display1,QueuedAfterReading) > 0)
/*  while(XEventsQueued (sm->grafix.display1,QueuedAlready) > 0) */
  {
#endif
  /*      x11_flush(); */
        XNextEvent(sm->grafix.display1,&event1);
        switch(event1.type)
        {
          case Expose:
	    if(event1.xexpose.window == sm->grafix.gamefg)
            {
              sm->bgdraw  = TRUE;
              sm->redraw  = TRUE;
            }
            else if(event1.xexpose.window == sm->grafix.statusfg)
            {
              sm->redraw   = TRUE;
              sm->statchg2 = sm->statchg1 = TRUE;
            }
            else if(event1.xexpose.window == sm->grafix.killfg)
            {
              sm->redraw  = TRUE;
              sm->killchg = TRUE;
            }
            break;
          case ButtonPress:
            switch (event1.xbutton.button)
            {
              case Button1:
                sm->ownstick |= JOY_LEFT;    /* Left */
                break;
              case Button2:
                sm->ownstick &= ~JOY_HIDE;
                if (event1.xbutton.state & (ShiftMask | ControlMask))
                  sm->ownstick |= JOY_DOWN;  /* Down */
                else
                  sm->ownstick |= JOY_UP;    /* Up */
                break;
              case Button3:
                sm->ownstick |= JOY_RIGHT;   /* Right */
                break;
             }
             break;
           case ButtonRelease:
             switch (event1.xbutton.button)
             {
               case Button1:
                 sm->ownstick &= ~JOY_LEFT;     /* Left */
                 break;
               case Button2:
                 if (event1.xbutton.state & (ShiftMask | ControlMask))
                   sm->ownstick &= ~JOY_DOWN; /* Down */
                 else
                   sm->ownstick &= ~JOY_UP; /* Up */
                 break;
               case Button3:
                 sm->ownstick &= ~JOY_RIGHT;     /* Right */
                 break;
             }
             break;
          case KeyPress:
            i = XLookupString( (XKeyEvent*) &event1,text,10,&key,0);
            switch(key)
            {
              case XK_Up:
                sm->ownstick |= JOY_UP;
                sm->ownstick &= ~JOY_HIDE;
                break;
              case XK_Down:
                sm->ownstick &= ~JOY_HIDE;
                sm->ownstick |= JOY_DOWN;
                break;
              case XK_Right:
                sm->ownstick |= JOY_RIGHT;
                break;
              case XK_Left:
                sm->ownstick |= JOY_LEFT;
                break;
              case XK_Shift_L:
              case XK_Shift_R:
              case XK_Control_L:
              case XK_Control_R:
                sm->ownstick |= JOY_SLOW;
                break ;
              case XK_Return:
                if (sm->ownstick & JOY_RADAR)
                  sm->ownstick &= ~JOY_RADAR;	/* RADAR */
                else
                  sm->ownstick |= JOY_RADAR;
                break;
              case 'c':
                if (sm->ownstick & JOY_HIDE)
                  sm->ownstick &= ~JOY_HIDE;	/* HIDE */
                else
                  sm->ownstick |= JOY_HIDE;
                break;
              case ' ':
                sm->ownstick &= ~JOY_HIDE;
                sm->ownstick |= JOY_BUTTON;
                break;
              case 'm':
                if(sm->newmap)
                  sm->newmap = FALSE;
                else
                  sm->newmap = TRUE;
              case 'l':
                if(sm->locator)
                  sm->locator = FALSE;
                else
                  sm->locator = TRUE;
              case '1':
                if((!sm->gameflag) && sm->sologame)
                {
                  run_game(&(sm->std_maze),sm->playfeld);
                  sm->gameflag = TRUE;
                }
                break;
              case 'p':
                if(TRUE)
                {
                  sm->shownumbertmp++;
                  if (sm->shownumbertmp == sm->anzplayers) sm->shownumbertmp = 0;
                  sm->statchg1 = TRUE; sm->killchg = TRUE;
                }
                break;
              case 'j':
                if(sm->sologame)
                {
                  sm->joynumber++;
                  if (sm->joynumber == sm->anzplayers) sm->joynumber = 0;
                }
                break;
              case 's':
                sm->debug = TRUE;
                break;
              case 'Q':
                sm->exitprg = 1;
                break;
            }
            break;
          case KeyRelease:
            i = XLookupString( (XKeyEvent*) &event1,text,10,&key,0);
            switch(key)
            {
              case XK_Up:
                sm->ownstick &= ~JOY_UP;
                break;
              case XK_Down:
                sm->ownstick &= ~JOY_DOWN;
                break;
              case XK_Right:
                sm->ownstick &= ~JOY_RIGHT;
                break;
              case XK_Left:
                sm->ownstick &= ~JOY_LEFT;
                break;
              case XK_Shift_L:
              case XK_Shift_R:
              case XK_Control_L:
              case XK_Control_R:
                sm->ownstick &= ~JOY_SLOW;
                break;
              case ' ':
                sm->ownstick &= ~JOY_BUTTON;
                break;
            }
            break;
          case ConfigureNotify:
	    if(event1.xconfigure.window == sm->grafix.gamefg)
            {
              if(event1.xconfigure.width != sm->grafix.shint.width)
                sm->bgdraw=TRUE;
              sm->grafix.shint.width = event1.xconfigure.width;
              if(event1.xconfigure.height != sm->grafix.shint.height)
                sm->bgdraw=TRUE;
              sm->grafix.shint.height = event1.xconfigure.height;
              if(sm->bgdraw)
                sm->redraw = TRUE;
            }
	    else if(event1.xconfigure.window == sm->grafix.statusfg)
            {
              sm->grafix.sthint.width = event1.xconfigure.width;
              sm->grafix.sthint.height = event1.xconfigure.height;
              sm->statchg1 = sm->statchg2 = TRUE;
            }
	    else if(event1.xconfigure.window == sm->grafix.killfg)
            {
              sm->killchg = TRUE;
            }
            break;
        }
  }
}
