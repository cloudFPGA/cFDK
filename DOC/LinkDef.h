/*******************************************************************************
 * Copyright 2016 -- 2021 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

/*****************************************************************************
 * @file       LinkDef.h
 * @brief      A header file defining a logical hierarchy of groups/subroups
 *             for the documentation of the source code of cloudFPGA.
 * @author     DID
 * @details    The structure is as follows:
 *             
 *             [+] cF cF
 *              +--[+] cFSphere cFSphere
 *              |   +--[+] cFDK cFDK
 *              |       |
 *              |       +--[+] ShellLib SHELL Library
 *              |       |   |
 *              |       |   +--[+] FMC FMC
 *              |       |   +--[+] MEM MEM
 *              |       |   +--[+] NAL NAL
 *              |       |   +--[+] NTS NTS
 *              |       |       +-- NTS_ARP ARP
 *              |       |       |  [+] NTS_ARP_TEST ARP_TEST
 *              |       |       +-- NTS_ICMP ICMP
 *              |       |       |  [+]  NTS_ICMP_TEST ICMP_TEST
 *              |       |       +-- NTS_IPRX IPRX
 *              |       |       |  [+]  NTS_IPRX_TEST IPRX_TEST
 *              |       |       +-- NTS_IPTX IPTX
 *              |       |       |  [+] NTS_IPTX_TEST IPTX_TEST
 *              |       |       +-- NTS_RLB RLB
 *              |       |       |  [+]  NTS_RLB_TEST RLB_TEST
 *              |       |       +-- NTS_TOE TOE
 *              |       |       |  [+]  NTS_TOE_TEST TOE_TEST
 *              |       |       +-- NTS_TOECAM TOECAM
 *              |       |       |  [+]  NTS_TOECAM_TEST TOECAM_TEST
 *              |       |       +-- NTS_UOE UOE
 *              |       |       |  [+]  NTS_UOE_TEST UOE_TEST
 *              |       |       +-- NTS_SIM SimNts
 *              |       +--[+] SRA SRA
 *              |           +--[+] Kale Kale
 *              |           +--[+] Themisto Themisto
 *              +--[+] cFp cFp
 *                  +--[+] cFp_Zoo cFp_Zoo
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  Root Module
 *
 *****************************************************************************/

/** \defgroup cF cF
 *
 *  \brief This is the main cloudFPGA module.
 */


/*****************************************************************************
 *
 *  cF : Submodules
 *
 *****************************************************************************/

/** \defgroup cFSphere cFSphere
 *  @ingroup cF
 *  \brief The cloudFPGA Sphere or cFSphere is what constitutes the core of a cloudFPGA system. It is composed of several major software components, each of them being provided as a standalone cF repository.
 */

/** \defgroup cFp cFp
 *  @ingroup cF
 * 
 *  \brief cloudFPGA Projects (cFp). This is a group with existing projects developed with cFDK and can be used as templates for new projects.
 */

/*****************************************************************************
 *
 *  cFSphere : Submodules
 *
 *****************************************************************************/

/** \defgroup cFDK cFDK
 *  @ingroup cFSphere
 *  \brief This is the cloudFPGA Development Kit (cFDK). The documentation of cFDK is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html .
 */

/*****************************************************************************
 *
 *  cFDK : Submodules
 *
 *****************************************************************************/

/** \defgroup ShellLib SHELL Library
 *  @ingroup cFDK
 * 
 *  \brief This is the SHELL library of cloudFPGA platform with its three basic components, i.e. NTS, FMC, NAL. The documentation of the SHELL library is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#shell-library .
 */

/** \defgroup SRA SRA
 *  @ingroup cFDK
 * 
 *  \brief Shell-Role-Architecture (SRA). To abstract the details of the hardware from the user and to assert certain levels of security, cloudFPGA uses a Shell-Role-Architecture (SRA). The documentation of SRA is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#sras .
 */

/*****************************************************************************
 *
 *  cFDK / ShellLib : Submodules
 *
 *****************************************************************************/

/** \defgroup FMC FMC
 *  @ingroup ShellLib
 * 
 *  \brief FPGA Management Core (FMC). The FMC is the core of the cloudFPGA Shell-Role-Architecture. The FMC controls the FPGA and exposes the management functionality via a RESTful HTTP API. Its tasks and responsibilities are sometimes complex and depend on the current situation and environment. In order to unbundle all these dependencies and to allow future extensions easily, the FMC contains a small Instruction-Set-Architecture (ISA). A detailed documentation of the FMC can be found at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#fpga-management-core-fmc .
 */

/** \defgroup MEM MEM
 *  @ingroup ShellLib
 * 
 *  \brief Memory Sub-System (MEM). This component implements the dynamic memory controllers for the FPGA module FMKU2595 equipped with a XCKU060. This memory sub-system implements two DDR4 memory channels (MC0 and MC1 ), each with a capacity of 8GB. By convention, the memory channel #0 (MC0) is dedicated to the network transport and session (NTS) stack, and the memory channel #1 (MC1) is reserved for the user application. The documentation of MEM is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#memory-sub-system-mem
 */

/** \defgroup NAL NAL
 *  @ingroup ShellLib
 * 
 *  \brief Network Abstraction Layer (NAL). The NAL separates the privileged management traffic from the unprivileged Role traffic. It also abstracts the details of port opening, connection handling and IP Address translation. The documentation of NAL is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#network-abstraction-layer-nal.
 */

/** \defgroup NTS NTS
 *  @ingroup ShellLib
 * 
 *  \brief Network Transport Stack (NTS). This component implements the TCP/IP Network and Transport Stack used by the cloudFPGA platform. The documentation of NTS is available at https://pages.github.ibm.com/cloudFPGA/Doc/pages/cfdk.html#network-transport-stack-nts
 */


/*****************************************************************************
 *
 *  cFDK / SRA : Submodules
 *
 *****************************************************************************/

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


/*****************************************************************************
 *
 *  cFDK / ShellLib / FMC : Submodules
 *
 *****************************************************************************/

// [TODO - Fill in here.]


/*****************************************************************************
 *
 *  cFDK / ShellLib / NAL: Submodules
 *
 *****************************************************************************/

// [TODO - Fill in here.]


/*****************************************************************************
 *
 *  cFDK / ShellLib / NTS : Submodules
 *
 *****************************************************************************/

/** \defgroup NTS_ARP ARP
 *  @ingroup NTS
 * 
 *  \brief Address Resolution Protocol (ARS) server of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_ARP_TEST ARP_TEST
 *  @ingroup NTS_ARP
 * 
 *  \brief Testbench for the Address Resolution Protocol (ARS) server of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_ICMP ICMP
 *  @ingroup NTS
 * 
 *  \brief Internet Control Message Protocol server (ICMP) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_ICMP_TEST ICMP_TEST
 *  @ingroup NTS_ICMP
 * 
 *  \brief Testbench for the Internet Control Message Protocol server (ICMP) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_IPRX IPRX
 *  @ingroup NTS
 * 
 *  \brief IP Receiver packet handler (IPRX) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_IPRX_TEST IPRX_TEST
 *  @ingroup NTS_IPRX
 * 
 *  \brief Testbench for the IP Receiver packet handler (IPRX) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_IPTX IPTX
 *  @ingroup NTS
 * 
 *  \brief IP Transmitter packet handler (IPTX) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_IPTX_TEST IPTX_TEST
 *  @ingroup NTS_IPTX
 * 
 *  \brief Testbench for the IP Transmitter packet handler (IPTX) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_RLB RLB
 *  @ingroup NTS
 * 
 *  \brief Ready Logic Barrier (RLB) for the Network Transport Stack (NTS).
 */

/** \defgroup NTS_RLB_TEST RLB_TEST
 *  @ingroup NTS_RLB
 * 
 *  \brief Testbench for the Ready Logic Barrier (RLB) for the Network Transport Stack (NTS).
 */

/** \defgroup NTS_TOE TOE
 *  @ingroup NTS
 * 
 *  \brief TCP Offload Engine (TOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_TOE_TEST TOE_TEST
 *  @ingroup NTS_TOE
 * 
 *  \brief Testbench for the TCP Offload Engine (TOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_TOECAM TOECAM
 *  @ingroup NTS
 * 
 *  \brief Content-Addressable Memory (CAM) for the TCP Offload Engine (TOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_TOECAM_TEST TOECAM_TEST
 *  @ingroup NTS_TOECAM
 * 
 *  \brief Testbench for the Content-Addressable Memory (CAM) for the TCP Offload Engine (TOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_UOE UOE
 *  @ingroup NTS
 * 
 *  \brief UDP Offload Engine (UOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_UOE_TEST UOE_TEST
 *  @ingroup NTS_UOE
 * 
 *  \brief Testbench for the UDP Offload Engine (UOE) of the Network Transport Stack (NTS).
 */

/** \defgroup NTS_SIM SimNts
 *  @ingroup NTS
 * 
 *  \brief Support and utilities for the simulation of the Network Transport Stack (NTS).
 */


/*****************************************************************************
 *
 *  cFDK / cFp : Submodules
 *
 *****************************************************************************/

/** \defgroup cFp_Zoo cFp_Zoo
 *  @ingroup cFp
 * 
 *  \brief This project is based on Themisto Shell and it features several domain-specific accelerators.
 */

/*****************************************************************************
 *
 *  cFDK / cFp / cFp_Zoo : Submodules
 *
 *****************************************************************************/

/** \defgroup VitisVision cF Vitis Vision
 *  @ingroup cFp_Zoo
 *
 * \brief This is the accelerated functions of Vitis Vision sub-library from Vitis Open Source Library.
 */

/** \defgroup VitisQuantitativeFinance cF Vitis Quantitative Finance
 *  @ingroup cFp_Zoo
 *
 * \brief This is the accelerated functions of Vitis Quantitative Finance sub-library from Vitis Open Source Library.
 */


/*****************************************************************************
 *
 *  cFDK / cFp / cFp_Zoo / VitisVision : Submodules
 *
 *****************************************************************************/


//*********** Harris*****************************
/** \defgroup Harris Harris
 *  @ingroup VitisVision
 *
 * \brief This is the Harris accelerated function from Vitis Vision Open Source Library.
 */

/** \defgroup HarrisTB Harris Testbench
 *  @ingroup Harris
 *
 * \brief This is a subgroup of Harris accelerated function with only testbench-related functions/classes/.
 */

/** \defgroup HarrisHLS Harris Vivado HLS
 *  @ingroup Harris
 *
 * \brief This is a subgroup of Harris accelerated function with only synthesizable (Vivado HLS) functions/classes.
 */

//*********** Gammacorrection ********************
/** \defgroup Gammacorrection Gammacorrection
 *  @ingroup VitisVision
 *
 * \brief This is the Gammacorrection accelerated function from Vitis Vision Open Source Library.
 */

/** \defgroup GammacorrectionTB Gammacorrection Testbench
 *  @ingroup Gammacorrection
 *
 * \brief This is a subgroup of Gammacorrection accelerated function with only testbench-related functions/classes/.
 */

/** \defgroup GammacorrectionHLS Gammacorrection Vivado HLS
 *  @ingroup Gammacorrection
 *
 * \brief This is a subgroup of Gammacorrection accelerated function with only synthesizable (Vivado HLS) functions/classes.
 */


/*****************************************************************************
 *
 *  cFDK / cFp / cFp_Zoo / VitisQuantitativeFinance : Submodules
 *
 *****************************************************************************/


//*********** MCEuropeanEngine*****************************
/** \defgroup MCEuropeanEngine MCEuropeanEngine
 *  @ingroup VitisQuantitativeFinance
 *
 * \brief This is the MCEuropeanEngine accelerated function from Vitis Quantitative Finance Open Source Library.
 */

/** \defgroup MCEuropeanEngineTB MCEuropeanEngine Testbench
 *  @ingroup MCEuropeanEngine
 *
 * \brief This is a subgroup of MCEuropeanEngine accelerated function with only testbench-related functions/classes/.
 */

/** \defgroup MCEuropeanEngineHLS MCEuropeanEngine Vivado HLS
 *  @ingroup MCEuropeanEngine
 *
 * \brief This is a subgroup of MCEuropeanEngine accelerated function with only synthesizable (Vivado HLS) functions/classes.
 */

