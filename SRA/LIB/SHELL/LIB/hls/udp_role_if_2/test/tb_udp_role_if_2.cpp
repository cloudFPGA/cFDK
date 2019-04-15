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

#include "../src/udp_role_if_2.hpp"

using namespace std;


#define OK          true
#define KO          false
#define VALID       true
#define UNVALID     false
#define DEBUG_TRACE true


//------------------------------------------------------
//-- DUT INTERFACES AS GLOBAL VARIABLES
//------------------------------------------------------
//-- ROLE / Urif / Udp Interfaces
stream<UdpWord>         sROLE_Urif_Data     ("sROLE_Urif_Data");
stream<UdpWord>         sURIF_Role_Data     ("sURIF_Role_Data");
//-- UDMX / Urif / Open-Port Interfaces
stream<AxisAck>         sUDMX_Urif_OpnAck   ("sUDMX_OpnAck");
stream<UdpPort>         sURIF_Udmx_OpnReq   ("sURIF_Udmx_OpnReq");
//-- UDMX / Urif / Data & MetaData Interfaces
stream<UdpWord>         sUDMX_Urif_Data     ("sUDMX_Urif_Data");
stream<UdpMeta>         sUDMX_Urif_Meta     ("sUDMX_Urif_Meta");
stream<UdpWord>         sURIF_Udmx_Data     ("sURIF_Udmx_Data");
stream<UdpMeta>         sURIF_Udmx_Meta     ("sURIF_Udmx_Meta");
stream<UdpPLen>         sURIF_Udmx_PLen     ("sURIF_Udmx_PLen");

ap_uint<32>             sIpAddress = 0x0a0b0c0d;
stream<IPMeta>          sIPMeta_RX          ("sIPMeta_RX");
stream<IPMeta>          sIPMeta_TX          ("sIPMeta_TX");

//------------------------------------------------------
//-- TESTBENCH GLOBAL VARIABLES
//------------------------------------------------------
int         simCnt;


/*****************************************************************************
 * @brief Run a single iteration of the DUT model.
 * @ingroup udp_role_if
 * @return Nothing.
 ******************************************************************************/
void stepDut() {
    udp_role_if_2(
        0,
            sROLE_Urif_Data,    sURIF_Role_Data,
            sIPMeta_TX,         sIPMeta_RX,
            &sIpAddress,
            sUDMX_Urif_OpnAck,  sURIF_Udmx_OpnReq,
            sUDMX_Urif_Data,    sUDMX_Urif_Meta,
            sURIF_Udmx_Data,    sURIF_Udmx_Meta,    sURIF_Udmx_PLen);
    simCnt++;
    printf("[%4.4d] STEP DUT \n", simCnt);
}

/*****************************************************************************
 * @brief Initialize an input data stream from a file.
 * @ingroup udp_role_if
 *
 * @param[in] sDataStream, the input data stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputDataStream(stream<UdpWord> &sDataStream, const string dataStreamName, const string inpFileName) {
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    UdpWord     udpWord;

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
                // Print Data to console
                printf("[%4.4d] TB is filling input stream [%s] - Data write = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                        simCnt, dataStreamName.c_str(),
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
 * @ingroup udp_role_if
 *
 * @param[in] sMetaStream, the input meta stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] inpFileName, the name of the input file to read from.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool setInputMetaStream(stream<UdpMeta> &sMetaStream, const string dataStreamName, const string inpFileName) {
    string      strLine;
    ifstream    inpFileStream;
    string      datFile = "../../../../test/" + inpFileName;
    UdpWord     udpWord;
    UdpMeta     socketPair;

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
                //socketPair = {{0x0050, 0x0A0A0A0A}, {0x8000, 0x01010101}};
                socketPair = {{0x0050, 0x0A0B0C02}, {0x0050, 0x01010101}};

                //  Write to sMetaStream
                if (sMetaStream.full()) {
                    printf("### ERROR : Stream is full. Cannot write stream with data from file \"%s\".\n", inpFileName.c_str());
                    return(KO);
                }
                else {
                    sMetaStream.write(socketPair);
                    // Print Metadata to console
                    printf("[%4.4d] TB is filling input stream [%s] - Metadata = {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
                            simCnt, dataStreamName.c_str(),
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
 * @brief Read data from a stream.
 * @ingroup udp_role_if
 *
 * @param[in]  sDataStream,    the output data stream to read.
 * @param[in]  dataStreamName, the name of the data stream.
 * @param[out] udpWord,        a pointer to the storage location of the data
 *                              to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readDataStream(stream <UdpWord> &sDataStream, UdpWord *udpWord) {
    // Get the DUT/Data results
    sDataStream.read(*udpWord);
    return(VALID);
}

/*****************************************************************************
 * @brief Read an output metadata stream from the DUT.
 * @ingroup udp_role_if
 *
 * @param[in]  sMetaStream, the output metadata stream to read.
 * @param[in]  metaStreamName, the name of the meta stream.
 * @param[out] udpMeta, a pointer to the storage location of the metadata to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readMetaStream(stream <UdpMeta> &sMetaStream, const string metaStreamName,
                    UdpMeta *udpMeta) {
    // Get the DUT/Metadata results
    sMetaStream.read(*udpMeta);
    // Print DUT/Metadata to console
    printf("[%4.4d] TB is draining output stream [%s] - Metadata = {{SP=0x%4.4X,SA=0x%8.8X} {DP=0x%4.4X,DA=0x%8.8X}} \n",
            simCnt, metaStreamName.c_str(),
            udpMeta->src.port.to_int(), udpMeta->src.addr.to_int(), udpMeta->dst.port.to_int(), udpMeta->dst.addr.to_int());
    return(VALID);
}

/*****************************************************************************
 * @brief Read an output payload length stream from the DUT.
 * @ingroup udp_role_if
 *
 * @param[in]  sPLenStream, the output payload length stream to read.
 * @param[in]  plenStreamName, the name of the payload length stream.
 * @param[out] udpPLen, a pointer to the storage location of the payload length
 *              to read.
 * @return VALID if a data was read, otherwise UNVALID.
 ******************************************************************************/
bool readPLenStream(stream <UdpPLen> &sPLenStream, const string plenStreamName,
                    UdpPLen *udpPLen) {
    // Get the DUT/PayloadLength results
    sPLenStream.read(*udpPLen);
    // Print DUT/PayloadLength to console
    printf("[%4.4d] TB is draining output stream [%s] - Payload length = %d from DUT.\n",
            simCnt, plenStreamName.c_str(), udpPLen->to_int());
    return(VALID);
}

/*****************************************************************************
 * @brief Dump a data word to a file.
 * @ingroup udp_role_if
 *
 * @param[in] udpWord,      a pointer to the data word to dump.
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
 * @ingroup udp_role_if
 *
 * @param[in] udpMeta,      a pointer to the metadata structure to dump.
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
 * @ingroup udp_role_if
 *
 * @param[in] udpPLen,      a pointer to the payload length to dump.
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

/*****************************************************************************
 * @brief Fill an output file with data from an output stream.
 * @ingroup udp_role_if
 *
 * @param[in] sDataStream,    the output data stream to set.
 * @param[in] dataStreamName, the name of the data stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputDataStream(stream<UdpWord> &sDataStream,
                         const string    dataStreamName, const string   outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpWord     udpWord;
    bool        rc = OK;

    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAM AND DUMP DATA TO FILE
    while (!sDataStream.empty()) {
        if (readDataStream(sDataStream, &udpWord) == VALID) {
            // Print DUT/Data to console
            printf("[%4.4d] TB is draining output stream [%s] - Data read = {D=0x%16.16llX, K=0x%2.2X, L=%d} \n",
                    simCnt, dataStreamName.c_str(),
                    udpWord.tdata.to_long(), udpWord.tkeep.to_int(), udpWord.tlast.to_int());
            if (!dumpDataToFile(&udpWord, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}

/*****************************************************************************
 * @brief Fill an output file with metadata from an output stream.
 * @ingroup udp_role_if
 *
 * @param[in] sMetaStream,    the output metadata stream to set.
 * @param[in] metaStreamName, the name of the metadata stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputMetaStream(stream<UdpMeta> &sMetaStream,
                         const string    metaStreamName, const string outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpMeta     udpMeta;
    bool        rc = OK;

    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAM AND DUMP METADATA TO FILE
    while (!sMetaStream.empty()) {
        if (readMetaStream(sMetaStream, metaStreamName, &udpMeta) == VALID) {
            if (!dumpMetaToFile(&udpMeta, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}

/*****************************************************************************
 * @brief Fill an output file with payload length from an output stream.
 * @ingroup udp_role_if
 *
 * @param[in] sPLenStream,    the output payload length stream to set.
 * @param[in] plenStreamName, the name of the payload length stream.
 * @param[in] outFileName,    the name of the output file to write to.
 * @return OK if successful, otherwise KO.
 ******************************************************************************/
bool getOutputPLenStream(stream<UdpPLen> &sPLenStream,
                         const string    plenStreamName, const string outFileName)
{
    string      strLine;
    ofstream    outFileStream;
    string      datFile = "../../../../test/" + outFileName;
    UdpPLen     udpPLen;
    bool        rc = OK;

    //-- STEP-1 : OPEN FILE
    outFileStream.open(datFile.c_str());
    if ( !outFileStream ) {
        cout << "### ERROR : Could not open the output data file " << datFile << endl;
        return(KO);
    }

    //-- STEP-2 : EMPTY STREAM AND DUMP PAYLOAD LENGTH TO FILE
    while (!sPLenStream.empty()) {
        if (readPLenStream(sPLenStream, plenStreamName, &udpPLen) == VALID) {
            if (!dumpPLenToFile(&udpPLen, outFileStream)) {
                rc = KO;
                break;
            }
        }
    }

    //-- STEP-3: CLOSE FILE
    outFileStream.close();

    return(rc);
}

int main() {

    //------------------------------------------------------
    //-- TESTBENCH LOCAL VARIABLES
    //------------------------------------------------------
    int         nrErr = 0;
    UdpMeta     socketPair;

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
            sURIF_Udmx_OpnReq.read();
            printf("[%4.4d] URIF->UDMX_OpnReq : DUT is requesting to open a port.\n", simCnt);
            stepDut();
            sUDMX_Urif_OpnAck.write(true);
            printf("[%4.4d] UDMX->URIF_OpnAck : TB  acknowledges the port opening.\n", simCnt);
        }
    }

    //------------------------------------------------------
    //-- STEP-2 : CREATE TRAFFIC AS INPUT STREAMS
    //------------------------------------------------------
    if (nrErr == 0) {
        if (!setInputDataStream(sROLE_Urif_Data, "sROLE_Urif_Data", "ifsROLE_Urif_Data.dat")) {
            printf("### ERROR : Failed to set input data stream \"sROLE_DataStream\". \n");
            nrErr++;
        }

        if (!setInputDataStream(sUDMX_Urif_Data, "sUDMX_Urif_Data", "ifsUDMX_Urif_Data.dat")) {
            printf("### ERROR : Failed to set input data stream \"sUDMX_DataStream\". \n");
            nrErr++;
        }
        if (!setInputMetaStream(sUDMX_Urif_Meta, "sUDMX_Urif_Meta", "ifsUDMX_Urif_Data.dat")) {
            printf("### ERROR : Failed to set input meta stream \"sUDMX_MetaStream\". \n");
            nrErr++;
        }
        //there are 2 streams from the ROLE to UDMX
        sIPMeta_TX.write(IPMeta(0x0a0d0c02));
        sIPMeta_TX.write(IPMeta(0x0a0d0c02));
    }

    //------------------------------------------------------
    //-- STEP-3 : MAIN TRAFFIC LOOP
    //------------------------------------------------------
    bool sof = true;

    while (!nrErr) {

        if (simCnt < 42)
        {
            stepDut();

            if( !sIPMeta_RX.empty())
            {
              IPMeta tmp = sIPMeta_RX.read();
              printf("Role received IPMeta stream from 0x%8.8X.\n", (int) tmp.ipAddress);
            }

            // is done below
           // if( !sURIF_Udmx_Meta.empty() )
           // {
           // }

        } else {
            printf("## End of simulation at cycle=%3d. \n", simCnt);
            break;
        }

    }  // End: while()

    //-------------------------------------------------------
    //-- STEP-4 : DRAIN AND WRITE OUTPUT FILE STREAMS
    //-------------------------------------------------------
    //---- URIF-->ROLE ----
    if (!getOutputDataStream(sURIF_Role_Data, "sURIF_Role_Data", "ofsURIF_Role_Data.dat"))
        nrErr++;
    //---- URIF->UDMX ----
    if (!getOutputDataStream(sURIF_Udmx_Data, "sURIF_Udmx_Data", "ofsURIF_Udmx_Data.dat"))
            nrErr++;
    if (!getOutputMetaStream(sURIF_Udmx_Meta, "sURIF_Udmx_Meta", "ofsURIF_Udmx_Meta.dat"))
        nrErr++;
    if (!getOutputPLenStream(sURIF_Udmx_PLen, "sURIF_Udmx_PLen", "ofsURIF_Udmx_PLen.dat"))
        nrErr++;

    //------------------------------------------------------
    //-- STEP-5 : COMPARE INPUT AND OUTPUT FILE STREAMS
    //------------------------------------------------------
    int rc1 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Role_Data.dat \
                                            ../../../../test/ifsUDMX_Urif_Data.dat");
    if (rc1)
        printf("## Error : File \'ofsURIF_Role_Data.dat\' does not match \'ifsUDMX_Urif_Data.dat\'.\n");

    int rc2 = system("diff --brief -w -i -y ../../../../test/ofsURIF_Udmx_Data.dat \
                                            ../../../../test/ifsROLE_Urif_Data.dat");
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
