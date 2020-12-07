NOTE
==========

This ./ref foler is to store important auto generated files for comparsion with current one. 

I.e. if e.g. addresses of the AXI4Lite Interface changes, make should fail...

**To regenrate the Address checking:**
```
cp  ./nrc_prj/solution1/impl/ip/drivers/nrc_main_v1_0/src/xnrc_main_hw.h > ./ref/xnrc_main_hw.h
sed "s/[ \t]*\/\/.*$//" ref/xnrc_main_hw.h | grep -v '^$' > ref/xnrc_main_hw.h_stripped
```

