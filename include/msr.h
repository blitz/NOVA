/*
 * Model-Specific Registers (MSR)
 *
 * Copyright (C) 2005-2008, Udo Steinberg <udo@hypervisor.org>
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include "compiler.h"
#include "types.h"

class Msr
{
    public:

        // MSRs starting with IA32_ are architectural
        enum Register
        {
            IA32_PLATFORM_ID        = 0x17,
            IA32_APIC_BASE          = 0x1b,
            IA32_FEATURE_CONTROL    = 0x3a,
            IA32_BIOS_SIGN_ID       = 0x8b,
            IA32_SMM_MONITOR_CTL    = 0x9b,
            IA32_PERF_CTR           = 0xc1,
            IA32_MTRR_CAP           = 0xfe,
            IA32_SYSENTER_CS        = 0x174,
            IA32_SYSENTER_ESP       = 0x175,
            IA32_SYSENTER_EIP       = 0x176,
            IA32_PERF_EVT_SEL       = 0x186,
            IA32_THERM_INTERRUPT    = 0x19b,
            IA32_THERM_STATUS       = 0x19c,
            IA32_MISC_ENABLE        = 0x1a0,
            IA32_FLEX_BRVID_SEL     = 0x1a1,
            IA32_DEBUG_CTL          = 0x1d9,
            IA32_MTRR_PHYS_BASE     = 0x200,
            IA32_MTRR_PHYS_MASK     = 0x201,
            IA32_MTRR_DEF_TYPE      = 0x2ff,
            IA32_PERF_GLOBAL_CTRL   = 0x38f,

            IA32_VMX_BASIC          = 0x480,
            IA32_VMX_CTLS_PIN       = 0x481,
            IA32_VMX_CTLS_CPU_1     = 0x482,
            IA32_VMX_CTLS_EXIT      = 0x483,
            IA32_VMX_CTLS_ENTRY     = 0x484,
            IA32_VMX_CTLS_MISC      = 0x485,
            IA32_VMX_CR0_FIXED0     = 0x486,
            IA32_VMX_CR0_FIXED1     = 0x487,
            IA32_VMX_CR4_FIXED0     = 0x488,
            IA32_VMX_CR4_FIXED1     = 0x489,
            IA32_VMX_VMCS_ENUM      = 0x48a,
            IA32_VMX_CTLS_CPU_2     = 0x48b,

            IA32_EFER               = 0xc0000080,

            AMD_PERF_EVT_SEL        = 0xc0010000,
            AMD_PERF_CTR            = 0xc0010004,
            AMD_IPMR                = 0xc0010055
        };

        enum Feature_Control
        {
            FEATURE_LOCKED          = 1ul << 0,
            FEATURE_VMX_I_SMX       = 1ul << 1,
            FEATURE_VMX_O_SMX       = 1ul << 2
        };

        template <typename T>
        ALWAYS_INLINE
        static inline T read (Register msr)
        {
            T val;
            switch (sizeof (T)) {
                case 8:
                    asm volatile ("rdmsr" : "=A" (val) : "c" (msr));
                    return val;
                default:
                    asm volatile ("rdmsr" : "=a" (val) : "c" (msr) : "edx");
                    return val;
            }
        }

        template <typename T>
        ALWAYS_INLINE
        static inline void write (Register msr, T val)
        {
            switch (sizeof (T)) {
                case 8:
                    asm volatile ("wrmsr" : : "A" (val), "c" (msr));
                    break;
                default:
                    asm volatile ("wrmsr" : : "a" (val), "d" (0), "c" (msr));
                    break;
            }
        }
};
