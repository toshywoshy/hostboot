/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/usr/isteps/istep06/host_start_occ_xstop_handler.C $       */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015,2018                        */
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

#include <stdint.h>
#include <trace/interface.H>
#include <errl/errlentry.H>
#include <errl/errlmanager.H>
#include <isteps/hwpisteperror.H>
#include <initservice/isteps_trace.H>
#include <kernel/vmmmgr.H>
#include <sys/mm.h>
#include <pm/pm_common.H>
#include <targeting/common/commontargeting.H>
#include <isteps/pm/occCheckstop.H>
#include <util/misc.H>
#include <util/utilsemipersist.H>
#ifdef CONFIG_BMC_IPMI
#include <ipmi/ipmisensor.H>
#endif
#include <sys/misc.h>
#include <xscom/xscomif.H>
#include <initservice/initserviceif.H>
#include <kernel/machchk.H>

namespace ISTEP_06
{
void* host_start_occ_xstop_handler( void *io_pArgs )
{
    ISTEP_ERROR::IStepError l_stepError;

    errlHndl_t l_err = nullptr;

    TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
               "host_start_occ_xstop_handler entry" );

    TARGETING::Target* masterproc = NULL;
    TARGETING::targetService().masterProcChipTargetHandle(masterproc);

    do
    {
        // If we have nothing external (FSP or OCC) to handle checkstops we are
        //  better off just crashing and having a chance to pull the HB
        //  traces off the system live

        TARGETING::Target * l_sys = nullptr;
        TARGETING::targetService().getTopLevelTarget( l_sys );
        assert(l_sys != nullptr);

#ifndef CONFIG_HANG_ON_MFG_SRC_TERM
        //When in MNFG_FLAG_SRC_TERM mode enable reboots to allow HB
        //to analyze now that the OCC is up and alive
        auto l_mnfgFlags =
          l_sys->getAttr<TARGETING::ATTR_MNFG_FLAGS>();

        // Check to see if SRC_TERM bit is set in MNFG flags
        if ((l_mnfgFlags & TARGETING::MNFG_FLAG_SRC_TERM) &&
            !(l_mnfgFlags & TARGETING::MNFG_FLAG_IMMEDIATE_HALT))
        {
            l_err = nullptr;

            //If HB_VOLATILE MFG_TERM_REBOOT_ENABLE flag is set at this point
            //Create errorlog to terminate the boot.
            Util::semiPersistData_t l_semiData;
            Util::readSemiPersistData(l_semiData);
            if (l_semiData.mfg_term_reboot == Util::MFG_TERM_REBOOT_ENABLE)
            {
                /*@
                 * @errortype
                 * @moduleid    ISTEP::MOD_OCC_XSTOP_HANDLER
                 * @reasoncode  ISTEP::RC_PREVENT_REBOOT_IN_MFG_TERM_MODE
                 * @devdesc     System rebooted without xstop in MFG TERM mode.
                 * @custdesc    A problem occurred during the IPL of the system.
                 */
                l_err = new ERRORLOG::ErrlEntry
                  (ERRORLOG::ERRL_SEV_CRITICAL_SYS_TERM,
                   ISTEP::MOD_OCC_XSTOP_HANDLER,
                   ISTEP::RC_PREVENT_REBOOT_IN_MFG_TERM_MODE,
                   0,
                   0,
                   true /*HB SW error*/ );
                l_stepError.addErrorDetails(l_err);
                break;
            }

            //Put a mark in HB VOLATILE
            Util::semiPersistData_t l_newSemiData;  //inits to 0s
            Util::readSemiPersistData(l_newSemiData);
            l_newSemiData.mfg_term_reboot = Util::MFG_TERM_REBOOT_ENABLE;
            Util::writeSemiPersistData(l_newSemiData);

            //Enable reboots so FIRDATA will be analyzed on XSTOP
            SENSOR::RebootControlSensor l_rbotCtl;
            l_err = l_rbotCtl.setRebootControl(
                SENSOR::RebootControlSensor::autoRebootSetting::ENABLE_REBOOTS);

            if(l_err)
            {
                TRACFCOMP(ISTEPS_TRACE::g_trac_isteps_trace,
                          "Failed to enable BMC auto reboots....");
                l_stepError.addErrorDetails(l_err);
                break;
            }
        }
#endif

#ifdef CONFIG_IPLTIME_CHECKSTOP_ANALYSIS
        // TODO - RTC 190807 - Skip OCC Start in MPIPL path in OPAL
        // Revisit this to enable the OCC loading in 6.11 once PM related
        // issue is resolved
        if (TARGETING::is_sapphire_load())
        {
            if(l_sys->getAttr<TARGETING::ATTR_IS_MPIPL_HB>() == true)
            {
                break;
            }
        }

        void* l_homerVirtAddrBase = reinterpret_cast<void*>
          (VmmManager::INITIAL_MEM_SIZE);
        uint64_t l_homerPhysAddrBase = mm_virt_to_phys(l_homerVirtAddrBase);
        uint64_t l_commonPhysAddr = l_homerPhysAddrBase + VMM_HOMER_REGION_SIZE;

        TRACFCOMP(ISTEPS_TRACE::g_trac_isteps_trace, "host_start_occ_xstop_handler:"
                  " l_homerPhysAddrBase=0x%x, l_commonPhysAddr=0x%x",
                  l_homerPhysAddrBase, l_commonPhysAddr);

        // Load the OCC directly into SRAM and start it in a special mode
        //  that only handles checkstops
        l_err  = HBPM::loadPMComplex(masterproc,
                                     l_homerPhysAddrBase,
                                     l_commonPhysAddr,
                                     HBPM::PM_LOAD,
                                     true);
        if(l_err)
        {
            TRACFCOMP(ISTEPS_TRACE::g_trac_isteps_trace,
                                                        "loadPMComplex failed");
            l_stepError.addErrorDetails(l_err);
            break;
        }

        l_err = HBOCC::startOCCFromSRAM(masterproc);
        if(l_err)
        {
            TRACFCOMP(ISTEPS_TRACE::g_trac_isteps_trace,
                                                     "startOCCFromSRAM failed");
            l_stepError.addErrorDetails(l_err);
            break;
        }
#endif

    }while(0);

    if(l_err)
    {
        ERRORLOG::errlCommit(l_err, HWPF_COMP_ID);
    }

    // Now that the checkstop handler is running (or we don't have one),
    //  setup the machine check code to trigger a checkstop for UE
    TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
               "Enabling machine check handler to generate checkstops" );

    uint64_t l_xstopXscom = XSCOM::generate_mmio_addr( masterproc,
                       Kernel::MachineCheck::MCHK_XSTOP_FIR_SCOM_ADDR );

    set_mchk_data( l_xstopXscom,
                   Kernel::MachineCheck::MCHK_XSTOP_FIR_VALUE );

    TRACFCOMP( ISTEPS_TRACE::g_trac_isteps_trace,
               "host_start_occ_xstop_handler exit" );

    return l_stepError.getErrorHandle();
}

};
