
puts    ""
my_puts "End at: [clock format [clock seconds] -format {%T %a %b %d %Y}] "
my_puts "################################################################################"
my_puts "##"
my_puts "##  DONE WITH THE CREATION OF \[ ${nrGenIPs} \] IP CORE(S) FROM THE MANAGED IP REPOSITORY "
my_puts "##"
if { ${nrErrors} != 0 } {
  my_puts "##"
  my_err_puts "##                   Warning: Failed to generate ${nrErrors} IP core(s) ! "
  my_puts "##"
  exit ${KO}
}
my_puts "################################################################################\n"


# Close project
#-------------------------------------------------
close_project

exit ${nrErrors}

