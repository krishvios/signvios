/*!
* \brief Contains definitions for video bit streams
*
* Sorenson Communications Inc. Confidential. --  Do not distribute
* Copyright 2019 Sorenson Communications, Inc. -- All rights reserved
*/
#pragma once

// When negotiating h.263 video capabilities, in the event that bppMaxKb is NOT
// expressed, these are the default values as defined in the H.263 document.
// SQCIF and QCIF are the same default as defined in H.263.
const int nMAX_SQCIF_BYTES_PER_CODED_PICTURE = 64 * 128; // nMaxFrameBytes
const int nMAX_QCIF_BYTES_PER_CODED_PICTURE = 64 * 128;
const int nMAX_SIF_BYTES_PER_CODED_PICTURE = 256 * 128;
const int nMAX_CIF_BYTES_PER_CODED_PICTURE = 256 * 128;

// H.264 Capability Parameter IDs
const int nPROFILE_ID = 41;
const int nLEVEL_ID = 42;
const int nCUSTOM_MAX_MBPS_ID = 3;
const int nCUSTOM_MAX_FS_ID = 4;
const int nCUSTOM_MAX_DPB_ID = 5;
const int nCUSTOM_MAX_BR_AND_CPB_ID = 6;
const int nMAX_STATIC_MBPS_ID = 7;
const int nMAX_RCMD_NAL_UNIT_SIZE_ID = 8;
const int nMAX_NAL_UNIT_SIZE_ID = 9;
const int nSAMPLE_ASPECT_RATIO_SUPPORTED_ID = 10;
const int nADDITIONAL_MODES_SUPPORTED_ID = 11;

// H.264 Defined Values given the level number
const int nMAX_MBPS_1   = 1485;
const int nMAX_MBPS_1_1 = 3000;
const int nMAX_MBPS_1_2 = 6000;
const int nMAX_MBPS_1_3 = 11880;
const int nMAX_MBPS_2   = 11880;
const int nMAX_MBPS_2_1 = 19800;
const int nMAX_MBPS_2_2 = 20250;
const int nMAX_MBPS_3   = 40500;
const int nMAX_MBPS_3_1 = 108000;
const int nMAX_MBPS_3_2 = 216000;
const int nMAX_MBPS_4   = 245760;
const int nMAX_MBPS_4_1 = 245760;
const int nMAX_MBPS_4_2 = 491520;
const int nMAX_MBPS_5   = 589824;
const int nMAX_MBPS_5_1 = 983040;
const int nMAX_MBPS_CMP1   = 1500 / 500;  // Used for comparison and negotiation of upgrades
const int nMAX_MBPS_CMP1_1 = 3000 / 500;
const int nMAX_MBPS_CMP1_2 = 6000 / 500;
const int nMAX_MBPS_CMP1_3 = 12000 / 500;
const int nMAX_MBPS_CMP2   = 12000 / 500;
const int nMAX_MBPS_CMP2_1 = 20000 / 500;
const int nMAX_MBPS_CMP2_2 = 20000 / 500;
const int nMAX_MBPS_CMP3   = 40500 / 500;

const int nMAX_FS_1     = 99;
const int nMAX_FS_1_1   = 396;
const int nMAX_FS_1_2   = 396;
const int nMAX_FS_1_3   = 396;
const int nMAX_FS_2     = 396;
const int nMAX_FS_2_1   = 792;
const int nMAX_FS_2_2   = 1620;
const int nMAX_FS_3     = 1620;
const int nMAX_FS_3_1   = 3600;
const int nMAX_FS_3_2   = 5120;
const int nMAX_FS_4     = 8192;
const int nMAX_FS_4_1   = 8192;
const int nMAX_FS_4_2   = 8192;
const int nMAX_FS_5     = 22080;
const int nMAX_FS_5_1   = 36864;
const int nMAX_FS_CMP1     = 1;    // Used for comparison and negotiation of upgrades
const int nMAX_FS_CMP1_1   = 2;
const int nMAX_FS_CMP1_2   = 2;
const int nMAX_FS_CMP1_3   = 2;
const int nMAX_FS_CMP2     = 2;
const int nMAX_FS_CMP2_1   = 3;
const int nMAX_FS_CMP2_2   = 6;
const int nMAX_FS_CMP3     = 6;

// The maximum decoded picture buffer size in units of 1024 bytes.
const int nMAX_DPB_1    = (int)(148.5 * 1024); // Convert from 1024 units to bytes
const int nMAX_DPB_1_1  = (int)(337.5 * 1024); // Convert from 1024 units to bytes
const int nMAX_DPB_1_2  = (int)(891 * 1024);   // Convert from 1024 units to bytes
const int nMAX_DPB_1_3  = (int)(891 * 1024);   // Convert from 1024 units to bytes
const int nMAX_DPB_2    = (int)(891 * 1024);   // Convert from 1024 units to bytes
const int nMAX_DPB_2_1  = (int)(1782 * 1024);  // Convert from 1024 units to bytes
const int nMAX_DPB_2_2  = (int)(3037.5 * 1024);// Convert from 1024 units to bytes
const int nMAX_DPB_3    = (int)(3037.5 * 1024);// Convert from 1024 units to bytes
// DJM: Note I added 3.1 for completeness but didn't check
// if the value needed changed from 3.0
const int nMAX_DPB_3_1  = (int)(3037.5 * 1024);// Convert from 1024 units to bytes

const int nMAX_BR_1     = 64;
const int nMAX_BR_1_1   = 192;
const int nMAX_BR_1_2   = 384;
const int nMAX_BR_1_3   = 768;
const int nMAX_BR_2     = 2000;
const int nMAX_BR_2_1   = 4000;
const int nMAX_BR_2_2   = 4000;
const int nMAX_BR_3     = 10000;

const int nMAX_CPB_1    = 175;
const int nMAX_CPB_1_1  = 500;
const int nMAX_CPB_1_2  = 1000;
const int nMAX_CPB_1_3  = 2000;
const int nMAX_CPB_2    = 2000;
const int nMAX_CPB_2_1  = 4000;
const int nMAX_CPB_2_2  = 4000;
const int nMAX_CPB_3    = 10000;

const int nSQCIF_MB     = 48; // nMacroblocks (16x16 pixel blocks per frame)
const int nQCIF_MB      = 99;
const int nSIF_MB       = 330;
const int nCIF_MB       = 396;
