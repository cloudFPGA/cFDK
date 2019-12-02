#!/bin/bash
length=$(cat $0 | grep -v "only" | grep -v "useMPI" | grep -v "vivado" | grep -v "incr" | grep -v "bash" | grep -v "date" | grep -v "echo" | grep -v "grep" | grep -v "more" | wc -l)
if [  $length -lt 12 ]; then
	echo "handle_vivado.tcl: Nothing to do for target. Stop."
else
more +7 $0| head -n9 
vivado -mode batch -source handle_vivado.tcl -notrace -log handle_vivado.log -tclargs -full_src -force \
-forceWithoutBB  -synth -impl -bitgen \
-create -synth \
-create -synth \








fi
retVal=$?
echo "handle_vivado.tcl returned with $retVal"
if [ $retVal -ne 47 ]; then
  echo "\n\tERROR: handle_vivado.tcl finished with an unexpected value!"
  exit -1
else 
  retVal=0
fi
date > end.date
echo "last monolithic build was successfull at " > /home/ngl/gitrepo/cloudFPGA/cFp/cFp_Flash/xpr//.project_monolithic.stamp
date  >> /home/ngl/gitrepo/cloudFPGA/cFp/cFp_Flash/xpr//.project_monolithic.stamp
exit $retVal
