<project xmlns="com.autoesl.autopilot.project" top="smc_main" name="smc">
    <files>
        <file name="smc.cpp" sc="0" tb="false" cflags=""/>
        <file name="smc.hpp" sc="0" tb="false" cflags=""/>
        <file name="../../smc.cpp" sc="0" tb="1" cflags="-DDEBUG"/>
        <file name="../../tb/tb_smc.cpp" sc="0" tb="1" cflags="-DDEBUG"/>
    </files>
    <includePaths/>
    <libraryPaths/>
    <Simulation>
        <SimFlow name="csim" clean="true" csimMode="0" lastCsimMode="0" compiler="true"/>
    </Simulation>
    <solutions xmlns="">
        <solution name="solution1" status="active"/>
    </solutions>
</project>

