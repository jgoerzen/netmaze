/*
 * this is just a test.
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

String resources[] = {
  "*titlelabel.labelString: Netmaze - The Multiplayercombatgame!!",
  "*namelabel.labelString:    Your Name:   ",
  "*msglabel.labelString: Messages:",
  "*commentlabel.labelString: Your Comment:",
  "*ntext.columns: 16",
  "*ntext.highlightThickness: 0",
  "*ntext.editable: False",
  "*ntext.value: This is the name",
  "*ntext.cursorPositionVisible: False",
  "*ctext.columns: 32",
  "*ctext.highlightThickness: 0",
  "*ctext.editable: False",
  "*ctext.value: This is the comment",
  "*ctext.cursorPositionVisible: False",
  "*msgtext.columns: 36",
  "*msgtext.highlightThickness: 0",
  "*msgtext.editable: False",
  "*msgtext.value: This is the message-box.\\nHere you can read\\nthe messages from other\\nplayers.",
  "*msgtext.cursorPositionVisible: False",
  "*msgtext.rows: 5",
  "*msgtext.editMode: Multi_Line_Edit",
  "*pmsgtext.columns: 36",
  "*pmsgtext.highlightThickness: 0",
  "*pmsgtext.value: Write a message here",
  "*slabel.labelString: Send",
  "*tlabel.labelString: to",
/*  "*text.shadowThickness: 5", */
/*  "*text.sensitive: True", */
 NULL
};

void hinfo(Widget w,XtPointer p1,XtPointer p2);
void hquit(Widget w,XtPointer p1,XtPointer p2);
void InputHandler(XtPointer c,int *s,XtInputId *id);
XtAppContext app_con;

void main(int argc,char **argv)
{
  Widget top,ntext,ctext,vrow,form,tmp,tmp1,msgtext,pmsgtext,info,quit,help,fs;
  Arg args[2];

  top = XtAppInitialize(&app_con, "Netmaze", NULL, 0,
                        &argc, argv, resources, NULL, 0);

  form = XtCreateManagedWidget("form",xmFormWidgetClass,top,NULL,0);

  vrow = XtCreateManagedWidget("vrow",xmRowColumnWidgetClass,form,NULL,0);

  XtCreateManagedWidget("titlelabel",xmLabelWidgetClass,vrow,NULL,0);

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,vrow,NULL,0);

  tmp1 = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,vrow,NULL,0);
  XtSetArg(args[0],XmNorientation,XmHORIZONTAL);
  tmp = XtCreateManagedWidget("hrow",xmRowColumnWidgetClass,tmp1,args,1);
  XtCreateManagedWidget("namelabel",xmLabelWidgetClass,tmp,NULL,0);
  ntext = XtCreateManagedWidget("ntext",xmTextWidgetClass,tmp,NULL,0);
  tmp = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,tmp1,args,1);
  XtCreateManagedWidget("commentlabel",xmLabelWidgetClass,tmp,NULL,0);
  ctext = XtCreateManagedWidget("ctext",xmTextWidgetClass,tmp,NULL,0);

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,vrow,NULL,0);

  tmp = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,vrow,args,1);
  XtCreateManagedWidget("slabel",xmLabelWidgetClass,tmp,NULL,0);
  pmsgtext = XtCreateManagedWidget("pmsgtext",xmTextWidgetClass,tmp,NULL,0);
  XtCreateManagedWidget("tlabel",xmLabelWidgetClass,tmp,NULL,0);

  tmp = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,vrow,args,1);
  XtCreateManagedWidget("tomaster",xmPushButtonWidgetClass,tmp,NULL,0);
  XtCreateManagedWidget("toteam",xmPushButtonWidgetClass,tmp,NULL,0);
  XtCreateManagedWidget("toall",xmPushButtonWidgetClass,tmp,NULL,0);

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,vrow,NULL,0);

  tmp = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,vrow,args,1);
  XtCreateManagedWidget("msglabel",xmLabelWidgetClass,tmp,NULL,0);
  msgtext = XtCreateManagedWidget("msgtext",xmTextWidgetClass,tmp,NULL,0);

  XtCreateManagedWidget("a_separator",xmSeparatorWidgetClass,vrow,NULL,0);

  tmp = XtCreateManagedWidget("a_row",xmRowColumnWidgetClass,vrow,args,1);
  quit = XtCreateManagedWidget("pushquit",xmPushButtonWidgetClass,tmp,NULL,0);
  help = XtCreateManagedWidget("pushhelp",xmPushButtonWidgetClass,tmp,NULL,0);
  info = XtCreateManagedWidget("pushinfo",xmPushButtonWidgetClass,tmp,NULL,0);

  fs = XmCreateFileSelectionDialog(vrow,"fileselect",NULL,0);
  XtAddCallback(info, XmNactivateCallback, hinfo,(XtPointer) fs);
  XtAddCallback(quit, XmNactivateCallback, hquit,(XtPointer) top);

  XtAppAddInput(app_con,0,(XtPointer) XtInputReadMask,InputHandler,(XtPointer) 0);

  XtRealizeWidget(top);
  XtAppMainLoop(app_con);
}

void hinfo(Widget w,XtPointer p1,XtPointer p2)
{
  fprintf(stderr,"OK");
  XtManageChild((Widget) p1);
}

void hquit(Widget w,XtPointer p1,XtPointer p2)
{
  XtDestroyApplicationContext(app_con);
  exit(0);
}

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

