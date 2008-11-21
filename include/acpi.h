/*
 * Advanced Configuration and Power Interface (ACPI)
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

#include "acpi_gas.h"
#include "compiler.h"
#include "types.h"

class Acpi
{
    friend class Acpi_table_fadt;
    friend class Acpi_table_rsdt;
    friend class Acpi_rsdp;

    private:
        static Paddr    fadt_addr;
        static Paddr    madt_addr;
        static Paddr    rsdt_addr;
        static Paddr    xsdt_addr;

        static Acpi_gas pm1a_sts;
        static Acpi_gas pm1b_sts;
        static Acpi_gas pm1a_ena;
        static Acpi_gas pm1b_ena;
        static Acpi_gas pm1a_cnt;
        static Acpi_gas pm1b_cnt;
        static Acpi_gas pm2_cnt;
        static Acpi_gas pm_tmr;
        static Acpi_gas reset_reg;

        static uint32   feature;
        static uint32   smi_cmd;
        static uint8    enable_val;
        static uint8    reset_val;

        static unsigned hw_read (Acpi_gas *gas);

        static void hw_write (Acpi_gas *gas, unsigned val);

        INIT
        static void setup_sci();

        INIT
        static void enable();

    public:
        enum Mode
        {
            PIC,
            APIC
        };

        enum Register
        {
            PM1_STS,
            PM1_ENA,
            PM1_CNT,
            PM2_CNT,
            PM_TMR,
            RESET
        };

        enum PM1_Status
        {
            PM1_STS_TMR         = 1u << 0,      // 0x1
            PM1_STS_BM          = 1u << 4,      // 0x10
            PM1_STS_GBL         = 1u << 5,      // 0x20
            PM1_STS_PWRBTN      = 1u << 8,      // 0x100
            PM1_STS_SLPBTN      = 1u << 9,      // 0x200
            PM1_STS_RTC         = 1u << 10,     // 0x400
            PM1_STS_PCIE_WAKE   = 1u << 14,     // 0x4000
            PM1_STS_WAKE        = 1u << 15      // 0x8000
        };

        enum PM1_Enable
        {
            PM1_ENA_TMR         = 1u << 0,      // 0x1
            PM1_ENA_GBL         = 1u << 5,      // 0x20
            PM1_ENA_PWRBTN      = 1u << 8,      // 0x100
            PM1_ENA_SLPBTN      = 1u << 9,      // 0x200
            PM1_ENA_RTC         = 1u << 10,     // 0x400
            PM1_ENA_PCIE_WAKE   = 1u << 14      // 0x4000
        };

        enum PM1_Control
        {
            PM1_CNT_SCI_EN      = 1u << 0,      // 0x1
            PM1_CNT_BM_RLD      = 1u << 1,      // 0x2
            PM1_CNT_GBL_RLS     = 1u << 2,      // 0x4
            PM1_CNT_SLP_TYP     = 7u << 10,     // 0x400
            PM1_CNT_SLP_EN      = 1u << 13      // 0x2000
        };

        static Mode mode;
        static unsigned irq;
        static unsigned gsi;

        static unsigned const timer_frequency = 3579545;

        INIT
        static void setup();

        static void delay (unsigned ms);

        static void reset();

        ALWAYS_INLINE
        static inline unsigned read (Register reg);

        ALWAYS_INLINE
        static inline void write (Register reg, unsigned val);
};
