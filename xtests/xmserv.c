/*
 * This is just a test.
 * You can use this program as an external menu for the netserv
 *
 * Call: netserv -exmenu xmserv
 * with 'xmserv' = pathname of this program
 *
 * OK, it's Motif ... but soon, I will program a tcl-version.
 */

#include <stdio.h>
#include <unistd.h>

#include <X11/Intrinsic.h>

#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/FormP.h>
#include <Xm/FileSB.h>
#include <Xm/ToggleB.h>
#include <Xm/Scale.h>

enum { PUSHRUN,PUSHRUN1,PUSHTEAM,PUSHSTOP,PUSHQUIT,PUSHLIST,TEAMOK,TEAMCANCEL,
       CFGOK, PUSHCFG };


enum { MENU_NOP , MENU_LOADMAZE , MENU_RUNGAME , MENU_STOPGAME , MENU_LIST ,
       MENU_DISCONNECT , MENU_SETMODE , MENU_SETDIVIDER , MENU_ERROR ,
       MENU_RUNTEAMGAME , MENU_SETTEAMS };

String resources[] = {
  "*titlelabel.labelString: Netservcontrol",
  "*pushquit.labelString: Quit Prorgamm",
  "*pushrun.labelString: Run Game",
  "*pushrun1.labelString: Run Game with Teams",
  "*pushstop.labelString: Stop Game",
  "*pushlist.labelString: List Connections",
  "*pushteam.labelString: Select Teams",
  "*pushcfg.labelString: Configure",
  "*tok.labelString: Ok",
  "*tcancel.labelString: 	Cancel",
  "*tscale.minimum:	1",
  "*tscale.maximum:	16",
  "*tscale.showValue:	True",
  "*tscale.orientation:	Horizontal",
  "*cfgdiv1.labelString: Divider 1",
  "*cfgdiv2.labelString: Divider 2",
  "*cfgdiv3.labelString: Divider 4",
  "*cfgm1.labelString: Classic Mode",
  "*cfgm2.labelString: Extended Mode",
  "*cfgm3.labelString: Just-4-Fun Mode",
  "*cfgok.labelString: Set",
 NULL
};

void ButtonCB(Widget w,XtPointer p1,XtPointer p2);
void InputHandler(XtPointer c,int *s,XtInputId *id);
void SetTeamValues(void),GetTeamValues(void);
Widget CreateTeamDialog(Widget parent);
Widget CreateConfigDialog(Widget parent);
int GetConfigValue(void);

XtAppContext app_con;
Widget teamdialog,teams[32];
Widget cfgdiv1,cfgdiv2,cfgdiv3,cfgmode1,cfgmode2,cfgmode3,configdialog;

int teamnr[32] = { 0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7} ;

void main(int argc,char **argv)
{
  Widget top,vrow,form,tmp,quit,run,run1,stop,list,team,cfg;
  Arg args[2];

  top = XtAppInitialize(&app_con, "Netserv", NULL, 0,
                        &argc, argv, resources, NULL, 0);

  teamdialog = CreateTeamDialog(top);
  configdialog = CreateConfigDialog(top);

  form = XtCreateManagedWidget("form",xmFormWidgetClass,top,NULL,0);

  vrow = XtCreateManagedWidget("vrow",xmRowColumnWidgetClass,form,NULL,0);

  XtCreateManagedWidget("titlelabel",xmLabelWidgetClass,vrow,NULL,0);

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,vrow,NULL,0);

  stop = XtCreateManagedWidget("pushstop",xmPushButtonWidgetClass,vrow,NULL,0);
  run = XtCreateManagedWidget("pushrun",xmPushButtonWidgetClass,vrow,NULL,0);
  run1 = XtCreateManagedWidget("pushrun1",xmPushButtonWidgetClass,vrow,NULL,0);
  list = XtCreateManagedWidget("pushlist",xmPushButtonWidgetClass,vrow,NULL,0);
  team = XtCreateManagedWidget("pushteam",xmPushButtonWidgetClass,vrow,NULL,0);
  cfg = XtCreateManagedWidget("pushcfg",xmPushButtonWidgetClass,vrow,NULL,0);

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,vrow,NULL,0);

  XtSetArg(args[0],XtNorientation,XmHORIZONTAL);
  tmp = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,vrow,args,1);
  quit = XtCreateManagedWidget("pushquit",xmPushButtonWidgetClass,tmp,NULL,0);

  XtAddCallback(run, XmNactivateCallback, ButtonCB,(XtPointer) PUSHRUN);
  XtAddCallback(run1, XmNactivateCallback, ButtonCB,(XtPointer) PUSHRUN1);
  XtAddCallback(quit, XmNactivateCallback, ButtonCB,(XtPointer) PUSHQUIT);
  XtAddCallback(stop, XmNactivateCallback, ButtonCB,(XtPointer) PUSHSTOP);
  XtAddCallback(list, XmNactivateCallback, ButtonCB,(XtPointer) PUSHLIST);
  XtAddCallback(team, XmNactivateCallback, ButtonCB,(XtPointer) PUSHTEAM);
  XtAddCallback(cfg, XmNactivateCallback, ButtonCB,(XtPointer) PUSHCFG);

  XtAppAddInput(app_con,0,(XtPointer)XtInputReadMask,InputHandler,(XtPointer)0);

  XtRealizeWidget(top);
  XtAppMainLoop(app_con);
}

/********************************
 * Button-Callback handler
 */

void ButtonCB(Widget w,XtPointer p1,XtPointer p2)
{
  char buf[64];
  int i;

  switch( (int) p1)
  {
    case PUSHQUIT:
      XtDestroyApplicationContext(app_con);
      exit(0);
      break;
    case PUSHRUN:
      buf[0] = MENU_RUNGAME;
      fwrite(buf,1,1,stdout);
      fflush(stdout);
      break;
    case PUSHSTOP:
      buf[0] = MENU_STOPGAME;
      fwrite(buf,1,1,stdout);
      fflush(stdout);
      break;
    case PUSHLIST:
      buf[0] = MENU_LIST;
      fwrite(buf,1,1,stdout);
      fflush(stdout);
      break;
    case PUSHTEAM:
      XtManageChild(teamdialog);
      break;
    case PUSHRUN1:
      buf[0] = MENU_SETTEAMS;
      for(i=0;i<32;i++)
        buf[i+1] = (char) teamnr[i];
      fwrite(buf,33,1,stdout);
      fflush(stdout);
      buf[0] = MENU_RUNTEAMGAME;
      fwrite(buf,1,1,stdout);
      fflush(stdout);
      break;
    case PUSHCFG:
      XtManageChild(configdialog);
      break;
    case TEAMOK:
      GetTeamValues();
      XtUnmanageChild(teamdialog);
      break;
    case TEAMCANCEL:
      SetTeamValues();
      XtUnmanageChild(teamdialog);
      break;
    case CFGOK:
      XtUnmanageChild(configdialog);
      i = GetConfigValue();
      buf[0] = MENU_SETDIVIDER;
      buf[1] = i & 0xf;
      buf[2] = MENU_SETMODE;
      buf[3] = i >> 4;
      fwrite(buf,7,1,stdout);
      fflush(stdout);

      break;

  }
}

/***************************
 * stdin-input-handler
 */

void InputHandler(XtPointer c,int *s,XtInputId *id)
{
  char buf[1024];
  int i;
  memset(buf,0,1024);
  i = read(*s,buf,1024);
  fprintf(stderr,"%d:%s\n",i,buf);
  if(i == 0)
  {
    XtDestroyApplicationContext(app_con);
    exit(0);
  }
}

/*******************************
 * team-stuff
 */

Widget CreateTeamDialog(Widget parent)
{
  Widget t,r,rb,r1,t1;
  int i,j,k;
  Arg args[4];
  char *labels[] = { "Players 1 to 8" , "Players 9 to 15" ,
                     "Players 16 to 24" , "Players 25 to 32" };

  t = XmCreateFormDialog(parent,"TeamSelection",NULL,0);
  r = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,t,NULL,0);
  XtSetArg(args[0],XmNlabelString,XmStringCreateSimple("Please select the teams:"));
  XtCreateManagedWidget("a_label",xmLabelWidgetClass,r,args,1);
  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,r,NULL,0);

  XtSetArg(args[0],XmNorientation,XmHORIZONTAL);
  r1 = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,r,args,1);

  for(k=0,j=0;j<4;j++)
  {
    rb = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,r1,NULL,0);
    XtSetArg(args[0],XmNlabelString,XmStringCreateSimple(labels[j]));
    XtCreateManagedWidget("alabel",xmLabelWidgetClass,rb,args,1);
    for(i=0;i<8;i++,k++)
    {
      teams[k] = XtCreateManagedWidget("tscale",xmScaleWidgetClass,rb,NULL,0);
    }
  }

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,r,NULL,0);
  XtSetArg(args[0],XmNorientation,XmHORIZONTAL);
  r1 = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,r,args,1);
  t1 = XtCreateManagedWidget("tok",xmPushButtonWidgetClass,r1,NULL,0);
  XtAddCallback(t1, XmNactivateCallback, ButtonCB,(XtPointer) 6);
  t1 = XtCreateManagedWidget("tcancel",xmPushButtonWidgetClass,r1,NULL,0);
  XtAddCallback(t1, XmNactivateCallback, ButtonCB,(XtPointer) 7);

  SetTeamValues();

  return t;
}

void SetTeamValues(void)
{
  int i;

  for(i=0;i<32;i++)
    XmScaleSetValue(teams[i],teamnr[i]+1);
}

void GetTeamValues(void)
{
  int i;

  for(i=0;i<32;i++)
  {
    XmScaleGetValue(teams[i],&teamnr[i]);
    teamnr[i]--;
  }
}

/********************************
 * CONFIG-stuff
 */

Widget CreateConfigDialog(Widget parent)
{
  Widget t,r,b,row;
  Arg args[2];

  t = XmCreateFormDialog(parent,"TeamSelection",NULL,0);
  row = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,t,NULL,0);
  r = XmCreateSimpleRadioBox(row,"radiobox",NULL,0);
  XtManageChild(r);
  cfgdiv1 = XtCreateManagedWidget("cfgdiv1",xmToggleButtonWidgetClass,r,NULL,0);
  XmToggleButtonSetState(cfgdiv1,True,False);
  cfgdiv2 = XtCreateManagedWidget("cfgdiv2",xmToggleButtonWidgetClass,r,NULL,0);
  cfgdiv3 = XtCreateManagedWidget("cfgdiv3",xmToggleButtonWidgetClass,r,NULL,0);
  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,row,NULL,0);
  r = XmCreateSimpleRadioBox(row,"radiobox",NULL,0);
  XtManageChild(r);
  cfgmode1 = XtCreateManagedWidget("cfgm1",xmToggleButtonWidgetClass,r,NULL,0);
  XmToggleButtonSetState(cfgmode1,True,False);
  cfgmode2 = XtCreateManagedWidget("cfgm2",xmToggleButtonWidgetClass,r,NULL,0);
  cfgmode3 = XtCreateManagedWidget("cfgm3",xmToggleButtonWidgetClass,r,NULL,0);
  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,row,NULL,0);
  XtSetArg(args[0],XmNorientation,XmHORIZONTAL);
  r = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,row,args,1);
  b = XtCreateManagedWidget("cfgok",xmPushButtonWidgetClass,r,NULL,0);
  XtAddCallback(b,XmNactivateCallback,ButtonCB,(XtPointer) CFGOK);

  return t;
}

int GetConfigValue(void)
{
  int val=0;

  if(XmToggleButtonGetState(cfgdiv1)) val = 1;
  else if(XmToggleButtonGetState(cfgdiv2)) val = 2;
  else if(XmToggleButtonGetState(cfgdiv3)) val = 3;
  if(XmToggleButtonGetState(cfgmode1)) val |= 0x10;
  else if(XmToggleButtonGetState(cfgmode2)) val |= 0x20;
  else if(XmToggleButtonGetState(cfgmode3)) val |= 0x30;

  return val;
}


