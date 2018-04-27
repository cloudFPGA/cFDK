

all: memtest 

memtest:
	vivado_hls -f run_hls.tcl
	#mkdir -p ../ip/
	cp -R ./memtest_prj/solution1/impl/ip/ ../

clean: 
	${RM} -rf memtest_prj ../ip/
	${RM} -f vivado*.log vivado*.jou


