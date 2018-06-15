# REQUIRES xpr_settings to be sourced before!

proc secureSynth {} {
  my_puts "Secure synth started at: [clock format [clock seconds] -format {%T %a %b %d %Y}]"
  set toSynth [get_runs -filter {PROGRESS < 100} *synth*] 
  puts $toSynth
  while { [ llength $toSynth] ne 0 } {
    if { [catch {launch_runs synth_1 -jobs 8}] ne 0} {
      foreach j $toSynth {
        puts "$j"
        reset_run $j 
      }
      my_err_puts "Must reset some synth runs once more: $toSynth"
      reset_run synth_1
      launch_runs synth_1 -jobs 8
    }
    wait_on_run synth_1
    set toSynth [get_runs -filter {PROGRESS < 100} *synth*] 
  
  }

    my_puts "################################################################################"
    my_puts "Secure Synth end at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
}

proc secureImpl {} {
  my_puts "Secure Impl started at: [clock format [clock seconds] -format {%T %a %b %d %Y}]"
  set toImpl [get_runs -filter {PROGRESS < 100} *impl*] 
  puts $toImpl
  set firstRun 1
  while { [ llength $toImpl] ne 0 } {
    if { [catch {launch_runs impl_1 -next_step -jobs 8}] ne 0} {
      foreach j $toImpl {
        puts "$j"
        reset_run $j 
      }
      my_err_puts "Must reset some implemenation runs once more: $toImpl"
      reset_run impl_1
      launch_runs impl_1 -next_step -jobs 8
      if { $firstRun } {
        my_puts "make sure, that synthesis is complete: "
        secureSynth
      } 
      set firstRun 0
    }
    wait_on_run impl_1
    set toImpl [get_runs -filter {PROGRESS < 100} *impl*] 
  }

    my_puts "################################################################################"
    my_puts "Secure Impl end at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
}


proc guidedSynth {} {
  my_puts "Guided synth started at: [clock format [clock seconds] -format {%T %a %b %d %Y}]"
  set toSynth [get_runs -filter {PROGRESS < 100} *synth*] 
  my_puts "to Synth:  $toSynth"

  if { [catch {launch_runs synth_1 -jobs 8}] ne 0} {
    foreach j $toSynth {
      puts "$j"
      reset_run $j 
    }
    my_err_puts "Must reset some synth runs once more: $toSynth"
    #reset_run synth_1
    launch_runs synth_1 -jobs 8
  }
  wait_on_run synth_1

  my_puts "################################################################################"
  my_puts "Guided Synth end at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"
}

proc guidedImpl {} {
  my_puts "Guided Impl started at: [clock format [clock seconds] -format {%T %a %b %d %Y}]"
  set toImpl [get_runs -filter {PROGRESS < 100} *impl*] 
  my_puts "To Impl: $toImpl"

  if { [catch {launch_runs impl_1 -next_step -jobs 8}] ne 0} {
    foreach j $toImpl {
      puts "$j"
      reset_run $j 
    }
    my_err_puts "Must reset some implemenation runs once more: $toImpl"
    # sometimes impl_1 itself is not in toImpl, don't know why
    reset_run impl_1 
    if { [catch {launch_runs impl_1 -next_step -jobs 8}] ne 0} {
      #If still error occurs, then try to resynth
      my_puts "make sure, that synthesis is complete: "
      guidedSynth
    } 
    launch_runs impl_1 -next_step -jobs 8
  }
  wait_on_run impl_1

  my_puts "################################################################################"
  my_puts "Guided Impl end at: [clock format [clock seconds] -format {%T %a %b %d %Y}] \n"

}
