#!/bin/bash 

# try to avoid /opt/Xilinx/Vivado/2017.4/bin/loader: line 194: 28017 Killed                  "$RDI_PROG" "$@" to stop implementation --> restart make command 

echo "called at $(date)" > secureMake.log

while true ; do
	make $1 
	logs=$(find ./xpr/topFMKU60_Flash.runs/ | grep runme.log | tr " " "\n") 
	succes=true 
	#echo $logs 
	for file in $logs 
	do 
		#echo $file
		killed=$(cat $file | grep Killed | wc -l)
		#echo $killed
		if [ $killed -gt 0 ];  then
			succes=false 
			echo "$file was killed " >> secureMake.log
			date >> secureMake.log 
			#echo "$file was killed, removing all logs" >> secureMake.log
			#rm -rf ./xpr/topFMKU60_Flash.runs/*/runme.log
			# log files should be overriden by themselfes
			break;
		fi
	done
	if $succes; then
		break
	fi
done

echo "final done " >> secureMake.log 
date >> secureMake.log

