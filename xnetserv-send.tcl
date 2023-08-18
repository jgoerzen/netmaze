#!/usr/bin/wish -f

puts $argv

set cmdlist [split "$argv" "!"]

set i [expr [llength $cmdlist] -1]
set x 0

while {$x<=$i} {
  set sendcommand "[lindex $cmdlist $x]"
  eval send xnetserv.tcl {$sendcommand}
  incr x
}

# set sendcommand "$argv"
# eval send xnetserv.tcl {$sendcommand}

exit
