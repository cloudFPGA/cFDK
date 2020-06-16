/*****************************************************************************
 * @file       LinkDef.h
 * @brief      A header file defining a logical hierarchy of groups/subroups
 *             for the documentation of the source code of cloudFPGA.
 * @author     DID
 * Copyright 2020 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

// cFDK : The root hierarchy

/** \defgroup cFDK cFDK
 *
 *  \brief This is the cloudFPGA Development Kit (cFDK). The documentation of cFDK is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html .
 */


// cFDK submodule: SHELL Library

/** \defgroup ShellLib SHELL Library
 *  @ingroup cFDK
 * 
 *  \brief This is the SHELL library of cloudFPGA platform with its three basic components, i.e. NTS, FMC, NRC. The documentation of the SHELL library is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#shell-library .
 */


/** \defgroup NTS NTS
 *  @ingroup ShellLib
 * 
 *  \brief Network Transport Stack (NTS). This is the component for the TCP/IP Network and Transport Stack used by the cloudFPGA platform. The documentation of NTS is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#network-transport-stack-nts
 */

/** \defgroup NTS_UOE UDP Offload Engine (UOE).
 *  @ingroup NTS
 * 
 *  \brief UDP Offload Engine (UOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_TOE TCP Offload Engine (TOE).
 *  @ingroup NTS
 * 
 *  \brief TCP Offload Engine (TOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_IPRX IP Receiver packet handler (IPRX).
 *  @ingroup NTS
 * 
 *  \brief IP Receiver packet handler (IPRX) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_IPTX IP Transmitter packet handler (IPRX).
 *  @ingroup NTS
 * 
 *  \brief IP Transmitter packet handler (IPTX) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_ICMP Internet Control Message Protocol server (ICMP)
.
 *  @ingroup NTS
 * 
 *  \brief Internet Control Message Protocol server (ICMP) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_ARP Address Resolution Server (ARS).
 *  @ingroup NTS
 * 
 *  \brief Address Resolution Server (ARS) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_SIM NTS Simulation
 *  @ingroup NTS
 * 
 *  \brief Simulation support and utilities for the Network Transport Stack (NTS).
 */



/** \defgroup FMC FMC
 *  @ingroup ShellLib
 * 
 *  \brief FPGA Management Core (FMC). The FMC is the core of the cloudFPGA Shell-Role-Architecture. Its tasks and responsibilities are sometimes complex and depend on the current situation and environment. In order to unbundle all these dependencies and to allow future extensions easily, the FMC contains a small Instruction-Set-Architecture (ISA). All global operations issue opcodes to execute the current task. The global operations are persistent between IP core runs and react on environment changes, the issued Instructions are all executed in the same IP core run. The global operations are started according to the EMIF Flags in the FMC Write Register (see below). The documentation of the FMC HTTP API can be found at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#fpga-management-core-fmc .
 */

/** \defgroup NRC NRC
 *  @ingroup ShellLib
 * 
 *  \brief Network Routing Core (NRC). The NRC is responsible for managing all UDP/TCP traffic of the FPGA. The documentation of NRC is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#network-routing-core-nrc .
 */



// cFDK submodule: SRA

/** \defgroup SRA SRA
 *  @ingroup cFDK
 * 
 *  \brief Shell-Role-Architecture (SRA). To abstract the details of the hardware from the user and to assert certain levels of security, cloudFPGA uses a Shell-Role-Architecture (SRA). The documentation of SRA is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#sras .
 */

/** \defgroup Themisto Themisto
 *  @ingroup SRA
 * 
 *  \brief This SRA type enables node2node communication within cloudFPGA. The network data streams have now a parallel meta stream to select the destinations or to see the source, respectively. The dual memory port is stream based. The documentation of Themisto is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#the-themisto-sra .
 */

/** \defgroup Kale Kale
 *  @ingroup SRA
 * 
 *  \brief This version of the cF shell is used for the testing and the bring-up of a cloudFPGA (cF) module. The documentation of Kale is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#the-shell-kale .
 */



// cFDK submodule: MIDLW

/** \defgroup MIDLW MIDLW
 *  @ingroup cFDK
 * 
 *  \brief The middleware of cloudFPGA. The documentation of middleware is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#midlw .
 */

