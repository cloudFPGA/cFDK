

all: memtest 

memtest:
	vivado_hls -f run_hls.tcl

clean: 
	${RM} -rf memtest_prj
	${RM} -f vivado*.log vivado*.jou


