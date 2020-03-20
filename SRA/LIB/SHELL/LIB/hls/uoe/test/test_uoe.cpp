/*****************************************************************************
 * @file       : test_uoe.cpp
 * @brief      : Testbench for the UDP Offload Engine (UOE).
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 *****************************************************************************/

#include "../src/uoe.hpp"
//#include "../../toe/src/toe.hpp"
#include "../../toe/test/test_toe_utils.hpp"

#include <set>
#include <stdio.h>
#include <hls_stream.h>

using namespace hls;
using namespace std;

//---------------------------------------------------------
// HELPERS FOR THE DEBUGGING TRACES
//  .e.g: DEBUG_LEVEL = (MDL_TRACE | IPS_TRACE)
//---------------------------------------------------------
#define THIS_NAME "TB"

#define TRACE_OFF    0x0000
#define TRACE_CGR    1 << 1
#define TRACE_DUMTF  1 << 2
#define TRACE_ALL    0xFFFF

#define DEBUG_LEVEL (TRACE_ALL)

//---------------------------------------------------------
//-- TESTBENCH GLOBAL DEFINES
//    'STARTUP_DELAY' is used to delay the start of the [TB] functions.
//---------------------------------------------------------
#define TB_MAX_SIM_CYCLES     250000
#define TB_STARTUP_DELAY           0
#define TB_GRACE_TIME           5000  // Adds some cycles to drain the DUT before exiting

//---------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//--  These variables might be updated/overwritten by the
//--  content of a test-vector file.
//---------------------------------------------------------
bool            gTraceEvent     = false;
bool            gFatalError     = false;
unsigned int    gSimCycCnt      = 0;
unsigned int    gMaxSimCycles = TB_STARTUP_DELAY + TB_MAX_SIM_CYCLES;

//---------------------------------------------------------
//-- TESTBENCH MODES OF OPERATION
//---------------------------------------------------------
enum TestMode { RX_MODE='0',   TX_MODE='1', BIDIR_MODE='2',
                ECHO_MODE='3', OPEN_MODE='4' };









#define maxOverflowValue 2000

uint32_t clockCounter = 0;

/******************************************************************************
 * @brief Increment the simulation counter
 ******************************************************************************/
void stepSim() {
    gSimCycCnt++;
    if (gTraceEvent || ((gSimCycCnt % 1000) == 0)) {
        printInfo(THIS_NAME, "-- [@%4.4d] -----------------------------\n", gSimCycCnt);
        gTraceEvent = false;
    }
    else if (0) {
        printInfo(THIS_NAME, "------------------- [@%d] ------------\n", gSimCycCnt);
    }
}


ap_uint<64>	convertString(std::string stringToConvert) {
	ap_uint<64>	tempOutput = 0;
	unsigned short int tempValue = 16;
	static const char* const lut = "0123456789ABCDEF";
	
	for (unsigned short int i = 0; i<stringToConvert.size();++i) {
		tempValue = 16;
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == stringToConvert[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(63-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
		else return -1;
	}
	return tempOutput;
}

std::vector<std::string> parseLine(std::string stringBuffer)
{
	std::vector<std::string> tempBuffer;
	bool found = false;

	while (stringBuffer.find(" ") != std::string::npos)	// Search for spaces delimiting the different data words
	{
		std::string temp = stringBuffer.substr(0, stringBuffer.find(" "));							// Split the the string
		stringBuffer = stringBuffer.substr(stringBuffer.find(" ")+1, stringBuffer.length());	// into two
		tempBuffer.push_back(temp);		// and store the new part into the vector. Continue searching until no more spaces are found.
	}
	tempBuffer.push_back(stringBuffer);	// and finally push the final part of the string into the vector when no more spaces are present.
	return tempBuffer;
}

string decodeApUint64(ap_uint<64> inputNumber) {
	std::string 				outputString	= "0000000000000000";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 15;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[15-i] = lut[tempValue];
	}
	return outputString;
}

string decodeApUint32(ap_uint<32> inputNumber) {
	std::string 				outputString	= "00000000";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 7;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[7-i] = lut[tempValue];
	}
	return outputString;
}

string decodeApUint16(ap_uint<16> inputNumber) {
	std::string 				outputString	= "0000";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 3;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[4-i] = lut[tempValue];
	}
	return outputString;
}

string decodeApUint8(ap_uint<8> inputNumber) {
	string 						outputString	= "00";
	unsigned short int			tempValue 		= 16;
	static const char* const	lut 			= "0123456789ABCDEF";
	for (int i = 1;i>=0;--i) {
	tempValue = 0;
	for (unsigned short int k = 0;k<4;++k) {
		if (inputNumber.bit((i+1)*4-k-1) == 1)
			tempValue += static_cast <unsigned short int>(pow(2.0, 3-k));
		}
		outputString[1-i] = lut[tempValue];
	}
	return outputString;
}

ap_uint<64> encodeApUint64(string dataString){
	ap_uint<64> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<dataString.size();++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == dataString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(63-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

ap_uint<32> encodeApUint32(string parseString){
	ap_uint<32> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<8;++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == parseString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(31-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

ap_uint<8> encodeApUint8(string keepString){
	ap_uint<8> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<2;++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == keepString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(7-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}

ap_uint<16> encodeApUint16(string parseString){
	ap_uint<16> tempOutput = 0;
	unsigned short int	tempValue = 16;
	static const char* const	lut = "0123456789ABCDEF";

	for (unsigned short int i = 0; i<4;++i) {
		for (unsigned short int j = 0;j<16;++j) {
			if (lut[j] == parseString[i]) {
				tempValue = j;
				break;
			}
		}
		if (tempValue != 16) {
			for (short int k = 3;k>=0;--k) {
				if (tempValue >= pow(2.0, k)) {
					tempOutput.bit(15-(4*i+(3-k))) = 1;
					tempValue -= static_cast <unsigned short int>(pow(2.0, k));
				}
			}
		}
	}
	return tempOutput;
}


/*****************************************************************************
 * @brief Empty an UdpMeta stream to a DAT file.
 *
 * @param[in/out] ss,       a ref to the UDP metadata stream to drain.
 * @param[in]     ssName,   the name of the UDP metadata stream to drain.
 * @param[in]     fileName, the DAT file to write to.
 * @param[out     nrChunks, a ref to the number of written chunks.
 * @param[out]    nrFrames, a ref to the number of written AXI4 streams.
 * @param[out]    nrBytes,  a ref to the number of written bytes.
  *
 * @return NTS_OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
#ifndef __SYNTHESIS__
    bool drainUdpMetaStreamToFile(stream<metadata> &ss, string ssName,
            string datFile, int &nrChunks, int &nrFrames, int &nrBytes) {  // [TODO - Rename UdpMeta]
        ofstream    outFileStream;
        char        currPath[FILENAME_MAX];
        //string      strLine;
        //int         lineCnt=0;
        metadata      udpMeta;

        const char *myName  = concat3(THIS_NAME, "/", "DUMTF");

        //-- REMOVE PREVIOUS FILE
        remove(ssName.c_str());

        //-- OPEN FILE
        if (!outFileStream.is_open()) {
            outFileStream.open(datFile.c_str(), ofstream::out);
            if (!outFileStream) {
                printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", datFile.c_str());
                return(NTS_KO);
            }
        }

        // Assess that file has ".dat" extension
        if ( datFile.find_last_of ( '.' ) != string::npos ) {
            string extension ( datFile.substr( datFile.find_last_of ( '.' ) + 1 ) );
            if (extension != "dat") {
                printError(THIS_NAME, "Cannot dump SocketPair stream to file \'%s\' because file is not of type \'DAT\'.\n", datFile.c_str());
                outFileStream.close();
                return(NTS_KO);
            }
        }

        //-- READ FROM STREAM AND WRITE TO FILE
        outFileStream << std::hex << std::noshowbase;
        outFileStream << std::setfill('0');
        outFileStream << std::uppercase;
        while (!(ss.empty())) {
            ss.read(udpMeta);
            SocketPair socketPair(SockAddr(udpMeta.sourceSocket.addr,
                                           udpMeta.sourceSocket.port),
                                  SockAddr(udpMeta.destinationSocket.addr,
                                           udpMeta.destinationSocket.port));
            writeSocketPairToFile(socketPair, outFileStream);
            nrChunks++;
            nrBytes += 12;
            nrFrames++;
            if (DEBUG_LEVEL & TRACE_DUMTF) {
                printInfo(myName, "Writing new socket-pair to file:\n");
                printSockPair(myName, socketPair);
            }
        }

        //-- CLOSE FILE
        outFileStream.close();

        return(NTS_OK);

    }
#endif


/*****************************************************************************
 * @brief Create the golden Rx reference files from an input test file.
 *
 * @param[in] inpData_FileName, the input data file to generate from.
 * @param[in] outData_GoldName, the output data gold file to create.
 * @param[in] outMeta_GoldName, the output meta gold file to create.
 * @param[in] udpPortSet,       a ref to an associative container which holds
 *                              the UDP destination ports.
 *
 * @return NTS_ OK if successful,  otherwise NTS_KO.
 ******************************************************************************/
int createGoldenRxFiles(
        string        inpData_FileName,
        string        outData_GoldName,
        string        outMeta_GoldName,
        set<UdpPort> &udpPorts)
{
    const char *myName  = concat3(THIS_NAME, "/", "CGF");

    ifstream    ifsData;
    ofstream    ofsDataGold;
    ofstream    ofsMetaGold;

    char        currPath[FILENAME_MAX];
    int         ret=NTS_OK;
    int         inpChunks=0,  outChunks=0;
    int         inpPackets=0, outPackets=0;
    int         inpBytes=0,   outBytes=0;

    //-- STEP-1 : OPEN INPUT FILE AND ASSESS ITS EXTENSION
    ifsData.open(inpData_FileName.c_str());
    if (!ifsData) {
        getcwd(currPath, sizeof(currPath));
        printError(myName, "Cannot open the file: %s \n\t (FYI - The current working directory is: %s) \n",
                   inpData_FileName.c_str(), currPath);
        return(NTS_KO);
    }
    if (not isDatFile(inpData_FileName)) {
        printError(myName, "Cannot create golden files from input file \'%s\' because file is not of type \'.dat\'.\n",
                   inpData_FileName.c_str());
        ifsData.close();
        return(NTS_KO);
    }

    //-- STEP-2 : OPEN THE OUTPUT GOLD FILES
    remove(outData_GoldName.c_str());
    remove(outMeta_GoldName.c_str());
    if (!ofsDataGold.is_open()) {
        ofsDataGold.open (outData_GoldName.c_str(), ofstream::out);
        if (!ofsDataGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outData_GoldName.c_str());
        }
    }
    if (!ofsMetaGold.is_open()) {
        ofsMetaGold.open (outMeta_GoldName.c_str(), ofstream::out);
        if (!ofsMetaGold) {
            printFatal(THIS_NAME, "Could not open the output gold file \'%s\'. \n",
                       outMeta_GoldName.c_str());
        }
    }

    //-- STEP-3 : READ AND PARSE THE INPUT UDP FILE
    while ((ifsData.peek() != EOF) && (ret != NTS_KO)) {
        IpPacket    ip4DataPkt; // [FIXME-create a SimIp4Packet class]
        AxisIp4     ip4RxData;
        bool        endOfPkt=false;
        bool        rc;
        // Read one packet at a time from input file
        while ((ifsData.peek() != EOF) && (!endOfPkt)) {
            rc = readAxiWordFromFile(&ip4RxData, ifsData);
            if (rc) {
                if (ip4RxData.isValid()) {
                    ip4DataPkt.push_back(ip4RxData);
                    if (ip4RxData.tlast == 1) {
                        inpPackets++;
                        endOfPkt = true;
                    }
                }
                else {
                    // We always abort the stream as this point by asserting
                    // 'tlast' and de-asserting 'tkeep'.
                    ip4DataPkt.push_back(AxiWord(ip4RxData.tdata, 0x00, 1));
                    inpPackets++;
                    endOfPkt = true;
                }
                inpChunks++;
                inpBytes += ip4RxData.keepToLen();
            }
        } // End-of: while ((ifsData.peek() != EOF) && (!endOfPkt))
        // Build the UDP datagram and corresponding metada expected at the output of UOE
        if (endOfPkt) {
            // Check that the incoming IPv4 packet is UDP
            Ip4Prot ip4Prot = ip4DataPkt.getIpProtocol();
            if (ip4Prot != UDP_PROTOCOL) {
                printWarn(myName, "IP packet #%d is dropped because it is not an UDP packet.\n");
                printInfo(myName, "  Received Ip4Prot = 0x%2.2X\n", ip4Prot.to_uchar());
                printInfo(myName, "  Expected Ip4Prot = 0x%2.2X\n", UDP_PROTOCOL.to_uint());
                ret = NTS_KO;
                continue;
            }
            // Retrieve the UDP datagram from the IPv4 Packet
            UdpDatagram udpDatagram;  // [FIXME-create a class SimUdpDatagram]
            udpDatagram = ip4DataPkt.getUdpDatagram();
            // Assess the checksum is valid (or 0x0000)
            UdpCsum udpHCsum = udpDatagram.getUdpChecksum();
            UdpCsum calcCsum = udpDatagram.reCalculateUdpChecksum(ip4DataPkt.getIpSourceAddress(),
                                                                  ip4DataPkt.getIpDestinationAddress(),
                                                                  ip4DataPkt.getIpProtocol());
            if ((udpHCsum != 0) and (udpHCsum != calcCsum)) {
                // UDP datagram comes with an invalid checksum
                printWarn(myName, "IP4 packet #%d contains an UDP datgram with an invalid checksum. It will be dropped by the UDP core.\n", inpPackets);
                printWarn(myName, "\tFound checksum field=0x%4.4X, Was expecting 0x%4.4X)\n",
                          udpHCsum.to_uint(), calcCsum.to_ushort());
            }
            else {
                // Part-1: Metadata
                SocketPair socketPair(SockAddr(ip4DataPkt.getIpSourceAddress(),
                                               ip4DataPkt.getUdpSourcePort()),
                                      SockAddr(ip4DataPkt.getIpDestinationAddress(),
                                               ip4DataPkt.getUdpDestinationPort()));
                writeSocketPairToFile(socketPair, ofsMetaGold);
                if (DEBUG_LEVEL & TRACE_CGR) {
                    printInfo(myName, "Writing new socket-pair to file:\n");
                    printSockPair(myName, socketPair);
                }
                // Part-2: Update the UDP container set
                udpPorts.insert(ip4DataPkt.getUdpDestinationPort());
                // Part-3: Write UDP datagram payload to gold file

                if (udpDatagram.writePayloadToDatFile(ofsDataGold) == false) {
                    printError(myName, "Failed to write UDP payload to GOLD file.\n");
                    ret = NTS_KO;
                }
                else {
                    outPackets += 1;
                    outChunks  += udpDatagram.size();
                    outBytes   += udpDatagram.length();
                }
            }
        } // End-of: if (endOfPkt)
    } // End-of: while ((ifsData.peek() != EOF) && (ret != NTS_KO))

    //-- STEP-4: CLOSE FILES
    ifsData.close();
    ofsDataGold.close();
    ofsMetaGold.close();

    //-- STEP-5: PRINT RESULTS
    printInfo(myName, "Done with the creation of the golden files.\n");
    printInfo(myName, "\tProcessed %5d chunks in %4d packets, for a total of %6d bytes.\n",
              inpChunks, inpPackets, inpBytes);
    printInfo(myName, "\tGenerated %5d chunks in %4d packets, for a total of %6d bytes.\n",
              outChunks, outPackets, outBytes);
    return(ret);
}

/*****************************************************************************
 * @brief Main function.
 *
 * @param[in]  mode,     The test mode (0=RX_MODE, 1=TX_MODE, 2=BIDIR_MODE).
 * @param[in]  inpFile1, The pathname of the 1st input test vector file.
 * @param[in]  inpFile2, The pathname of the 2nd input test vector file.
 *
 * @remark:
 *  The number of input parameters is variable and depends on the testing mode.
 *   Example (see also file '../run_hls.tcl'):
 *    csim_design -argv "0 ../../../../test/testVectors/siIPRX_OneDatagram.dat"
 *   The possible options can be summarized as follows:
 *       IF (mode==0)
 *         inpFile1 = siIPRX_<FileName>.dat
 *       ELSE-IF (mode == 1)
 *         inpFile1 = siURIF_<Filename>.dat
 *       ELSE-IF (mode == 2)
 *         inpFile1 = siIPRX_<FileName>.dat
 *         inpFile2 = siURIF_<Filename>.dat
 *
 ******************************************************************************/
int main(int argc, char *argv[]) {

    gSimCycCnt = 0;

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr  = 0;        // Tb error counter.
    char        tbMode = *argv[1]; // Indicates the TB testing mode.

    UdpPort     portToOpen;
    StsBool     openReply;

    AxisIp4                 inputPathInputData          = AxisIp4(0, 0, 0);
	metadata				inputPathInputMetadata		= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));
	axiWord					inputPathOutputData			= axiWord(0, 0, 0);
	ap_uint<16>				openPortData				= 0;
	bool					portStatusData				= false;
	metadata				inputPathOutputMetadata 	= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));
    axiWord                 outputPathInData            = axiWord(0, 0, 0);
    //OBSOLETE_20200310 	axiWord					outputPathOutData			= axiWord(0, 0, 0);
	ap_uint<16> 			outputPathInputLength		= 0;
	metadata				outputPathInputMetadata		= metadata(sockaddr_in(0, 0), sockaddr_in(0, 0));

	/// Input File Temp Variables ///
	uint16_t				inputSourceIP				= 0;
	uint16_t				inputDestinationIP			= 0;
	//uint64_t 				dataString					= 0;
	std::string 			dataString;
	axiWord					inputData					= axiWord(0, 0, 0);
	uint16_t				sop 						= 0;
	uint16_t				eop							= 0;
	uint16_t				mod							= 0;
	uint16_t				valid						= 0;
	uint32_t				errCount					= 0;

	ifstream rxInput;
	ifstream txInput;
	ofstream rxOutput;
	ofstream txOutput;
	ofstream portStatus;
	ifstream goldenOutputRx;
	ifstream goldenOutputTx;

	uint32_t				overflowCounter				= 0;

    //------------------------------------------------------
    //-- DUT STREAM INTERFACES and RELATED VARIABLEs
    //------------------------------------------------------
    stream<AxisIp4>         ssIPRX_UOE_Data    ("ssIPRX_UOE_Data");
    int                     nrIPRX_UOE_Chunks  = 0;
    int                     nrIPRX_UOE_Packets = 0;
    int                     nrIPRX_UOE_Bytes   = 0;
    stream<axiWord>			ssUOE_IPTX_Data    ("ssUOE_IPTX_Data");

    stream<UdpPort>         ssURIF_UOE_OpnReq  ("ssURIF_UOE_OpnReq");
    stream<StsBool>         ssUOE_URIF_OpnRep  ("ssUOE_URIF_OpnRep");
    stream<UdpPort>         ssURIF_UOE_ClsReq  ("ssURIF_UOE_ClsReq");

    stream<AppData>         ssUOE_URIF_Data    ("ssUOE_URIF_Data");
    int                     nrUOE_URIF_DataChunks = 0;
    int                     nrUOE_URIF_DataGrams  = 0;
    int                     nrUOE_URIF_DataBytes  = 0;
    stream<metadata>        ssUOE_URIF_Meta    ("ssUOE_URIF_Meta");
    int                     nrUOE_URIF_MetaChunks = 0;
    int                     nrUOE_URIF_MetaGrams  = 0;
    int                     nrUOE_URIF_MetaBytes  = 0;

    stream<axiWord> 		ssURIF_UOE_Data    ("ssURIF_UOE_Data");
	stream<metadata> 		ssURIF_UOE_Meta    ("ssURIF_UOE_Meta");
	stream<ap_uint<16> > 	ssURIF_UOE_PLen    ("ssURIF-UOE_PLen");

	stream<AxiWord>			soICMP_Data        ("soICMP_Data");

    //------------------------------------------------------
    //-- PARSING THE TESBENCH ARGUMENTS
    //------------------------------------------------------
    if (argc < 3) {
        printFatal(THIS_NAME, "Expected a minimum of 2 or 3 parameters with one of the following synopsis:\n \t\t mode(0|3) siIPRX_<Filename>.dat\n \t\t mode(1)   siURIF_<Filename>.dat\n \t\t mode(2)   siIPRX_<Filename>.dat siURIF_<Filename>.dat\n");
    }
    switch (tbMode=*argv[1]) {
    case RX_MODE:
        break;
    case TX_MODE:
        printFatal(THIS_NAME, "Not yet implemented...\n");
        break;
    case BIDIR_MODE:
        printFatal(THIS_NAME, "Not yet implemented...\n");
        break;
    case ECHO_MODE:
        printFatal(THIS_NAME, "Not yet implemented...\n");
        break;
    case OPEN_MODE:
        printFatal(THIS_NAME, "Not yet implemented...\n");
        break;
    default:
        printFatal(THIS_NAME, "Expected a minimum of 2 or 3 parameters with one of the following synopsis:\n \t\t mode(0|3) siIPRX_<Filename>.dat\n \t\t mode(1)   siURIF_<Filename>.dat\n \t\t mode(2)   siIPRX_<Filename>.dat siURIF_<Filename>.dat\n");
    }
    printInfo(THIS_NAME, "This run executes in mode \'%c\'.\n", tbMode);

/*** OBSOLETE-20200313 ****************
	errCount = 0;
	int counter = 0;
	if (argc != 3) {
		std::cout << "You need to provide at least one parameter (the input file name)!" << std::endl;
		return -1;
	}
	rxInput.open(argv[1]);
	if (!rxInput)	{
		std::cout << " Error opening Rx input file!" << std::endl;
		return -1;
	}
	txInput.open(argv[2]);
	if (!txInput) {
		std::cout << " Error opening Tx input file!" << std::endl;
		return -1;
	}
	rxOutput.open("rxOutput.dat");
	if (!rxOutput) {
		std::cout << " Error opening output file!" << std::endl;
		return -1;
	}
	portStatus.open("portStatus.dat");
	if (!portStatus) {
		std::cout << " Error opening port status file!" << std::endl;
		return -1;
	}
	txOutput.open("txOutput.dat");
	if (!txOutput) {
		cout << "Error openingn Tx Output file!" << endl;
		return -1;
	}
	goldenOutputRx.open("../../../../test/rxGoldenOutput.short.dat");
	if (!goldenOutputRx) {
		cout << "Error opening golden output file for the Rx side. Check that the correct path is provided!" << endl;
		return -1;
	}
	goldenOutputTx.open("../../../../test/txGoldenOutput.short.dat");
	if (!goldenOutputTx) {
		cout << "Error opening golden output file for the Tx side. Check that the correct path is provided!" << endl;
		return -1;
	}
	std::cerr << "Input File: " << argv[1] << std::endl << std::endl;
	***********************************/

    printInfo(THIS_NAME, "############################################################################\n");
    printInfo(THIS_NAME, "## TESTBENCH STARTS HERE                                                  ##\n");
    printInfo(THIS_NAME, "############################################################################\n\n");

    // [TODO] int tbRun = nrETH_IPRX_Chunks + TB_GRACE_TIME;

    if (tbMode == OPEN_MODE) {
        //---------------------------------------------------------------
        //-- OPEN_MODE: Attempt to close a port that isn't open.
        //--    Expected response is: Nothing.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #1 : Request to close a port that isn't open.\n");
        ssURIF_UOE_ClsReq.write(0x1532);
        for (int i=0; i<10; ++i) {
        uoe(
            //-- IPRX / IP Rx / Data Interface
            ssIPRX_UOE_Data,
            //-- IPTX / IP Tx / Data Interface
            ssUOE_IPTX_Data,
            //-- URIF / Control Port Interfaces
            ssURIF_UOE_OpnReq,
            ssUOE_URIF_OpnRep,
            ssURIF_UOE_ClsReq,
            //-- URIF / Rx Data Interfaces
            ssUOE_URIF_Data,
            ssUOE_URIF_Meta,
            //-- URIF / Tx Data Interfaces
            ssURIF_UOE_Data,
            ssURIF_UOE_Meta,
            ssURIF_UOE_PLen,
            //-- ICMP / Message Data Interface
            soICMP_Data);
        //-- INCREMENT GLOBAL SIMULATION COUNTER
        stepSim();
    }
        printInfo(THIS_NAME, "== OPEN-TEST #1 : Done.\n\n");

        //---------------------------------------------------------------
        //-- OPEN_MODE: Atempt to open a new port.
        //--     Expected response is: Port opened successfully
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #2 : Request to open a port.\n");
        portToOpen = 0x80;
        printInfo(THIS_NAME, "Now - Trying to open port #%d.\n", portToOpen.to_int());
        ssURIF_UOE_OpnReq.write(0x80);
        for (int i=0; i<3; ++i) {
            uoe(
            //-- IPRX / IP Rx / Data Interface
            ssIPRX_UOE_Data,
            //-- IPTX / IP Tx / Data Interface
            ssUOE_IPTX_Data,
            //-- URIF / Control Port Interfaces
            ssURIF_UOE_OpnReq,
            ssUOE_URIF_OpnRep,
            ssURIF_UOE_ClsReq,
            //-- URIF / Rx Data Interfaces
            ssUOE_URIF_Data,
            ssUOE_URIF_Meta,
            //-- URIF / Tx Data Interfaces
            ssURIF_UOE_Data,
            ssURIF_UOE_Meta,
            ssURIF_UOE_PLen,
            //-- ICMP / Message Data Interface
            soICMP_Data);
        //-- INCREMENT GLOBAL SIMULATION COUNTER
        stepSim();
    }
        openReply = false;
        if (!ssUOE_URIF_OpnRep.empty()) {
        openReply = ssUOE_URIF_OpnRep.read();
        if (openReply) {
            printInfo(THIS_NAME, "OK - Port #%d was successfully opened.\n", portToOpen.to_int());
        }
        else {
            printError(THIS_NAME, "KO - Failed to open port #%d.\n", portToOpen.to_int());
            nrErr++;
        }
    }
        else {
        printError(THIS_NAME, "NoReply - Failed to open port #%d.\n", portToOpen.to_int());
        nrErr++;
    }
        printInfo(THIS_NAME, "== OPEN-TEST #2 : Done.\n\n");

        //---------------------------------------------------------------
        //-- OPEN_MODE: Close an already open port
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #3 : Request to close an opened port.\n");
        UdpPort portToClose = portToOpen;
        printInfo(THIS_NAME, "Now - Trying to close port #%d.\n", portToOpen.to_int());
        ssURIF_UOE_ClsReq.write(portToClose);
        for (int i=0; i<10; ++i) {
        uoe(
            //-- IPRX / IP Rx / Data Interface
            ssIPRX_UOE_Data,
            //-- IPTX / IP Tx / Data Interface
            ssUOE_IPTX_Data,
            //-- URIF / Control Port Interfaces
            ssURIF_UOE_OpnReq,
            ssUOE_URIF_OpnRep,
            ssURIF_UOE_ClsReq,
            //-- URIF / Rx Data Interfaces
            ssUOE_URIF_Data,
            ssUOE_URIF_Meta,
            //-- URIF / Tx Data Interfaces
            ssURIF_UOE_Data,
            ssURIF_UOE_Meta,
            ssURIF_UOE_PLen,
            //-- ICMP / Message Data Interface
            soICMP_Data);
        //-- INCREMENT GLOBAL SIMULATION COUNTER
        stepSim();
        }
        printInfo(THIS_NAME, "== OPEN-TEST #3 : Done.\n\n");

        //---------------------------------------------------------------
        //-- OPEN_MODE: Read in the test input data for the Rx side
        //--         without opening the port.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== OPEN-TEST #4 : Send IPv4 traffic to a closed port.\n");
        while (!rxInput.eof()) {
    		std::string stringBuffer;
    		getline(rxInput, stringBuffer);
    		std::vector<std::string> stringVector = parseLine(stringBuffer);
    		string dataString = stringVector[0];
    		string keepString = stringVector[2];
    		inputPathInputData.tdata = encodeApUint64(dataString);
    		inputPathInputData.tkeep = encodeApUint8(keepString);
    		inputPathInputData.tlast = atoi(stringVector[1].c_str());
    		ssIPRX_UOE_Data.write(inputPathInputData);
    		//noOfLines++;
    	}
        int tbRun = /*nrIPRX_UOE_Chunks*/ 1000 + TB_GRACE_TIME;
        while (tbRun) {
            uoe(
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_OpnReq,
                ssUOE_URIF_OpnRep,
                ssURIF_UOE_ClsReq,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_PLen,
                //-- ICMP / Message Data Interface
                soICMP_Data);
            tbRun--;
            stepSim();
            if (!ssUOE_URIF_Data.empty()) {
                AppData appData = ssUOE_URIF_Data.read();
                printError(THIS_NAME, "Received unexpected data from [UOE]");
                printAxiWord(THIS_NAME, appData);
                nrErr++;
            }
        }
        rxInput.close();
        if (nrErr == 0) {
            printInfo(THIS_NAME, "== OPEN-TEST #4 : Passed.\n\n");
        }
        else {
            printError(THIS_NAME, "== OPEN-TEST #4 : Failed. \n\n");
        }
    } // End-of: if (tbMode == OPEN_MODE)

    if (tbMode == RX_MODE) {
        //---------------------------------------------------------------
        //-- RX_MODE: Read in the test input data for the IPRX side.
        //--    This run will start by opening all the required ports in
        //--    listen mode.
        //---------------------------------------------------------------
        printInfo(THIS_NAME, "== RX-TEST : Send IPv4 traffic to an opened port.\n");

        //-- CREATE DUT INPUT TRAFFIC AS STREAMS --------------------
        if (feedAxisFromFile(ssIPRX_UOE_Data, "ssIPRX_UOE_Data", string(argv[2]),
                nrIPRX_UOE_Chunks, nrIPRX_UOE_Packets, nrIPRX_UOE_Bytes)) {
            printInfo(THIS_NAME, "Done with the creation of the input traffic as streams:\n");
            printInfo(THIS_NAME, "\tGenerated %d chunks in %d datagrams, for a total of %d bytes.\n\n",
                nrIPRX_UOE_Chunks, nrIPRX_UOE_Packets, nrIPRX_UOE_Bytes);
        }
        else {
            printError(THIS_NAME, "Failed to create traffic as input stream. \n");
            nrErr++;
        }

        //-- CREATE DUT OUTPUT TRAFFIC AS STREAMS -------------------
        string           ofsURIF_Data_FileName = "../../../../test/soURIF_Data.dat";
        string           ofsURIF_Meta_FileName = "../../../../test/soURIF_Meta.dat";
        string           ofsURIF_Gold_Data_FileName = "../../../../test/soURIF_Gold_Data.dat";
        string           ofsURIF_Gold_Meta_FileName = "../../../../test/soURIF_Gold_Meta.dat";
        vector<string>   ofNames;
        ofNames.push_back(ofsURIF_Data_FileName);
        ofNames.push_back(ofsURIF_Meta_FileName);
        ofstream         ofStreams[ofNames.size()]; // Stored in the same order

        //-- Remove previous old files and open new files
        for (int i = 0; i < ofNames.size(); i++) {
            remove(ofNames[i].c_str());
            if (not isDatFile(ofNames[i])) {
                printError(THIS_NAME, "File \'%s\' is not of type \'DAT\'.\n", ofNames[i].c_str());
                nrErr++;
                continue;
            }
            if (!ofStreams[i].is_open()) {
                ofStreams[i].open(ofNames[i].c_str(), ofstream::out);
                if (!ofStreams[i]) {
                    printError(THIS_NAME, "Cannot open the file: \'%s\'.\n", ofNames[i].c_str());
                    nrErr++;
                    continue;
                }
            }
        }

        //-- CREATE OUTPUT GOLD TRAFFIC
        set<UdpPort> udpDstPorts;
        if (createGoldenRxFiles(string(argv[2]), ofsURIF_Gold_Data_FileName,
                ofsURIF_Gold_Meta_FileName, udpDstPorts) != NTS_OK) {
            printError(THIS_NAME, "Failed to create golden Rx files. \n");
            nrErr++;
        }

        // Request to open a set of UDP ports in listen mode
        for (set<UdpPort>::iterator it=udpDstPorts.begin(); it!=udpDstPorts.end(); ++it) {
            portToOpen = *it;
            ssURIF_UOE_OpnReq.write(portToOpen);
            for (int i=0; i<1; ++i) {
                uoe(
                    //-- IPRX / IP Rx / Data Interface
                    ssIPRX_UOE_Data,
                    //-- IPTX / IP Tx / Data Interface
                    ssUOE_IPTX_Data,
                    //-- URIF / Control Port Interfaces
                    ssURIF_UOE_OpnReq,
                    ssUOE_URIF_OpnRep,
                    ssURIF_UOE_ClsReq,
                    //-- URIF / Rx Data Interfaces
                    ssUOE_URIF_Data,
                    ssUOE_URIF_Meta,
                    //-- URIF / Tx Data Interfaces
                    ssURIF_UOE_Data,
                    ssURIF_UOE_Meta,
                    ssURIF_UOE_PLen,
                    //-- ICMP / Message Data Interface
                    soICMP_Data);
                stepSim();
            }
        }
        // Check that port were successfully opened
        for (set<UdpPort>::iterator it=udpDstPorts.begin(); it!=udpDstPorts.end(); ++it) {
            portToOpen = *it;
            openReply = false;
            if (!ssUOE_URIF_OpnRep.empty()) {
                openReply = ssUOE_URIF_OpnRep.read();
                if (not openReply) {
                    printError(THIS_NAME, "KO - Failed to open port #%d.\n", portToOpen.to_int());
                }
            }
            else {
                printError(THIS_NAME, "NoReply - Failed to open port #%d.\n", portToOpen.to_int());
                nrErr++;
            }
        }

        //-- SEND THE IPv4 packets to [UOE]
        int tbRun = (nrErr == 0) ? (nrIPRX_UOE_Chunks + TB_GRACE_TIME) : 0;
        while (tbRun) {
            uoe(
                //-- IPRX / IP Rx / Data Interface
                ssIPRX_UOE_Data,
                //-- IPTX / IP Tx / Data Interface
                ssUOE_IPTX_Data,
                //-- URIF / Control Port Interfaces
                ssURIF_UOE_OpnReq,
                ssUOE_URIF_OpnRep,
                ssURIF_UOE_ClsReq,
                //-- URIF / Rx Data Interfaces
                ssUOE_URIF_Data,
                ssUOE_URIF_Meta,
                //-- URIF / Tx Data Interfaces
                ssURIF_UOE_Data,
                ssURIF_UOE_Meta,
                ssURIF_UOE_PLen,
                //-- ICMP / Message Data Interface
                soICMP_Data);
            tbRun--;
            stepSim();
        }

        //-- DRAIN UOE-->URIF DATA OUTPUT STREAM
        if (not drainAxiWordStreamToFile(ssUOE_URIF_Data, "ssUOE_URIF_Data",
                ofNames[0], nrUOE_URIF_DataChunks, nrUOE_URIF_DataGrams, nrUOE_URIF_DataBytes)) {
            printError(THIS_NAME, "Failed to drain UOE-to-URIF dat traffic from DUT. \n");
            nrErr++;
        }
        //-- DRAIN UOE-->URIF META OUTPUT STREAM
        if (not drainUdpMetaStreamToFile(ssUOE_URIF_Meta, "ssUOE_URIF_Meta",
                ofNames[1], nrUOE_URIF_MetaChunks, nrUOE_URIF_MetaGrams, nrUOE_URIF_MetaBytes)) {
            printError(THIS_NAME, "Failed to drain UOE-to-URIF meta traffic from DUT. \n");
            nrErr++;
        }

        printInfo(THIS_NAME, "############################################################################\n");
        printInfo(THIS_NAME, "## TESTBENCH ENDS HERE                                                    ##\n");
        printInfo(THIS_NAME, "############################################################################\n");

        //---------------------------------------------------------------
        //-- COMPARE OUTPUT DAT and GOLD STREAMS
        //---------------------------------------------------------------
        int res = system(("diff --brief -w " + \
                           std::string(ofsURIF_Data_FileName) + " " + \
                           std::string(ofsURIF_Gold_Data_FileName) + " ").c_str());
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsURIF_Data_FileName.c_str(), ofsURIF_Gold_Data_FileName.c_str());
            nrErr += 1;
        }
        res = system(("diff --brief -w " + \
                       std::string(ofsURIF_Meta_FileName) + " " + \
                       std::string(ofsURIF_Gold_Meta_FileName) + " ").c_str());
        if (res) {
            printError(THIS_NAME, "File \'%s\' does not match \'%s\'.\n", \
                       ofsURIF_Meta_FileName.c_str(), ofsURIF_Gold_Meta_FileName.c_str());
            nrErr += 1;
        }

    } // End-of: if (tbMode == RX_MODE)

    //---------------------------------------------------------------
    //-- PRINT TESTBENCH STATUS
    //---------------------------------------------------------------
    printf("\n\n");
    printInfo(THIS_NAME, "This testbench was executed with the following parameters: \n");
    printInfo(THIS_NAME, "\t==> TB Mode  = %c\n", *argv[1]);
    for (int i=2; i<argc; i++) {
        printInfo(THIS_NAME, "\t==> Param[%d] = %s\n", (i-1), argv[i]);
    }

    if (nrErr) {
        printError(THIS_NAME, "###########################################################\n");
        printError(THIS_NAME, "#### TEST BENCH FAILED : TOTAL NUMBER OF ERROR(S) = %2d ####\n", nrErr);
        printError(THIS_NAME, "###########################################################\n\n");

        printInfo(THIS_NAME, "FYI - You may want to check for \'ERROR\' and/or \'WARNING\' alarms in the LOG file...\n\n");
    }
    else {
        printInfo(THIS_NAME, "#############################################################\n");
        printInfo(THIS_NAME, "####               SUCCESSFUL END OF TEST                ####\n");
        printInfo(THIS_NAME, "#############################################################\n");
    }

    return nrErr;


	//** Test 5: Tx path **************************
    uint32_t noOfLines = 0;
	cerr << endl << "Test 5: Exercising the Tx Path" << endl;
	noOfLines = 0;
	overflowCounter = 0;
	while (!txInput.eof()) {
		std::string stringBuffer;
		getline(txInput, stringBuffer);
		std::vector<std::string> stringVector = parseLine(stringBuffer);
		string sourcePort 		= stringVector[0];
		string destinationPort 	= stringVector[1];
		string sourceIP			= stringVector[2];
		string destinationIP	= stringVector[3];
		string payloadLength	= stringVector[4];
		string dataString 		= stringVector[5];
		string keepString 		= stringVector[6];
		outputPathInData.data 	= encodeApUint64(dataString);
		outputPathInData.keep	= encodeApUint8(keepString);
		outputPathInData.last 	= atoi(stringVector[7].c_str());
		outputPathInputMetadata.sourceSocket.port		= encodeApUint16(sourcePort);
		outputPathInputMetadata.sourceSocket.addr		= encodeApUint32(sourceIP);
		outputPathInputMetadata.destinationSocket.port	= encodeApUint16(destinationPort);
		outputPathInputMetadata.destinationSocket.addr 	= encodeApUint32(destinationIP);
		outputPathInputLength	= encodeApUint16(payloadLength);
		ssURIF_UOE_Data.write(outputPathInData);
		if (outputPathInputLength != 0) {
			ssURIF_UOE_Meta.write(outputPathInputMetadata);
			ssURIF_UOE_PLen.write(outputPathInputLength);
		}
		noOfLines++;
	}
	noOfLines = 0;
	while (!ssURIF_UOE_Data.empty() || overflowCounter < maxOverflowValue) {
		uoe(
			//-- IPRX / IP Rx / Data Interface
			ssIPRX_UOE_Data,
			//-- IPTX / IP Tx / Data Interface
			ssUOE_IPTX_Data,
			//-- URIF / Control Port Interfaces
			ssURIF_UOE_OpnReq,
			ssUOE_URIF_OpnRep,
			ssURIF_UOE_ClsReq,
			//-- URIF / Rx Data Interfaces
			ssUOE_URIF_Data,
			ssUOE_URIF_Meta,
			//-- URIF / Tx Data Interfaces
			ssURIF_UOE_Data,
			ssURIF_UOE_Meta,
			ssURIF_UOE_PLen,
			//-- ICMP / Message Data Interface
			soICMP_Data);

		//std::cerr << ".";
		noOfLines++;
		if (ssURIF_UOE_Data.empty())
			overflowCounter++;
	}
	cerr << endl;
	overflowCounter = 0;
	cerr << endl << "Tx test complete. Verifying result." << endl;
	axiWord txCompareOutput = axiWord(0, 0, 0);

	while (!ssUOE_IPTX_Data.empty()) {
		axiWord tempOutput = ssUOE_IPTX_Data.read();
		goldenOutputTx >> hex >> txCompareOutput.data >> txCompareOutput.last >> txCompareOutput.keep;
		if (txCompareOutput.data != tempOutput.data || txCompareOutput.last != tempOutput.last || txCompareOutput.keep != tempOutput.keep) {
			errCount++;
			cerr << "X";
		}
		else
			cerr << ".";
		string dataOutput = decodeApUint64(tempOutput.data);
		string keepOutput = decodeApUint8(tempOutput.keep);
		txOutput << dataOutput << " " << tempOutput.last << " " << keepOutput << endl;
	}
	/*if (errCount != 0) {
		cerr << endl << "Errors during Tx testing. Check the output file." << endl;
	}
	else
		cerr << endl << "Tx Tests passed succesfully." << endl;*/
	axiWord compareWord = axiWord(0, 0, 0);
	while (!ssURIF_UOE_Data.empty() && !goldenOutputRx.eof()) {
		//noOfLines++;
		//counter++;
		//cerr << counter << " ";
		axiWord tempOutput = ssURIF_UOE_Data.read();
		goldenOutputRx >> hex >> compareWord.data >> compareWord.last;
		if (compareWord.data != tempOutput.data || compareWord.last != tempOutput.last) {
			errCount++;
			//cerr << "X";
		}
		//else
			//cerr << ".";
		string dataOutput = decodeApUint64(tempOutput.data);
		rxOutput << dataOutput << " " << tempOutput.last << endl;
	}
	if (errCount != 0) {
		cerr << endl << "Errors during testing. Check the output file." << endl;
	}
	else
		cerr << endl << "All Tests passed succesfully." << endl;
	errCount = 0;

	//OBSOLETE_20200305 while(!outPortUnreachableFIFO.empty())
	//OBSOLETE_20200305 	outPortUnreachableFIFO.read();

	while(!ssURIF_UOE_Meta.empty())
		ssURIF_UOE_Meta.read();
	while(!ssURIF_UOE_Data.empty())
		ssURIF_UOE_Data.read();

	rxInput.close();
	rxOutput.close();
	txInput.close();
	txOutput.close();
	portStatus.close();
	return 0;
}
