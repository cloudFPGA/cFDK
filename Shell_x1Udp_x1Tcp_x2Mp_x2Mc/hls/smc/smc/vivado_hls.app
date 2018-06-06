<project xmlns="com.autoesl.autopilot.project" top="smc_main" name="smc">
    <includePaths/>
    <libraryPaths/>
    <Simulation>
        <SimFlow name="csim" csimMode="0" lastCsimMode="0" compiler="true"/>
    </Simulation>
    <files xmlns="">
        <file name="../../tb/tb_smc.cpp" sc="0" tb="1" cflags=" -DDEBUG"/>
        <file name="../../smc.cpp" sc="0" tb="1" cflags=" -DDEBUG"/>
        <file name="smc.hpp" sc="0" tb="false" cflags=""/>
        <file name="smc.cpp" sc="0" tb="false" cflags=""/>
    </files>
    <solutions xmlns="">
        <solution name="solution1" status="active"/>
    </solutions>
</project>

