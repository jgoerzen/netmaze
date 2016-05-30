#!/usr/bin/wish8.5 -f

# NOTE: Tcl/Tk or C on some Sun machines seems to be broken in that it
# requres flush calls after every puts when the output is being piped to
# another program (as it is here).  That is why there are flush calls all
# over in this program.

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

proc playerbox {id} {
  global ids teams names allplayers
  toplevel .players.${id}.dlgbox
  label .players.${id}.dlgbox.l -text "Player $names($id), id $id.\n\
    Select Dismiss below to close this box.\n\
    Select Terminate to kick this player out\n\
    of the game."
  button .players.${id}.dlgbox.b1 -text "Dismiss" -command "\
    destroy .players.$id.dlgbox"
  button .players.${id}.dlgbox.b2 -text "Terminate" -command "\
    destroy .players.$id.dlgbox;\
    puts \"5 $id\";\
    flush stdout"
  pack .players.${id}.dlgbox.l .players.${id}.dlgbox.b1 \
    .players.${id}.dlgbox.b2
  wm title .players.${id}.dlgbox "xnetserv: info on $names($id) ($id)"
}

proc destroymainmenu {} {
  destroy .option1 .option2 .players .misc .options .msg
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
    isgip numplayers
  if {$addvars} {
    # ONLY add to the vars if this is a NEW creation
    lappend allplayers ${id}
    set names(${id}) $name
    set ids(${id}) $id
    incr numplayers
    set teams(${id}) $id
  }
  if {!$isgip} {
    # Only draw stuff if game is not in progress
    frame .players.${id}


    button .players.${id}.name -text $name -command "playerbox $id"
    bind .players.${id}.name <Any-Motion> {
      sethelp "These buttons let you view info on the player\nor\
        terminate the player's connection to the game."
    }

    entry .players.${id}.team -width 5 -relief sunken -textvariable \
      teams(${id})
    bind .players.${id}.team <Any-Motion> {
      sethelp "This lets you set the team a given player is on.\nAll\
        players with the same team number will be on the same team."
    }
      
      
    pack .players.${id} -in .players -fill x
    pack .players.${id}.name -side left -in .players.${id}
    pack .players.${id}.team -in .players.${id} -side right -padx 1m
  }
}

proc drawplayer {name id} {addplayerfunc $name $id 0}

proc addplayer {name id} {
  addplayerfunc $name $id 1
}

proc rmplayer {name id} {
  global allplayers names ids teams numplayers isgip
  incr numplayers -1
  unset names(${id})
  unset ids(${id})
  unset teams(${id})
  set listindex [lsearch -exact $allplayers ${id}]
  set allplayers [lreplace $allplayers $listindex $listindex]
  unset listindex
  if {!$isgip} {
    destroy .players.${id}
  }
}

proc resetplayers {} {
  global allplayers names ids teams numplayers isgip
  if {$numplayers} {
    unset allplayers
    unset names
    unset ids
    unset teams
  }
  set numplayers 0
  if {!$isgip} {
    destroymainmenu
    mainmenu
    puts "4"
    flush stdout
  }
}

proc startgame {} {
  global isgip
  destroymainmenu
  frame .gip
  label .gip.l -text "Netmaze game is in progress!\nOptions are disabled while\
     game is running.\nThis is Netmaze net protocol version 0.81" -fg white \
     -bg black
  label .gip.lp -image NMLogo
  button .gip.b -text "Abort game" -fg white -bg black -command {
    puts "3"
    flush stdout
  }
  pack .gip -fill both
  pack .gip.l .gip.lp .gip.b -in .gip -fill both
  set isgip 1
}

proc stopgame {} {
  global isgip
  if {$isgip} {
    destroy .gip
  }
  set isgip 0
  mainmenu
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

proc mainmenu {} {
  global newmaze allplayers names ids numplayers teams BEATDIV
  wm title . "xnetserv 0.82"
  
  frame .option1

  # netserv option 1
  
  button .option1.b -text "Load/Reinit maze:" -command {
    puts "1 $newmaze"
    flush stdout
  }
  bind .option1.b <Any-Motion> {
    sethelp "This will load a new maze (if the\nentry\
      box has a filename in it) or reinitialize\nthe\
      current maze."
  }


  entry .option1.e -width 20 -textvariable newmaze
  bind .option1.e <Any-Motion> {
    sethelp "Type the name of a maze to load here,\n\
      or leave blank to use the default maze."
  }


  pack .option1 -side top -padx 1m -pady 1m
  pack .option1.b .option1.e -in .option1 -side left
  
  # netserv option 2
  
  frame .option2

  button .option2.b -text "Start game" -command {
    if {$numplayers} {
      # First....do the options.
      # clear all options
      puts "98"
      flush stdout
      
      if {$BOUNCE} {puts "21"
      flush stdout }
      if {$DECAY} {puts "22"
      flush stdout }
      if {$MULTISHOT} {puts "23"
      flush stdout }
      if {$HURTS2SHOOT} {puts "24"
      flush stdout }
      if {$REPOWERONKILL} {puts "25"
      flush stdout }
      if {$FASTHEAL} {puts "26"
      flush stdout }
      if {$FASTWALK} {puts "27"
      flush stdout }
      if {$CLOAK} {puts "29"
      flush stdout }
      if {$DECSCORE} {puts "32"
      flush stdout }
      if {$TEAMSHOTHURT} {puts "34"
      flush stdout }
      
      if {$BEATDIV>=2} {puts "7"
      flush stdout }
      if {$BEATDIV==4} {puts "7"
      flush stdout }
      
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
      flush stdout
      unset startcmd thisplayer cmdlist i
    }
  }
  bind .option2.b <Any-Motion> {
    sethelp "Start a game with the current settings."
  }

  button .option2.reset -text "Reset teams" -command resetteams
  bind .option2.reset <Any-Motion> {
    sethelp "Reset teams so that everyone is\nplaying\
      individually.  (back to how they were set\nprior\
      to any modification)"
  }
  
  button .option2.refresh -text "Refresh" -command {
    puts "96"
    flush stdout
  }
  bind .option2.refresh <Any-Motion> {
    sethelp "Will refresh xnetserv's team listing in case it somehow got\
             corrupted."
  }

  pack .option2 -side top -padx 1m -pady 1m -side top
  pack .option2.b .option2.reset .option2.refresh -side left -in .option2 -padx 1m
  
  frame .players -relief ridge -borderwidth 4
  frame .players.labels
  label .players.labels.name -text "Players:"
  label .players.labels.team -text "Team:"
  pack .players -side top
  pack .players.labels -side top -in .players -fill x
  pack .players.labels.name -side left -in .players.labels
  pack .players.labels.team -side right -in .players.labels
  
  if {$numplayers} {
    foreach thisplayer $allplayers {
      drawplayer $names($thisplayer) $ids($thisplayer)
    }
  }
  
  ############ THE OPTIONS!
  
  frame .options -relief ridge -borderwidth 4
  frame .options.m
  frame .options.a
  pack .options -side top -fill x
  pack .options.m .options.a -fill x -in .options
  frame .options.m.f1
  frame .options.m.f2
  frame .options.a.f3

  checkbutton .options.m.f1.bounce -text Bounce -variable BOUNCE
  checkbutton .options.m.f1.decay -text Decay -variable DECAY
  checkbutton .options.m.f1.multishot -text "Multi-shot" -variable MULTISHOT
  checkbutton .options.m.f1.hurts2shoot -text "Hurts to shoot" -variable HURTS2SHOOT
  checkbutton .options.m.f1.repoweronkill -text "Repower on kill" -variable REPOWERONKILL
  checkbutton .options.m.f2.fastheal -text "Fast heal" -variable FASTHEAL
  checkbutton .options.m.f2.fastwalk -text "Fast walk" -variable FASTWALK
  checkbutton .options.m.f2.cloak -text "Cloaking" -variable CLOAK
  checkbutton .options.m.f2.decscore -text "Decrease score" -variable DECSCORE
  checkbutton .options.m.f2.teamshothurt -text "Team shots hurt" -variable TEAMSHOTHURT
  
  button .options.a.f3.beatbutton -text "Beat divider" -command {
    if {1==$BEATDIV} {
      set BEATDIV 2
    } elseif {$BEATDIV==2} {
      set BEATDIV 4
    } elseif {$BEATDIV==4} {
      set BEATDIV 1
    }
  }
  label .options.a.f3.beatlabel -textvariable BEATDIV
  
  pack .options.m.f1 .options.m.f2 -side left -fill x
  pack .options.a.f3 -fill x
  pack .options.m.f1.bounce .options.m.f1.decay -in .options.m.f1 -anchor w
  pack .options.m.f1.multishot .options.m.f1.hurts2shoot -in .options.m.f1 -anchor w
  pack .options.m.f1.repoweronkill -in .options.m.f1 -anchor w
  pack .options.m.f2.fastheal -in .options.m.f2 -anchor w
  pack .options.m.f2.fastwalk .options.m.f2.cloak -in .options.m.f2 -anchor w
  pack .options.m.f2.decscore .options.m.f2.teamshothurt -in .options.m.f2 -anchor w
  pack .options.a.f3.beatbutton .options.a.f3.beatlabel -in .options.a.f3 -anchor w -side left
  
  # messages
  
  frame .msg
  pack .msg -side top -fill x
  entry .msg.entry -textvariable MSGSEND
  pack .msg.entry -fill x -in .msg
  bind .msg.entry <Return> {
    set transcmd "97 "
    append transcmd $MSGSEND
    puts $transcmd
    flush stdout
    unset transcmd
  }
  bind .msg.entry <Any-Motion> {
    sethelp "Type a message here and press enter to send\nit\
             to the xterms of all players."
  }
  
  # quit

  frame .misc
    
  button .misc.b -text "Quit xnetserv" -command {
    puts "9"
    flush stdout
    exit
  }
  button .misc.help -text "Online help" -command {
    if {!$helpon} {
      toplevel .help
      label .help.intro -text "xnetserv by John Goerzen\nOnline help\nMove\
        the mouse over any item\nand help will appear here."
      frame .help.m
      label .help.m.l -textvariable helpmsg -relief sunken -borderwidth 4
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
      pack .help.m.l -in .help.m -side left
    }
  }
      
    
  pack .misc -side top -padx 1m -pady 1m -side top
  pack .misc.b .misc.help -side left -in .misc
  
}

proc intro {} {

  wm title . "Welcome to Netmaze!"
  label .li -text "Welcome to Netmaze!  Click below to begin" -bg midnightblue \
   -fg white
  pack .li
  label .l -text "Version 0.82 (0.81 compat)" -bg black -fg red
  . config -bg black
  image create photo NMLogo -file nmlogo.gif
  button .b -image NMLogo  -command {
    destroy .b .li
    destroy .l
    set isgip 0
    . config -bg #d9d9d9
    mainmenu
    return 0 
  }
  pack .b .l
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

intro
