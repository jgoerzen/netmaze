/* This is a demonstration on how to write robots.
 * How about a competition between some robots? :-)
 */

#include <string.h>
#include "netmaze.h"

extern struct shared_struct *sm;

/*******************
 * the init_robot is called one time right after the start
 */

void init_robot(void)
{
  strcpy(sm->ownname,"Dr.Dummy");
  strcpy(sm->owncomment,"Gotcha!!!");
}

void start_robot(int number)
{

}

/*******************
 * the own_action is called every 'beat'
 *
 * this bot really is a dummy, I use this guy
 * to test my program with 31 'players'.
 */

int own_action(void)
{
  static int i=0;
  int ret=0;

  i++; i &= 0x1f;

  switch(i & 0xf0)
  {
    case 0x00:
      ret = JOY_LEFT | JOY_BUTTON;
      break;
    case 0x10:
      ret = JOY_UP | JOY_BUTTON;
      break;
  }
  return ret;
}


