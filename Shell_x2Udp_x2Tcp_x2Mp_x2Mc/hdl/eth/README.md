
#########|#########|#########|#########|#########|#########|#########|#########|
##
##                             THIS DIRECTORY
##
##                       ~/SHELL/<BOARD_NAME>/src/eth
##
#########|#########|#########|#########|#########|#########|#########|#########|

Overview:
--------
  This directory contains the toplevel design of the 10 Gigabit Ethernet (ETH) 
  subsystem that is instantiated by the shell of the current target platform.
  From an Open Systems Interconnection (OSI) model point of view, this ETH
  module implements the Physical layer (L1) and the Data Link layer (L2) of the
  OSI model. These two layers are also refered to as the Network Access layer in
  the TCP/IP model.     


Structure:
---------
  
  ETH - TenGigEth
    +- META - TenGigEth_SyncBlock 
    +- CORE - TenGigEth_Core
    |    +- IP   - TenGigEthSubSys_X1Y8_R
    |    +- FIFO - TenGigEth_MacFifo
    +- ALGC - TenGigEth_AxiLiteClk

 
