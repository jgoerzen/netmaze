#!/usr/bin/wish8.5 -f

frame .init
pack .init
label .init.l1 -text "Initializing xnetserv, just a sec..."
pack .init.l1
set Initmessage "Loading defaults..."
label .init.message -textvariable Initmessage
pack .init.message -in .init
update idletasks
wm title . "xnetserv: initializing"
update idletasks
source xnetserv-setup.tcl
set Initmessage "Initializing Tix..."
update idletasks
load $tixfile
set Initmessage "Initializing xnetserv main program..."
update idletasks

##### SET UP PROCEDURES

proc unimplemented {} {
  toplevel .u
  label .u.msg -text "This function is not yet implemented."
  pack .u.msg
  button .u.b -text "OK" -command {
    destroy .u
    return 0
  }
  pack .u.b
  wm title .u "xnetserv: unimplemented function"
}

proc setcolor {id value} {
  global teams isdlgbox cframe pframe oframe
  
  # set teams($id) $value
  
  switch $teams($id) {
    0 {set color Yellow}
    1 {set color Red}
    2 {set color Green}
    3 {set color Blue}
    4 {set color SandyBrown}
    5 {set color DarkOliveGreen}
    6 {set color Orange}
    7 {set color DarkOrchid}
    8 {set color Pink}
    9 {set color VioletRed}
    10 {set color LightBlue}
    11 {set color LightCyan}
    12 {set color RosyBrown}
    13 {set color IndianRed}
    14 {set color DeepPink}
    15 {set color LightPink}
    16 {set color yellowGreen}
    17 {set color orchid}
    18 {set color lavender}
    19 {set color lemonchiffon}
    20 {set color YellowGreen}
    21 {set color khaki}
    22 {set color DarkKhaki}
    23 {set color violet}
    24 {set color plum}
    25 {set color DarkOrchid}
    26 {set color DarkViolet}
    27 {set color PaleTurquoise}
    28 {set color Turquoise}
    29 {set color darkturquoise}
    30 {set color coral}
    31 {set color Black}
  }
    
  if {$isdlgbox($id)==1} {
    $pframe.${id}.dlgbox.sample config -bg $color
  }
}

proc playerbox {id} {
  global ids teams names allplayers isdlgbox cframe pframe oframe
  toplevel $pframe.${id}.dlgbox
  set isdlgbox($id) 1
  label $pframe.${id}.dlgbox.l -text "Player $names($id), id $id.\n\
    Select Dismiss below to close this box.\n\
    Select Terminate to kick this player out\n\
    of the game.   You can select the player's\n\
    team below.  A sample of the color of that\n\
    player will be displayed."
  scale $pframe.${id}.dlgbox.team -label "Team number" -from 0 -to 31 \
    -orient horizontal -digits 0 -command "setcolor $id" -variable teams($id)
  label $pframe.${id}.dlgbox.sample -text "Player color" -fg black
  setcolor $id 0
  button $pframe.${id}.dlgbox.b1 -text "Dismiss" -command "\
    destroy $pframe.$id.dlgbox;\
    set isdlgbox($id) 0"
  button $pframe.${id}.dlgbox.b2 -text "Terminate" -command "\
    destroy $pframe.$id.dlgbox;\
    puts \"5 $id\";\
    set isdlgbox($id) 0"
  pack $pframe.${id}.dlgbox.l
  pack $pframe.${id}.dlgbox.team $pframe.${id}.dlgbox.sample -fill x
  pack $pframe.${id}.dlgbox.b1 $pframe.${id}.dlgbox.b2
  wm title $pframe.${id}.dlgbox "xnetserv: info on $names($id) ($id)"
}

proc destroymainmenu {} {
#  destroy .option1 .option2 .players .misc .options .msg .inmsg .sideleft\
#    .sideright
   destroy .mm .inmsg
}

proc sethelp {message} {
  global helpmsg helpon
  # Sets help message, if possible....
  if {$helpon} {
    if {$helpmsg!=$message} {
      set helpmsg $message
    }
  }
}
  
proc addplayerfunc {name id addvars} {
  # allplayers is a list containing all unique IDs.
  # a unique ID is $id_$name
  global allplayers teams names ids \
    isgip numplayers isdlgbox cframe pframe oframe
  if {$addvars} {
    # ONLY add to the vars if this is a NEW creation
    lappend allplayers ${id}
    set names(${id}) $name
    set ids(${id}) $id
    incr numplayers
    set teams(${id}) $id
    set isdlgbox($id) 0
  }
  if {!$isgip} {
    # Only draw stuff if game is not in progress
    frame $pframe.${id}


    button $pframe.${id}.name -text $name -command "playerbox $id"
    bind $pframe.${id}.name <Any-Motion> {
      sethelp "These buttons let you view info on the player or\
        terminate the player's connection to the game."
    }

    entry $pframe.${id}.team -width 5 -relief sunken -textvariable \
      teams(${id})
    bind $pframe.${id}.team <Any-Motion> {
      sethelp "This lets you set the team a given player is on.  All\
        players with the same team number will be on the same team."
    }

    bind $pframe.${id}.team <Return> "setcolor $id 0"

    pack $pframe.${id} -in $pframe -fill x
    pack $pframe.${id}.name -side left -in $pframe.${id}
    pack $pframe.${id}.team -in $pframe.${id} -side right -padx 1m
  }
}

proc drawplayer {name id} {addplayerfunc $name $id 0}

proc addplayer {name id} {
  addplayerfunc $name $id 1
}

proc rmplayer {name id} {
  global allplayers names ids teams numplayers isgip cframe pframe oframe
  incr numplayers -1
  unset names(${id})
  unset ids(${id})
  unset teams(${id})
  unset isdlgbox($id)
  set listindex [lsearch -exact $allplayers ${id}]
  set allplayers [lreplace $allplayers $listindex $listindex]
  unset listindex
  if {!$isgip} {
    destroy $pframe.${id}
  }
}

proc resetplayers {} {
  global allplayers names ids teams numplayers isgip isdlgbox\
    pframe oframe cframe
  
  if {$numplayers} {
    foreach thisplayer $allplayers {
      if {!$isgip} {destroy $pframe.$ids($thisplayer)}
    }
    unset allplayers
    unset names
    unset ids
    unset teams
    unset isdlgbox
  }
  set numplayers 0
}

proc startgame {} {
  global isgip
  destroymainmenu
  frame .gip
  label .gip.l -text "Netmaze game is in progress!\nOptions are disabled while\
     game is running.\nThis is Netmaze net protocol version 0.81" -fg white \
     -bg black
  label .gip.lp -image NMLogo -bg black -highlightthickness 0
  button .gip.b -text "Abort game" -fg white -bg black \
    -highlightthickness 0 -command {
    puts "3"
  }
  label .gip.msgs -textvariable dispmsg -fg green -bg black
  pack .gip -fill both
  pack .gip.l .gip.lp .gip.b .gip.msgs -in .gip -fill both
  set isgip 1
  update idletasks
}

proc stopgame {} {
  global isgip
  if {$isgip} {
    destroy .gip
  }
  set isgip 0
  mainmenu
  update idletasks
}

proc resetteams {} {
  # resets all teams to their defaults, that is, their ids
  global teams ids allplayers numplayers
  if {$numplayers} {
    foreach thisplayer $allplayers {
      set teams($thisplayer) $ids($thisplayer)
    }
  }
}

proc pickmazecmd {filename} {
  puts "1 $filename"
}

proc pickmaze {} {
  global cframe pframe oframe
  # Procedure to let user pick a new maze.
  tixExFileSelectDialog $cframe.nmaze -title "Select new maze" \
    -command pickmazecmd
  $cframe.nmaze subwidget fsbox config -pattern {*.maz}
  $cframe.nmaze subwidget fsbox config -dir {/usr/lib/games/netmaze/mazes}
  $cframe.nmaze subwidget fsbox config -filetypes {
    {{*.maz}		{Maze files}}
    {{*.txt}		{Text files}}
    {{*}		{All files}}
  }
  $cframe.nmaze popup
}

proc mainmenu {} {
  global newmaze allplayers names ids numplayers teams BEATDIV dispmsg \
    cframe pframe oframe
  wm title . "xnetserv 0.82"

  tixNoteBook .mm
  pack .mm
  .mm add cp -label "Control Panel" -underline 0
  .mm add players -label "Players" -underline 0
  .mm add options -label "Options" -underline 0
  
  set cframe [.mm subwidget cp]
  set pframe [.mm subwidget players]
  set oframe [.mm subwidget options]
  
  
  frame $cframe.o1

  # netserv option 1
  
  button $cframe.o1.b -text "Load new maze" -command {
    pickmaze }
  bind $cframe.o1.b <Any-Motion> {
    sethelp "This will load a new maze."
  }
  
  button $cframe.o1.b2 -text "Load default maze" -command {
    puts "1" }
  bind $cframe.o1.b2 <Any-Motion> {
    sethelp "This will load the default maze."
  }

  pack $cframe.o1 -side top -padx 1m -pady 1m
  pack $cframe.o1.b $cframe.o1.b2 -in $cframe.o1 -side left
  
  # netserv option 2
  
  frame $cframe.o2

  button $cframe.o2.b -text "Start game" -command {
    if {$numplayers} {
      # First....do the options.
      # clear all options
      puts "98"
      
      if {$BOUNCE} {puts "21"}
      if {$DECAY} {puts "22"}
      if {$MULTISHOT} {puts "23"}
      if {$HURTS2SHOOT} {puts "24"}
      if {$REPOWERONKILL} {puts "25"}
      if {$FASTHEAL} {puts "26"}
      if {$FASTWALK} {puts "27"}
      if {$CLOAK} {puts "29"}
      if {$DECSCORE} {puts "32"}
      if {$TEAMSHOTHURT} {puts "34"}
      
      if {$BEATDIV>=2} {puts "7"}
      if {$BEATDIV==4} {puts "7"}
      
      set startcmd "2"
      # Cycle through teams....
      foreach thisplayer $allplayers {
        lappend cmdlist $teams($thisplayer)
      }
      set i [expr [llength $cmdlist] -1]
      while {$i >= 0} {
        append startcmd " "
        append startcmd [lindex $cmdlist $i]
        incr i -1
      }
      puts $startcmd
      unset startcmd thisplayer cmdlist i
    }
  }
  bind $cframe.o2.b <Any-Motion> {
    sethelp "Start a game with the current settings."
  }

  button $cframe.o2.reset -text "Reset teams" -command resetteams
  bind $cframe.o2.reset <Any-Motion> {
    sethelp "Reset teams so that everyone is playing\
      individually.  (back to how they were set prior\
      to any modification)"
  }
  
  button $cframe.o2.refresh -text "Refresh" -command {
    puts "96"
  }
  bind $cframe.o2.refresh <Any-Motion> {
    sethelp "Will refresh xnetserv's team listing in case it somehow got\
             corrupted."
  }

  pack $cframe.o2 -side top -padx 1m -pady 1m -side top
  pack $cframe.o2.b $cframe.o2.reset $cframe.o2.refresh \
    -side left -in $cframe.o2 -padx 1m

  frame $pframe.labels
  label $pframe.labels.name -text "Players:"
  label $pframe.labels.team -text "Team:"
  pack $pframe.labels -side top -in $pframe -fill x
  pack $pframe.labels.name -side left -in $pframe.labels
  pack $pframe.labels.team -side right -in $pframe.labels
  
  if {$numplayers} {
    foreach thisplayer $allplayers {
      drawplayer $names($thisplayer) $ids($thisplayer)
    }
  }
  
  ############ THE OPTIONS!
  
  frame $oframe.m
  frame $oframe.a
  pack $oframe.m $oframe.a -fill x -in $oframe
  frame $oframe.m.f1
  frame $oframe.m.f2
  frame $oframe.a.f3

  checkbutton $oframe.m.f1.bounce -text Bounce -variable BOUNCE
  checkbutton $oframe.m.f1.decay -text Decay -variable DECAY
  checkbutton $oframe.m.f1.multishot -text "Multi-shot" -variable MULTISHOT
  checkbutton $oframe.m.f1.hurts2shoot -text "Hurts to shoot" -variable HURTS2SHOOT
  checkbutton $oframe.m.f1.repoweronkill -text "Repower on kill" -variable REPOWERONKILL
  checkbutton $oframe.m.f2.fastheal -text "Fast heal" -variable FASTHEAL
  checkbutton $oframe.m.f2.fastwalk -text "Fast walk" -variable FASTWALK
  checkbutton $oframe.m.f2.cloak -text "Cloaking" -variable CLOAK
  checkbutton $oframe.m.f2.decscore -text "Decrease score" -variable DECSCORE
  checkbutton $oframe.m.f2.teamshothurt -text "Team shots hurt" -variable TEAMSHOTHURT
  
  tixComboBox $oframe.a.f3.beatbox -label "Beat divider: " -editable false \
    -dropdown true -variable BEATDIV
  $oframe.a.f3.beatbox insert end 1
  $oframe.a.f3.beatbox insert end 2
  $oframe.a.f3.beatbox insert end 4
  
  pack $oframe.m.f1 $oframe.m.f2 -side left -fill x
  pack $oframe.a.f3 -fill x
  pack $oframe.m.f1.bounce $oframe.m.f1.decay -in $oframe.m.f1 -anchor w
  pack $oframe.m.f1.multishot $oframe.m.f1.hurts2shoot -in $oframe.m.f1 -anchor w
  pack $oframe.m.f1.repoweronkill -in $oframe.m.f1 -anchor w
  pack $oframe.m.f2.fastheal -in $oframe.m.f2 -anchor w
  pack $oframe.m.f2.fastwalk $oframe.m.f2.cloak -in $oframe.m.f2 -anchor w
  pack $oframe.m.f2.decscore $oframe.m.f2.teamshothurt -in $oframe.m.f2 -anchor w
  pack $oframe.a.f3.beatbox -in $oframe.a.f3 -anchor w -side left
  
  # messages
  
  frame $cframe.msg
  pack $cframe.msg -fill x
  label $cframe.msg.label -text "Send message:"
  pack $cframe.msg.label -side left -padx 1m
  entry $cframe.msg.entry -textvariable MSGSEND
  pack $cframe.msg.entry -fill x -in $cframe.msg -side left -fill x
  bind $cframe.msg.entry <Return> {
    set transcmd "97 "
    append transcmd $MSGSEND
    puts $transcmd
    unset transcmd
  }
  bind $cframe.msg.entry <Any-Motion> {
    sethelp "Type a message here and press enter to send it\
             to the xterms of all players."
  }
  
  # quit

  frame $cframe.misc
    
  button $cframe.misc.b -text "Quit" -command {
    puts "9"
    exit
  }
  button $cframe.misc.help -text "Help" -command {
    if {!$helpon} {
      toplevel .help
      label .help.intro -text "xnetserv by John Goerzen\nOnline help\nMove\
        the mouse over any item\nand help will appear here."
      frame .help.m
      message .help.m.l -textvariable helpmsg -relief sunken -borderwidth 4
      button .help.button -text "Dismiss" -command {
        destroy .help
        set helpon 0
      }
      label .help.author -text "E-mail: <jgoerzen@complete.org>"
      wm title .help "xnetserv help"
      set helpon 1
      pack .help.intro
      pack .help.m -fill x
      pack .help.button .help.author
      pack .help.m.l -in .help.m -fill x
    }
  }
      
    
# HP 2
  pack $cframe.misc -padx 1m -pady 1m
  pack $cframe.misc.b $cframe.misc.help -side left -in $cframe.misc
  
  # Messages from netserv
  
  frame .inmsg
  label .inmsg.l -textvariable dispmsg -fg blue
  
  pack .inmsg -padx 1m -pady 1m -fill x
  pack .inmsg.l -side left -in .inmsg -fill x
  
}

proc intro {} {

  wm title . "Welcome to Netmaze!"
  label .li -text "Welcome to Netmaze!  Click below to begin" -bg midnightblue \
   -fg white
  pack .li
  label .l -text "Version 0.82 (0.81 compat)" -bg black -fg red
  . config -bg black
  image create photo NMLogo -file nmlogo.gif
  button .b -image NMLogo -highlightthickness 0 -fg black -bg black -highlightbackground black -command {
    destroy .b .li
    destroy .l
    set isgip 0
    . config -bg #d9d9d9
    mainmenu
    return 0 
  }
  pack .b .l
}

proc inevhandler {} {
  global dispmsg
  
  gets stdin line
  switch $line {
    MESSAGE {
      gets stdin inline
      set dispmsg "$inline"
    }
    MESSAGECAT {
      gets stdin inline
      append dispmsg "\n$inline"
    }
    CONNECTIONLIST {
      resetplayers
      gets stdin inline
      while {$inline!="ENDCONNECTIONLIST"} {
        gets stdin playernum
        gets stdin playername
        gets stdin playerhost
        addplayer $playername $playernum
        gets stdin inline
      }
      update idletasks
    }
    STARTGAME {
      startgame
    }
    STOPGAME {
      stopgame
    }
  }
}




# This is it, folks: everything starts here.


set numplayers 0
set isgip 1
set helpon 0
set helpmsg "This is xnetserv by John Goerzen."
# isgip is set to true so that messages don't try to draw on the box.
# When the main box appears, isgip is set to 0.

# default options

set BOUNCE 0
set DECAY 0
set HURTS2SHOOT 0
set MULTISHOT 1
set REPOWERONKILL 0
set FASTHEAL 1
set FASTWALK 1
set CLOAK 0
set DECSCORE 0
set TEAMSHOTHURT 0

set BEATDIV 1

set dispmsg "Welcome to xnetserv!"

destroy .init
unset Initmessage

intro

fileevent stdin readable {inevhandler}
