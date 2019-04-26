The MPIv0_x2Mp_x2Mc SRA
============================

This interface one stream based MPI interface, as well as two stream based memory ports.

The vhdl interface to the ROLE looks like follows:

```vhdl
      ------------------------------------------------------
      -- SHELL / Global Input Clock and Reset Interface
      ------------------------------------------------------
      piSHL_156_25Clk                     => sSHL_156_25Clk,
      piSHL_156_25Rst                     => sSHL_156_25Rst,
            
     
      ------------------------------------------------------
      -- SHELL / MPI Interface
      ------------------------------------------------------
      poROLE_MPE_MPIif_mpi_call_TDATA      => sROLE_MPE_MPIif_in_mpi_call_TDATA  ,
      poROLE_MPE_MPIif_mpi_call_TVALID     => sROLE_MPE_MPIif_in_mpi_call_TVALID ,
      piROLE_MPE_MPIif_mpi_call_TREADY     => sROLE_MPE_MPIif_in_mpi_call_TREADY ,
      --piMPE_ROLE_MPIif_mpi_call_TDATA      => sMPE_ROLE_MPIif_out_mpi_call_TDATA   ,
      --piMPE_ROLE_MPIif_mpi_call_TVALID     => sMPE_ROLE_MPIif_out_mpi_call_TVALID  ,
      --poMPE_ROLE_MPIif_mpi_call_TREADY     => sMPE_ROLE_MPIif_out_mpi_call_TREADY  ,
      --piMPE_ROLE_MPIif_count_TDATA         => sMPE_ROLE_MPIif_out_count_TDATA      ,
      --piMPE_ROLE_MPIif_count_TVALID        => sMPE_ROLE_MPIif_out_count_TVALID     ,
      --poMPE_ROLE_MPIif_count_TREADY        => sMPE_ROLE_MPIif_out_count_TREADY     ,
      poROLE_MPE_MPIif_count_TDATA         => sROLE_MPE_MPIif_in_count_TDATA     ,
      poROLE_MPE_MPIif_count_TVALID        => sROLE_MPE_MPIif_in_count_TVALID    ,
      piROLE_MPE_MPIif_count_TREADY        => sROLE_MPE_MPIif_in_count_TREADY    ,
      --piMPE_ROLE_MPIif_rank_TDATA          => sMPE_ROLE_MPIif_out_rank_TDATA       ,
      --piMPE_ROLE_MPIif_rank_TVALID         => sMPE_ROLE_MPIif_out_rank_TVALID      ,
      --poMPE_ROLE_MPIif_rank_TREADY         => sMPE_ROLE_MPIif_out_rank_TREADY      ,
      poROLE_MPE_MPIif_rank_TDATA          => sROLE_MPE_MPIif_in_rank_TDATA      ,
      poROLE_MPE_MPIif_rank_TVALID         => sROLE_MPE_MPIif_in_rank_TVALID     ,
      piROLE_MPE_MPIif_rank_TREADY         => sROLE_MPE_MPIif_in_rank_TREADY     ,
      piMPE_ROLE_MPI_data_TDATA            => sMPE_ROLE_MPI_data_TDATA            ,
      piMPE_ROLE_MPI_data_TVALID           => sMPE_ROLE_MPI_data_TVALID           ,
      poMPE_ROLE_MPI_data_TREADY           => sMPE_ROLE_MPI_data_TREADY           ,
      piMPE_ROLE_MPI_data_TKEEP            => sMPE_ROLE_MPI_data_TKEEP            ,
      piMPE_ROLE_MPI_data_TLAST            => sMPE_ROLE_MPI_data_TLAST            ,
      poROLE_MPE_MPI_data_TDATA            => sROLE_MPE_MPI_data_TDATA            ,
      poROLE_MPE_MPI_data_TVALID           => sROLE_MPE_MPI_data_TVALID           ,
      piROLE_MPE_MPI_data_TREADY           => sROLE_MPE_MPI_data_TREADY           ,
      poROLE_MPE_MPI_data_TKEEP            => sROLE_MPE_MPI_data_TKEEP            ,
      poROLE_MPE_MPI_data_TLAST            => sROLE_MPE_MPI_data_TLAST            ,


      ------------------------------------------------------
      -- SHELL / Role / Mem / Mp0 Interface
      ------------------------------------------------------
      -- Memory Port #0 / S2MM-AXIS ------------------   
      ---- Stream Read Command -----------------
      piSHL_Rol_Mem_Mp0_Axis_RdCmd_tready => sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready,
      poROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  => sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata,
      poROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid => sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid,
      ---- Stream Read Status ------------------
      piSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  => sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata,
      piSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid => sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid,
      poROL_Shl_Mem_Mp0_Axis_RdSts_tready => sROL_Shl_Mem_Mp0_Axis_RdSts_tready,
      ---- Stream Data Input Channel -----------
      piSHL_Rol_Mem_Mp0_Axis_Read_tdata   => sSHL_Rol_Mem_Mp0_Axis_Read_tdata,
      piSHL_Rol_Mem_Mp0_Axis_Read_tkeep   => sSHL_Rol_Mem_Mp0_Axis_Read_tkeep,
      piSHL_Rol_Mem_Mp0_Axis_Read_tlast   => sSHL_Rol_Mem_Mp0_Axis_Read_tlast,
      piSHL_Rol_Mem_Mp0_Axis_Read_tvalid  => sSHL_Rol_Mem_Mp0_Axis_Read_tvalid,
      poROL_Shl_Mem_Mp0_Axis_Read_tready  => sROL_Shl_Mem_Mp0_Axis_Read_tready,
      ---- Stream Write Command ----------------
      piSHL_Rol_Mem_Mp0_Axis_WrCmd_tready => sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready,
      poROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  => sROL_Shl_Mem_Mp0_Axis_WrCmd_tdata,
      poROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid => sROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid,
      ---- Stream Write Status -----------------
      piSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid => sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid,
      piSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  => sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata,
      poROL_Shl_Mem_Mp0_Axis_WrSts_tready => sROL_Shl_Mem_Mp0_Axis_WrSts_tready,
      ---- Stream Data Output Channel ----------
      piSHL_Rol_Mem_Mp0_Axis_Write_tready => sSHL_Rol_Mem_Mp0_Axis_Write_tready,
      poROL_Shl_Mem_Mp0_Axis_Write_tdata  => sROL_Shl_Mem_Mp0_Axis_Write_tdata,
      poROL_Shl_Mem_Mp0_Axis_Write_tkeep  => sROL_Shl_Mem_Mp0_Axis_Write_tkeep,
      poROL_Shl_Mem_Mp0_Axis_Write_tlast  => sROL_Shl_Mem_Mp0_Axis_Write_tlast,
      poROL_Shl_Mem_Mp0_Axis_Write_tvalid => sROL_Shl_Mem_Mp0_Axis_Write_tvalid,
      
      ------------------------------------------------------
      -- SHELL / Role / Mem / Mp1 Interface
      ------------------------------------------------------
      -- Memory Port #1 / S2MM-AXIS ------------------   
      ---- Stream Read Command -----------------
      piSHL_Rol_Mem_Mp1_Axis_RdCmd_tready => sSHL_Rol_Mem_Mp1_Axis_RdCmd_tready,
      poROL_Shl_Mem_Mp1_Axis_RdCmd_tdata  => sROL_Shl_Mem_Mp1_Axis_RdCmd_tdata,
      poROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid => sROL_Shl_Mem_Mp1_Axis_RdCmd_tvalid,
      ---- Stream Read Status ------------------
      piSHL_Rol_Mem_Mp1_Axis_RdSts_tdata  => sSHL_Rol_Mem_Mp1_Axis_RdSts_tdata,
      piSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid => sSHL_Rol_Mem_Mp1_Axis_RdSts_tvalid,
      poROL_Shl_Mem_Mp1_Axis_RdSts_tready => sROL_Shl_Mem_Mp1_Axis_RdSts_tready,
      ---- Stream Data Input Channel -----------
      piSHL_Rol_Mem_Mp1_Axis_Read_tdata   => sSHL_Rol_Mem_Mp1_Axis_Read_tdata,
      piSHL_Rol_Mem_Mp1_Axis_Read_tkeep   => sSHL_Rol_Mem_Mp1_Axis_Read_tkeep,
      piSHL_Rol_Mem_Mp1_Axis_Read_tlast   => sSHL_Rol_Mem_Mp1_Axis_Read_tlast,
      piSHL_Rol_Mem_Mp1_Axis_Read_tvalid  => sSHL_Rol_Mem_Mp1_Axis_Read_tvalid,
      poROL_Shl_Mem_Mp1_Axis_Read_tready  => sROL_Shl_Mem_Mp1_Axis_Read_tready,
      ---- Stream Write Command ----------------
      piSHL_Rol_Mem_Mp1_Axis_WrCmd_tready => sSHL_Rol_Mem_Mp1_Axis_WrCmd_tready,
      poROL_Shl_Mem_Mp1_Axis_WrCmd_tdata  => sROL_Shl_Mem_Mp1_Axis_WrCmd_tdata,
      poROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid => sROL_Shl_Mem_Mp1_Axis_WrCmd_tvalid,
      ---- Stream Write Status -----------------
      piSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid => sSHL_Rol_Mem_Mp1_Axis_WrSts_tvalid,
      piSHL_Rol_Mem_Mp1_Axis_WrSts_tdata  => sSHL_Rol_Mem_Mp1_Axis_WrSts_tdata,
      poROL_Shl_Mem_Mp1_Axis_WrSts_tready => sROL_Shl_Mem_Mp1_Axis_WrSts_tready,
      ---- Stream Data Output Channel ----------
      piSHL_Rol_Mem_Mp1_Axis_Write_tready => sSHL_Rol_Mem_Mp1_Axis_Write_tready,
      poROL_Shl_Mem_Mp1_Axis_Write_tdata  => sROL_Shl_Mem_Mp1_Axis_Write_tdata,
      poROL_Shl_Mem_Mp1_Axis_Write_tkeep  => sROL_Shl_Mem_Mp1_Axis_Write_tkeep,
      poROL_Shl_Mem_Mp1_Axis_Write_tlast  => sROL_Shl_Mem_Mp1_Axis_Write_tlast,
      poROL_Shl_Mem_Mp1_Axis_Write_tvalid => sROL_Shl_Mem_Mp1_Axis_Write_tvalid,
      
      ------------------------------------------------------
      -- SHELL / Role / Mmio / Flash Debug Interface
      ------------------------------------------------------
      -- MMIO / CTRL_2 Register ----------------
      --piSHL_Rol_Mmio_UdpEchoCtrl          => sSHL_Rol_Mmio_UdpEchoCtrl,
      --piSHL_Rol_Mmio_UdpPostPktEn         => sSHL_Rol_Mmio_UdpPostPktEn,
      --piSHL_Rol_Mmio_UdpCaptPktEn         => sSHL_Rol_Mmio_UdpCaptPktEn,
      --piSHL_Rol_Mmio_TcpEchoCtrl          => sSHL_Rol_Mmio_TcpEchoCtrl,
      --piSHL_Rol_Mmio_TcpPostPktEn         => sSHL_Rol_Mmio_TcpPostPktEn,
      --piSHL_Rol_Mmio_TcpCaptPktEn         => sSHL_Rol_Mmio_TcpCaptPktEn,
      
      ------------------------------------------------------
      -- ROLE EMIF Registers
      ------------------------------------------------------
      poROL_SHL_EMIF_2B_Reg               => sROL_SHL_EMIF_2B_Reg,
      piSHL_ROL_EMIF_2B_Reg               => sSHL_ROL_EMIF_2B_Reg,
      
      ------------------------------------------------
      -- SMC Interface
      ------------------------------------------------ 
      piSMC_ROLE_rank                     => sSMC_ROL_rank,
      piSMC_ROLE_size                     => sSMC_ROL_size,
      piSMC_softReset                     => sSMC_SoftReset,
   
      ------------------------------------------------------
      ---- TOP : Secondary Clock (Asynchronous)
      ------------------------------------------------------
      piTOP_250_00Clk                     => sTOP_250_00Clk,  -- Freerunning
```

