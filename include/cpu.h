/*
 * Central Processing Unit (CPU)
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
#include "vectors.h"

#define REGISTER_SETTER(reg)                                    \
    ALWAYS_INLINE                                               \
    static inline void set_ ## reg (mword v)                    \
    {                                                           \
        asm volatile ("mov %0, %%" #reg :: "r" (v));            \
    }

#define REGISTER_GETTER(reg)                                    \
    ALWAYS_INLINE                                               \
    static inline mword get_ ## reg ()                          \
    {                                                           \
        mword v;                                                \
        asm volatile ("mov %%" #reg ", %0" : "=r" (v));         \
        return v;                                               \
    }

class Cpu
{
    private:
        static char const * const vendor_string[];

        ALWAYS_INLINE
        static inline void check_features();

        ALWAYS_INLINE
        static inline void setup_thermal();

        ALWAYS_INLINE
        static inline void setup_sysenter();

    public:
        enum Vendor
        {
            UNKNOWN,
            INTEL,
            AMD
        };

        enum Feature
        {
            FEAT_FPU          =  0,
            FEAT_VME          =  1,
            FEAT_DE           =  2,
            FEAT_PSE          =  3,
            FEAT_TSC          =  4,
            FEAT_MSR          =  5,
            FEAT_PAE          =  6,
            FEAT_MCE          =  7,
            FEAT_CX8          =  8,
            FEAT_APIC         =  9,
            FEAT_SEP          = 11,
            FEAT_MTRR         = 12,
            FEAT_PGE          = 13,
            FEAT_MCA          = 14,
            FEAT_CMOV         = 15,
            FEAT_PAT          = 16,
            FEAT_PSE36        = 17,
            FEAT_PSN          = 18,
            FEAT_CLFSH        = 19,
            FEAT_DS           = 21,
            FEAT_ACPI         = 22,
            FEAT_MMX          = 23,
            FEAT_FXSR         = 24,
            FEAT_SSE          = 25,
            FEAT_SSE2         = 26,
            FEAT_SS           = 27,
            FEAT_HTT          = 28,
            FEAT_TM           = 29,
            FEAT_PBE          = 31,
            FEAT_SSE3         = 32,
            FEAT_MONITOR      = 35,
            FEAT_DSCPL        = 36,
            FEAT_VMX          = 37,
            FEAT_SMX          = 38,
            FEAT_EST          = 39,
            FEAT_TM2          = 40,
            FEAT_SSSE3        = 41,
            FEAT_CNXTID       = 42,
            FEAT_CX16         = 45,
            FEAT_XTPR         = 46,
            FEAT_PDCM         = 47,
            FEAT_DCA          = 50,
            FEAT_SSE4_1       = 51,
            FEAT_SSE4_2       = 52,
            FEAT_APIC_2       = 53,
            FEAT_POPCNT       = 55,
            FEAT_XSAVE        = 58,
            FEAT_OSXSAVE      = 59,
            FEAT_SYSCALL      = 75,
            FEAT_XD           = 84,
            FEAT_MMX_EXT      = 86,
            FEAT_FFXSR        = 89,
            FEAT_RDTSCP       = 91,
            FEAT_EM64T        = 93,
            FEAT_3DNOW_EXT    = 94,
            FEAT_3DNOW        = 95,
            FEAT_LAHF_SAHF    = 96,
            FEAT_CMP_LEGACY   = 97,
            FEAT_SVM          = 98,
            FEAT_EXT_APIC     = 99,
            FEAT_LOCK_MOV_CR0 = 100
        };

        enum
        {
            CR0_PE      = 1ul << 0,             // 0x1
            CR0_MP      = 1ul << 1,             // 0x2
            CR0_EM      = 1ul << 2,             // 0x4
            CR0_TS      = 1ul << 3,             // 0x8
            CR0_ET      = 1ul << 4,             // 0x10
            CR0_NE      = 1ul << 5,             // 0x20
            CR0_WP      = 1ul << 16,            // 0x10000
            CR0_AM      = 1ul << 18,            // 0x40000
            CR0_NW      = 1ul << 29,            // 0x20000000
            CR0_CD      = 1ul << 30,            // 0x40000000
            CR0_PG      = 1ul << 31             // 0x80000000
        };

        enum
        {
            CR4_DE      = 1ul << 3,             // 0x8
            CR4_PSE     = 1ul << 4,             // 0x10
            CR4_PAE     = 1ul << 5,             // 0x20
            CR4_PGE     = 1ul << 7,             // 0x80
            CR4_VMXE    = 1ul << 13,            // 0x2000
            CR4_SMXE    = 1ul << 14             // 0x4000
        };

        enum
        {
            DR7_EMPTY   = 0x400,
        };

        enum
        {
            EFL_CF      = 1ul << 0,             // 0x1
            EFL_PF      = 1ul << 2,             // 0x4
            EFL_AF      = 1ul << 4,             // 0x10
            EFL_ZF      = 1ul << 6,             // 0x40
            EFL_SF      = 1ul << 7,             // 0x80
            EFL_TF      = 1ul << 8,             // 0x100
            EFL_IF      = 1ul << 9,             // 0x200
            EFL_DF      = 1ul << 10,            // 0x400
            EFL_OF      = 1ul << 11,            // 0x800
            EFL_IOPL    = 3ul << 12,            // 0x3000
            EFL_NT      = 1ul << 14,            // 0x4000
            EFL_RF      = 1ul << 16,            // 0x10000
            EFL_VM      = 1ul << 17,            // 0x20000
            EFL_AC      = 1ul << 18,            // 0x40000
            EFL_VIF     = 1ul << 19,            // 0x80000
            EFL_VIP     = 1ul << 20,            // 0x100000
            EFL_ID      = 1ul << 21             // 0x200000
        };

        static unsigned boot_lock           asm ("boot_lock");
        static unsigned booted;
        static unsigned secure;

        static unsigned id                  CPULOCAL_HOT WEAK;
        static unsigned package             CPULOCAL;
        static unsigned core                CPULOCAL;
        static unsigned thread              CPULOCAL;

        static Vendor   vendor              CPULOCAL;
        static unsigned platform            CPULOCAL;
        static unsigned family              CPULOCAL;
        static unsigned model               CPULOCAL;
        static unsigned stepping            CPULOCAL;
        static unsigned brand               CPULOCAL;
        static unsigned patch               CPULOCAL;

        static uint32 name[12]              CPULOCAL;
        static uint32 features[4]           CPULOCAL;
        static bool bsp                     CPULOCAL;

        static unsigned spinner             CPULOCAL;
        static unsigned vtlb_miss_count     CPULOCAL;
        static unsigned vtlb_refl_count     CPULOCAL;
        static unsigned vtlb_flush_count    CPULOCAL;
        static unsigned apic_timer_count    CPULOCAL;
        static unsigned irqs[NUM_GSI]       CPULOCAL;
        static unsigned exit[64]            CPULOCAL;

        static void init();
        static void wakeup_ap();
        static void delay (unsigned us);

        ALWAYS_INLINE
        static inline bool feature (Feature feat)
        {
            return features[feat / 32] & 1u << feat % 32;
        }

        ALWAYS_INLINE
        static inline void set_feature (unsigned feat)
        {
            features[feat / 32] |= 1u << feat % 32;
        }

        ALWAYS_INLINE
        static inline void pause()
        {
            asm volatile ("pause");
        }

        ALWAYS_INLINE
        static inline void halt()
        {
            asm volatile ("sti; hlt; cli");
        }

        ALWAYS_INLINE
        static inline uint64 time()
        {
            uint64 val;
            asm volatile ("rdtsc" : "=A" (val));
            return val;
        }

        ALWAYS_INLINE NORETURN
        static inline void shutdown()
        {
            for (;;)
                asm volatile ("cli; hlt");
        }

        ALWAYS_INLINE
        static inline void cpuid (unsigned leaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf));
        }

        ALWAYS_INLINE
        static inline void cpuid (unsigned leaf, unsigned subleaf, uint32 &eax, uint32 &ebx, uint32 &ecx, uint32 &edx)
        {
            asm volatile ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (leaf), "c" (subleaf));
        }

        REGISTER_GETTER(cr0)
        REGISTER_SETTER(cr0)
        REGISTER_GETTER(cr4)
        REGISTER_SETTER(cr4)

        REGISTER_GETTER(dr0)
        REGISTER_GETTER(dr1)
        REGISTER_GETTER(dr2)
        REGISTER_GETTER(dr3)
        REGISTER_GETTER(dr6)
        REGISTER_GETTER(dr7)

        REGISTER_SETTER(dr0)
        REGISTER_SETTER(dr1)
        REGISTER_SETTER(dr2)
        REGISTER_SETTER(dr3)
        REGISTER_SETTER(dr6)
        REGISTER_SETTER(dr7)


        ALWAYS_INLINE
        static inline void store_fence()
        {
            asm volatile ("sfence" : : : "memory");
        }

        ALWAYS_INLINE
        static inline void flush_cache()
        {
            asm volatile ("wbinvd" : : : "memory");
        }
};
