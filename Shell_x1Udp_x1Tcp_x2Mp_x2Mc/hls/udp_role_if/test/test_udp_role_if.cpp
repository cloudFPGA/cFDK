/*****************************************************************************
 * @file       : test_udp_role_if.cpp
 * @brief      : Testbench for the UDP role interface.
 *
 * System:     : cloudFPGA
 * Component   : Shell, Network Transport Session (NTS)
 * Language    : Vivado HLS
 *
 * Copyright 2009-2015 - Xilinx Inc.  - All rights reserved.
 * Copyright 2015-2018 - IBM Research - All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>
#include <hls_stream.h>

#include "../src/udp_role_if.hpp"

using namespace std;

#define OK			true
#define KO			false
#define	VALID		true
#define UNVALID		false
#define DEBUG_TRACE true


//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------
//-- ROLE / Urif / Udp Interfaces
stream<UdpWord>			sROLE_Urif_Data		("sROLE_Urif_Data");
stream<UdpWord> 		sURIF_Role_Data		("sURIF_Role_Data");
//-- UDMX / Urif / Open-Port Interfaces
stream<AxisAck> 		sUDMX_Urif_OpnAck	("sUDMX_OpnAck");
stream<UdpWord>     	sUDMX_Urif_Data		("sUDMX_Urif_Data");
stream<UdpMeta>     	sUDMX_Urif_Meta		("sUDMX_Urif_Meta");
//-- UDMX / Urif / Data & MetaData Interfaces
stream<UdpPort>	    	sURIF_Udmx_OpnReq	("sURIF_Udmx_OpnReq");
stream<UdpWord>  		sURIF_Udmx_Data		("sURIF_Udmx_Data");
stream<UdpMeta> 		sURIF_Udmx_Meta		("sURIF_Udmx_Meta");
stream<UdpPLen> 		sURIF_Udmx_PLen		("sURIF_Udmx_PLen");

//------------------------------------------------------
//-- TBENCH INPUT STREAM INTERFACES AS GLOBAL BARIABLES
//------------------------------------------------------
stream<UdpWord>       	sROLE_DataStream	("sROLE_DataStream");

stream<UdpWord>       	sUDMX_DataStream	("sUDMX_DataStream");
stream<UdpMeta>       	sUDMX_MetaStream	("sUDMX_MetaStream");

#pragma HLS STREAM variable=sROLE_DataStream depth=1024 dim=1

#pragma HLS STREAM variable=sUDMX_DataStream depth=1024 dim=1
#pragma HLS STREAM variable=sUDMX_MetaStream depth=256  dim=1

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int 	 	simCnt;


/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup udp_mux
 * @return Nothing.
 ******************************************************************************/
void stepDut() {

	udp_role_if(
		sROLE_Urif_Data, 	sURIF_Role_Data,
		sUDMX_Urif_OpnAck,  sURIF_Udmx_OpnReq,
		sUDMX_Urif_Data, 	sUDMX_Urif_Meta,
		sURIF_Udmx_Data, 	sURIF_Udmx_Meta,	sURIF_Udmx_PLen);
	simCnt++;
	printf("[ RUN DUT ] : cycle=%3d \n", simCnt);
}

/*****************************************************************************
 * @brief Initialize an input data stream from a file.
 * @ingroup udp_mux
 *
 * @param[in] sDataStream, the input data stream to set.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputDataStream(stream<UdpWord> &sDataStream, const string inpFileName) {
	string 		strLine;
	ifstream 	inpFileStream;
	string 		datFile = "../../../../test/" + inpFileName;
	UdpWord		udpWord;

	//-- STEP-1 : OPEN FILE
	inpFileStream.open(datFile.c_str());
	if ( !inpFileStream ) {
		cout << "### ERROR : Could not open the input data file " << datFile << endl;
		return(KO);
	}

	//-- STEP-2 : SET DATA STREAM
	while (inpFileStream) {

		if (!inpFileStream.eof()) {

			getline(inpFileStream, strLine);
			if (strLine.empty()) continue;
			sscanf(strLine.c_str(), "%llx %x %d", &udpWord.tdata, &udpWord.tkeep, &udpWord.tlast);

			// Write to sDataStream
			if (sDataStream.full()) {
				printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
				return(KO);
			} else {
				sDataStream.write(udpWord);
				printf("%s->TB : TB is writing data to input stream {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
						inpFileName.c_str(),
						udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
			}
		}
	}

	//-- STEP-3: CLOSE FILE
	inpFileStream.close();

	return(OK);
}

/*****************************************************************************
 * @brief Initialize an input meta stream from a file.
 * @ingroup udp_mux
 *
 * @param[in] sMetaStream, the input meta stream to set.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputMetaStream(stream<UdpMeta> &sMetaStream, const string inpFileName) {
	string 		strLine;
	ifstream 	inpFileStream;
	string 		datFile = "../../../../test/" + inpFileName;
	UdpWord		udpWord;
	UdpMeta 	socketPair;

	//-- STEP-1 : OPEN FILE
	inpFileStream.open(datFile.c_str());
	if ( !inpFileStream ) {
		cout << "### ERROR : Could not open the input data file " << datFile << endl;
		return(KO);
	}

	//-- STEP-2 : SET DATA STREAM
	while (inpFileStream) {

		if (!inpFileStream.eof()) {

			getline(inpFileStream, strLine);
			if (strLine.empty()) continue;
			sscanf(strLine.c_str(), "%llx %x %d", &udpWord.tdata, &udpWord.tkeep, &udpWord.tlast);

			// Check if the LAST bit is set
			if (udpWord.tlast) {

				// Create an connection association {{SrcPort, SrcAdd}, {DstPort, DstAdd}}
				socketPair = {{0x0050, 0x0A0A0A0A}, {0x8000, 0x01010101}};

				//  Write to sMetaStream
				if (sMetaStream.full()) {
					printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
					return(KO);
				}
				else {
					sMetaStream.write(socketPair);
					printf("%s->TB : TB is writing metadata to input stream {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
							inpFileName.c_str(),
							socketPair.src.port.to_int(), socketPair.src.addr.to_int(), socketPair.dst.port.to_int(), socketPair.dst.addr.to_int());
				}
			}
		}
	}

	//-- STEP-3: CLOSE FILE
	inpFileStream.close();

	return(OK);
}

/*****************************************************************************
 * @brief Read an output data stream from the DUT.
 * @ingroup udp_mux
 *
 * @param[in]  sDataStream, the output data stream to read.
 * @param[in]  dataStreamName, the name of the data stream.
 * @param[out] udpWord, a pointer to the storage location of the data to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readDataStream(stream <UdpWord> &sDataStream, const string dataStreamName, UdpWord *udpWord) {
	if (!sDataStream.empty()) {
		// Get the DUT/Data results
		sDataStream.read(*udpWord);
		// Print DUT/Data to console
		printf("DUT[%s]->TB : TB is reading data from DUT {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
				dataStreamName.c_str(),
				udpWord->tdata.to_long(), udpWord->tkeep.to_int(), udpWord->tlast.to_int());
		return(VALID);
	}
	else
		return(UNVALID);
}

/*****************************************************************************
 * @brief Read an output metadata stream from the DUT.
 * @ingroup udp_mux
 *
 * @param[in]  sMetaStream, the output metadata stream to read.
 * @param[in]  metaStreamName, the name of the meta stream.
 * @param[out] udpMeta, a pointer to the storage location of the metadata to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readMetaStream(stream <UdpMeta> &sMetaStream, const string metaStreamName, UdpMeta *udpMeta) {
	if (!sMetaStream.empty()) {
		// Get the DUT/Metadata results
		sMetaStream.read(*udpMeta);
		// Print DUT/Metadata to console
		printf("DUT[%s]->TB : TB is reading metadata from DUT {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
				metaStreamName.c_str(),
				udpMeta->src.port.to_int(), udpMeta->src.addr.to_int(), udpMeta->dst.port.to_int(), udpMeta->dst.addr.to_int());
		return(VALID);
	}
	else
		return(UNVALID);
}

/*****************************************************************************
 * @brief Read an output payload length stream from the DUT.
 * @ingroup udp_mux
 *
 * @param[in]  sPLenStream, the output payload length stream to read.
 * @param[in]  plenStreamName, the name of the payload length stream.
 * @param[out] udpPLen, a pointer to the storage location of the payload length
 *  			to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readPLenStream(stream <UdpPLen> &sPLenStream, const string plenStreamName, UdpPLen *udpPLen) {
	if (!sPLenStream.empty()) {
		// Get the DUT/PayloadLength results
		sPLenStream.read(*udpPLen);
		// Print DUT/PayloadLength to console
		printf("DUT[%s]->TB : TB is reading payload length = %d from DUT.\n",
				plenStreamName.c_str(), udpPLen->to_int());
		return(VALID);
	}
	else
		return(UNVALID);
}

/*****************************************************************************
 * @brief Dump a data word to a file.
 * @ingroup udp_mux
 *
 * @param[in] udpWord,		a pointer to the data word to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpDataToFile(UdpWord *udpWord, ofstream &outFileStream) {
	if (!outFileStream.is_open()) {
		printf("### ERROR : Output file stream is not open. \n");
		return(KO);
	}
	outFileStream << hex << noshowbase << setfill('0') << setw(16) << udpWord->tdata.to_uint64();
	outFileStream << " ";
	outFileStream << hex << noshowbase << setfill('0') << setw(2)  << udpWord->tkeep.to_int();
	outFileStream << " ";
	outFileStream << setw(1) << udpWord->tlast.to_int() << "\n";
	return(OK);
}

/*****************************************************************************
 * @brief Dump a metadata information to a file.
 * @ingroup udp_mux
 *
 * @param[in] udpMeta,		a pointer to the metadata structure to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpMetaToFile(UdpMeta *udpMeta, ofstream &outFileStream) {
	if (!outFileStream.is_open()) {
		printf("### ERROR : Output file stream is not open. \n");
		return(KO);
	}
	outFileStream << hex << noshowbase << setfill('0') << setw(4) << udpMeta->src.port.to_int();
	outFileStream << " ";
	outFileStream << hex << noshowbase << setfill('0') << setw(8) << udpMeta->src.addr.to_int();
	outFileStream << " ";
	outFileStream << hex << noshowbase << setfill('0') << setw(4) << udpMeta->dst.port.to_int();
	outFileStream << " ";
	outFileStream << hex << noshowbase << setfill('0') << setw(8) << udpMeta->dst.addr.to_int();
	outFileStream << "\n";
	return(OK);
}

/*****************************************************************************
 * @brief Dump a payload length information to a file.
 * @ingroup udp_mux
 *
 * @param[in] udpPLen,		a pointer to the payload length to dump.
 * @param[in] outFileStream,the output file stream to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool dumpPLenToFile(UdpPLen *udpPLen, ofstream &outFileStream) {
	if (!outFileStream.is_open()) {
		printf("### ERROR : Output file stream is not open. \n");
		return(KO);
	}
	outFileStream << hex << noshowbase << setfill('0') << setw(4) << udpPLen->to_int();
	outFileStream << "\n";
	return(OK);
}


int main() {

	//------------------------------------------------------
	//-- TESTBENCH LOCAL VARIABLES
	//------------------------------------------------------
	int 		nrErr 	= 0;
	UdpMeta 	socketPair;

	UdpWord  	roleWord, udmxWord, urifWord;
	UdpMeta		roleMeta, udmxMeta, urifMeta;
	UdpPLen		rolePLen, udmxPLen, urifPLen;

	//------------------------------------------------------
	//-- OPEN TEST BENCH FILES
	//------------------------------------------------------
	ofstream ofsURIF_Role_Data;
	ofstream ofsURIF_Udmx_Data, ofsURIF_Udmx_Meta, ofsURIF_Udmx_PLen;

	ofsURIF_Role_Data.open("../../../../test/ofsURIF_Role_Data.dat");
	if ( !ofsURIF_Role_Data ) {
		cout << "### ERROR : Could not open the output test file \'ofsURIF_Role_Data.dat\'." << endl;
		return(-1);
	}

	ofsURIF_Udmx_Data.open("../../../../test/ofsURIF_Udmx_Data.dat");
	if ( !ofsURIF_Udmx_Data ) {
		cout << "### ERROR : Could not open the output test file \'ofsURIF_Udmx_Data.dat\'." << endl;
		return(-1);
	}
	ofsURIF_Udmx_Meta.open("../../../../test/ofsURIF_Udmx_Meta.dat");
	if ( !ofsURIF_Udmx_Meta ) {
		cout << "### ERROR : Could not open the output test file \'ofsURIF_Udmx_Meta.dat\'." << endl;
		return(-1);
	}
	ofsURIF_Udmx_PLen.open("../../../../test/ofsURIF_Udmx_PLen.dat");
	if ( !ofsURIF_Udmx_PLen ) {
		cout << "### ERROR : Could not open the output test file \'ofsURIF_Udmx_PLen.dat\'." << endl;
		return(-1);
	}


	printf("#####################################################\n");
	printf("## TESTBENCH STARTS HERE                           ##\n");
	printf("#####################################################\n");

	simCnt = 0;
	nrErr  = 0;

	//------------------------------------------------------
	//-- STEP-1 : OPEN PORT REQUEST
	//------------------------------------------------------
	for (int i=0; i<15; ++i) {
		stepDut();
		if ( !sURIF_Udmx_OpnReq.empty() ) {
			printf("URIF->UDMX_OpnReq : DUT is requesting to open a port.\n");
			sURIF_Udmx_OpnReq.read();
			sUDMX_Urif_OpnAck.write(true);
			printf("UDMX->URIF_OpnAck : TB  acknowledges the port opening.\n");
			break;
		}
	}

	//------------------------------------------------------
	//-- STEP-2 : CREATE TRAFFIC AS INPUT STREAMS
	//------------------------------------------------------
	if (nrErr == 0) {
		if (!setInputDataStream(sROLE_DataStream, "ifsROLE_Urif_Data.dat")) {
			printf("### ERROR : Failed to set input data stream \"sROLE_DataStream\". \n");
			nrErr++;
		}

		if (!setInputDataStream(sUDMX_DataStream, "ifsUDMX_Urif_Data.dat")) {
			printf("### ERROR : Failed to set input data stream \"sUDMX_DataStream\". \n");
			nrErr++;
		}
		if (!setInputMetaStream(sUDMX_MetaStream, "ifsUDMX_Urif_Data.dat")) {
			printf("### ERROR : Failed to set input meta stream \"sUDMX_MetaStream\". \n");
			nrErr++;
		}
	}

	//------------------------------------------------------
	//-- STEP-3 : MAIN TRAFFIC LOOP
	//------------------------------------------------------
	bool sof = true;

	while (!nrErr) {

		//-------------------------------------------------
		//-- STEP-3.0 : FEED INPUTS
		//-------------------------------------------------
		// ROLE->URIF
		if (!sROLE_DataStream.empty() && !sROLE_Urif_Data.full())
			sROLE_Urif_Data.write(sROLE_DataStream.read());

		// UDMX->>URIF
		if (sof) {
			if (!sUDMX_MetaStream.empty() && !sUDMX_Urif_Meta.full())
				sUDMX_Urif_Meta.write(sUDMX_MetaStream.read());
			sof = false;
		}
		if (!sUDMX_DataStream.empty() && !sUDMX_Urif_Data.full()) {
			urifWord = sUDMX_DataStream.read();
			sUDMX_Urif_Data.write(urifWord);
			if (urifWord.tlast)
				sof = true;
		}

		//-------------------------------------------------
		//-- STEP-3.1 : RUN DUT
		//-------------------------------------------------
		if (simCnt < 35)
			stepDut();
		else {
			printf("## End of simulation at cycle=%3d. \n", simCnt);
			break;
		}

		//-------------------------------------------------------
		//-- STEP-3.2 : DRAIN DUT AND WRITE OUTPUT FILE STREAMS
		//-------------------------------------------------------
		//---- URIF-->ROLE ----
		if (readDataStream(sURIF_Role_Data, "sURIF_Role_Data", &roleWord) == VALID) {
			if (!dumpDataToFile(&roleWord, ofsURIF_Role_Data)) {
				nrErr++;
				break;
			}
		}

		//---- URIF->UDMX ----
		if (readDataStream(sURIF_Udmx_Data, "sURIF_Udmx_Data", &udmxWord) == VALID) {
			if (!dumpDataToFile(&udmxWord, ofsURIF_Udmx_Data)) {
				nrErr++;
				break;
			}
		}
		if (readMetaStream(sURIF_Udmx_Meta, "sURIF_Udmx_Meta", &udmxMeta) == VALID) {
			if (!dumpMetaToFile(&udmxMeta, ofsURIF_Udmx_Meta)) {
				nrErr++;
				break;
			}
			if (!readPLenStream(sURIF_Udmx_PLen, "sURIF_Udmx_PLen", &udmxPLen)) {
				nrErr++;
				break;
			}
			else {
				if (!dumpPLenToFile(&udmxPLen, ofsURIF_Udmx_PLen)) {
					nrErr++;
					break;
				}
			}
		}

	}  // End: while()

	//------------------------------------------------------
	//-- STEP-4 : CLOSE TEST BENCH OUTPUT FILES
	//------------------------------------------------------
	ofsURIF_Role_Data.close();

	ofsURIF_Udmx_Data.close();
	ofsURIF_Udmx_Meta.close();
	ofsURIF_Udmx_PLen.close();

	//------------------------------------------------------
	//-- STEP-5 : COMPARE INPUT AND OUTPUT FILE STREAMS
	//------------------------------------------------------
	int rc1 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Role_Data.dat ../../../../test/ifsUDMX_Urif_Data.dat");
	if (rc1)
		printf("## Error : File \'ofsURIF_Role_Data.dat\' does not match \'ifsUDMX_Urif_Data.dat\'.\n");

	int rc2 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Udmx_Data.dat ../../../../test/ifsROLE_Urif_Data.dat");
	if (rc2)
		printf("## Error : File \'ofsURIF_Udmx_Data.dat\' does not match \'ifsROLE_Urif_Data.dat\'.\n");
	nrErr += rc1 + rc2;

	printf("#####################################################\n");
	if (nrErr)
		printf("## ERROR - TESTBENCH FAILED (RC=%d) !!!             ##\n", nrErr);
	else
		printf("## SUCCESSFULL END OF TESTBENCH (RC=0)             ##\n");

	printf("#####################################################\n");

	return(nrErr);
}
