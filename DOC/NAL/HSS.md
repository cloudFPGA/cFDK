Housekeeping Sub-System (HSS)
==============================

This document describes the design of the **Houskeeping Sub-System (HSS)** of the [NAL](./NAL.md) used by the *cloudFPGA* platform.

## Overview

The HSS module contains all processes that are necessary to keep the administrative data, as well as debug and status information up to date. 

The architecture is written in C++/HLS in [hss.cpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.cpp)/[hss.hpp](../../SRA/LIB/SHELL/LIB/hls/NAL/src/hss.hpp).