#include "../toe/toe.hpp"

using namespace hls;

/** @defgroup echo_server_application Echo Server Application
 *
 */
void echo_app(stream<ap_uint<16> >& listenPort, stream<bool>& listenPortStatus,
								// This is disabled for the time being, because it adds complexity/potential issues
								//stream<ap_uint<16> >& closePort,
								stream<appNotification>& notifications, stream<appReadRequest>& readRequest,
								stream<ap_uint<16> >& rxMetaData, stream<axiWord>& rxData,
								stream<ipTuple>& openConnection, stream<openStatus>& openConStatus,
								stream<ap_uint<16> >& closeConnection,
								stream<ap_uint<16> >& txMetaData, stream<axiWord>& txData,
								stream<ap_int<17> >& txStatus);
