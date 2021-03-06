/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/usr/fsiscom/fsiscom_reasoncodes.H $               */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2011,2018                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#ifndef __FSISCOM_REASONCODES_H
#define __FSISCOM_REASONCODES_H

#include <hbotcompid.H>

namespace FSISCOM
{
    enum FSISCOMModuleId
    {
        MOD_FSISCOM_INVALID             = 0x00, // Zero is an invalid module id
        MOD_FSISCOM_PERFORM_OP          = 0x01, // fsiscom.C : fsiScomPerformOp
        MOD_FSISCOM_RT_SEND_SCOM_TO_FSP = 0x02,
        MOD_FSISCOM_RT_SEND_MULTI_SCOM_TO_FSP = 0x03,
    };

    enum FSISCOMReasonCode
    {
        RC_INVALID                   = FSISCOM_COMP_ID | 0x00,
        RC_WRITE_ERROR               = FSISCOM_COMP_ID | 0x01,
        RC_READ_ERROR                = FSISCOM_COMP_ID | 0x02,
        RC_INVALID_LENGTH            = FSISCOM_COMP_ID | 0x03,
        RC_INVALID_ADDRESS           = FSISCOM_COMP_ID | 0x04,
        RC_INVALID_OPTYPE            = FSISCOM_COMP_ID | 0x05,
        RC_RT_INTERFACE_ERR          = FSISCOM_COMP_ID | 0x06,
        RC_RT_NULL_FW_MSG_PTR        = FSISCOM_COMP_ID | 0x07,
    };
};

#endif
