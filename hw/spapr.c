/*
 * QEMU PowerPC pSeries Logical Partition (aka sPAPR) hardware System Emulator
 *
 * Copyright (c) 2004-2007 Fabrice Bellard
 * Copyright (c) 2007 Jocelyn Mayer
 * Copyright (c) 2010 David Gibson, IBM Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include "sysemu.h"
#include "hw.h"
#include "elf.h"

#include "hw/boards.h"
#include "hw/ppc.h"
#include "hw/loader.h"

#include "hw/spapr.h"
#include "hw/spapr_vio.h"

#include <libfdt.h>

#define KERNEL_LOAD_ADDR        0x00000000
#define INITRD_LOAD_ADDR        0x02800000
#define FDT_MAX_SIZE            0x10000
#define RTAS_MAX_SIZE           0x10000

#define TIMEBASE_FREQ           512000000ULL

#define MAX_CPUS                32

sPAPREnvironment *spapr;

static void *spapr_create_fdt(int *fdt_size, ram_addr_t ramsize,
                              const char *cpu_model, CPUState *envs[],
                              sPAPREnvironment *spapr,
                              target_phys_addr_t initrd_base,
                              target_phys_addr_t initrd_size,
                              const char *kernel_cmdline,
                              target_phys_addr_t rtas_addr,
                              target_phys_addr_t rtas_size,
                              long hash_shift)
{
    void *fdt;
    uint64_t mem_reg_property[] = { 0, cpu_to_be64(ramsize) };
    uint32_t start_prop = cpu_to_be32(initrd_base);
    uint32_t end_prop = cpu_to_be32(initrd_base + initrd_size);
    uint32_t pft_size_prop[] = {0, cpu_to_be32(hash_shift)};
    char hypertas_prop[] = "hcall-pft\0hcall-term";
    int i;
    char *modelname;
    int ret;

#define _FDT(exp) \
    do { \
        int ret = (exp);                                           \
        if (ret < 0) {                                             \
            fprintf(stderr, "qemu: error creating device tree: %s: %s\n", \
                    #exp, fdt_strerror(ret));                      \
            exit(1);                                               \
        }                                                          \
    } while (0)

    fdt = qemu_mallocz(FDT_MAX_SIZE);
    _FDT((fdt_create(fdt, FDT_MAX_SIZE)));

    _FDT((fdt_finish_reservemap(fdt)));

    /* Root node */
    _FDT((fdt_begin_node(fdt, "")));
    _FDT((fdt_property_string(fdt, "device_type", "chrp")));
    _FDT((fdt_property_string(fdt, "model", "qemu,emulated-pSeries-LPAR")));

    _FDT((fdt_property_cell(fdt, "#address-cells", 0x2)));
    _FDT((fdt_property_cell(fdt, "#size-cells", 0x2)));

    /* /chosen */
    _FDT((fdt_begin_node(fdt, "chosen")));

    _FDT((fdt_property_string(fdt, "bootargs", kernel_cmdline)));
    _FDT((fdt_property(fdt, "linux,initrd-start",
                       &start_prop, sizeof(start_prop))));
    _FDT((fdt_property(fdt, "linux,initrd-end",
                       &end_prop, sizeof(end_prop))));

    _FDT((fdt_end_node(fdt)));

    /* memory node */
    _FDT((fdt_begin_node(fdt, "memory@0")));

    _FDT((fdt_property_string(fdt, "device_type", "memory")));
    _FDT((fdt_property(fdt, "reg",
                       mem_reg_property, sizeof(mem_reg_property))));

    _FDT((fdt_end_node(fdt)));

    /* cpus */
    _FDT((fdt_begin_node(fdt, "cpus")));

    _FDT((fdt_property_cell(fdt, "#address-cells", 0x1)));
    _FDT((fdt_property_cell(fdt, "#size-cells", 0x0)));

    modelname = qemu_strdup(cpu_model);

    for (i = 0; i < strlen(modelname); i++) {
        modelname[i] = toupper(modelname[i]);
    }

    for (i = 0; i < smp_cpus; i++) {
        CPUState *env = envs[i];
        char *nodename;
        uint32_t segs[] = {cpu_to_be32(28), cpu_to_be32(40),
                           0xffffffff, 0xffffffff};

        if (asprintf(&nodename, "%s@%x", modelname, i) < 0) {
            fprintf(stderr, "Allocation failure\n");
            exit(1);
        }

        _FDT((fdt_begin_node(fdt, nodename)));

        free(nodename);

        _FDT((fdt_property_cell(fdt, "reg", i)));
        _FDT((fdt_property_string(fdt, "device_type", "cpu")));

        _FDT((fdt_property_cell(fdt, "cpu-version", env->spr[SPR_PVR])));
        _FDT((fdt_property_cell(fdt, "dcache-block-size",
                                env->dcache_line_size)));
        _FDT((fdt_property_cell(fdt, "icache-block-size",
                                env->icache_line_size)));
        _FDT((fdt_property_cell(fdt, "timebase-frequency", TIMEBASE_FREQ)));
        /* Hardcode CPU frequency for now.  It's kind of arbitrary on
         * full emu, for kvm we should copy it from the host */
        _FDT((fdt_property_cell(fdt, "clock-frequency", 1000000000)));
        _FDT((fdt_property_cell(fdt, "ibm,slb-size", env->slb_nr)));
        _FDT((fdt_property(fdt, "ibm,pft-size",
                           pft_size_prop, sizeof(pft_size_prop))));
        _FDT((fdt_property_string(fdt, "status", "okay")));
        _FDT((fdt_property(fdt, "64-bit", NULL, 0)));

        if (envs[i]->mmu_model & POWERPC_MMU_1TSEG) {
            _FDT((fdt_property(fdt, "ibm,processor-segment-sizes",
                               segs, sizeof(segs))));
        }

        _FDT((fdt_end_node(fdt)));
    }

    qemu_free(modelname);

    _FDT((fdt_end_node(fdt)));

    /* RTAS */
    _FDT((fdt_begin_node(fdt, "rtas")));

    _FDT((fdt_property(fdt, "ibm,hypertas-functions", hypertas_prop,
                       sizeof(hypertas_prop))));

    _FDT((fdt_end_node(fdt)));

    /* vdevice */
    _FDT((fdt_begin_node(fdt, "vdevice")));

    _FDT((fdt_property_string(fdt, "device_type", "vdevice")));
    _FDT((fdt_property_string(fdt, "compatible", "IBM,vdevice")));
    _FDT((fdt_property_cell(fdt, "#address-cells", 0x1)));
    _FDT((fdt_property_cell(fdt, "#size-cells", 0x0)));

    _FDT((fdt_end_node(fdt)));

    _FDT((fdt_end_node(fdt))); /* close root node */
    _FDT((fdt_finish(fdt)));

    /* re-expand to allow for further tweaks */
    _FDT((fdt_open_into(fdt, fdt, FDT_MAX_SIZE)));

    ret = spapr_populate_vdevice(spapr->vio_bus, fdt);
    if (ret < 0) {
        fprintf(stderr, "couldn't setup vio devices in fdt\n");
        exit(1);
    }

    /* RTAS */
    ret = spapr_rtas_device_tree_setup(fdt, rtas_addr, rtas_size);
    if (ret < 0) {
        fprintf(stderr, "Couldn't set up RTAS device tree properties\n");
    }

    _FDT((fdt_pack(fdt)));

    *fdt_size = fdt_totalsize(fdt);

    return fdt;
}

static uint64_t translate_kernel_address(void *opaque, uint64_t addr)
{
    return (addr & 0x0fffffff) + KERNEL_LOAD_ADDR;
}

static void emulate_spapr_hypercall(CPUState *env)
{
    env->gpr[3] = spapr_hypercall(env, env->gpr[3], &env->gpr[4]);
}

/* pSeries LPAR / sPAPR hardware init */
static void ppc_spapr_init(ram_addr_t ram_size,
                           const char *boot_device,
                           const char *kernel_filename,
                           const char *kernel_cmdline,
                           const char *initrd_filename,
                           const char *cpu_model)
{
    CPUState *envs[MAX_CPUS];
    void *fdt, *htab;
    int i;
    ram_addr_t ram_offset;
    target_phys_addr_t fdt_addr, rtas_addr;
    uint32_t kernel_base, initrd_base;
    long kernel_size, initrd_size, htab_size, rtas_size;
    long pteg_shift = 17;
    int fdt_size;
    char *filename;

    spapr = qemu_malloc(sizeof(*spapr));
    cpu_ppc_hypercall = emulate_spapr_hypercall;

    /* We place the device tree just below either the top of RAM, or
     * 2GB, so that it can be processed with 32-bit code if
     * necessary */
    fdt_addr = MIN(ram_size, 0x80000000) - FDT_MAX_SIZE;
    /* RTAS goes just below that */
    rtas_addr = fdt_addr - RTAS_MAX_SIZE;

    /* init CPUs */
    if (cpu_model == NULL) {
        cpu_model = "POWER7";
    }
    for (i = 0; i < smp_cpus; i++) {
        CPUState *env = cpu_init(cpu_model);

        if (!env) {
            fprintf(stderr, "Unable to find PowerPC CPU definition\n");
            exit(1);
        }
        /* Set time-base frequency to 512 MHz */
        cpu_ppc_tb_init(env, TIMEBASE_FREQ);
        qemu_register_reset((QEMUResetHandler *)&cpu_reset, env);

        env->hreset_vector = 0x60;
        env->hreset_excp_prefix = 0;
        env->gpr[3] = i;

        envs[i] = env;
    }

    /* allocate RAM */
    ram_offset = qemu_ram_alloc(NULL, "ppc_spapr.ram", ram_size);
    cpu_register_physical_memory(0, ram_size, ram_offset);

    /* allocate hash page table.  For now we always make this 16mb,
     * later we should probably make it scale to the size of guest
     * RAM */
    htab_size = 1ULL << (pteg_shift + 7);
    htab = qemu_mallocz(htab_size);

    for (i = 0; i < smp_cpus; i++) {
        envs[i]->external_htab = htab;
        envs[i]->htab_base = -1;
        envs[i]->htab_mask = htab_size - 1;
    }

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, "spapr-rtas.bin");
    rtas_size = load_image_targphys(filename, rtas_addr, ram_size - rtas_addr);
    if (rtas_size < 0) {
        hw_error("qemu: could not load LPAR rtas '%s'\n", filename);
        exit(1);
    }
    qemu_free(filename);

    spapr->vio_bus = spapr_vio_bus_init();

    for (i = 0; i < MAX_SERIAL_PORTS; i++) {
        if (serial_hds[i]) {
            spapr_vty_create(spapr->vio_bus, i, serial_hds[i]);
        }
    }

    if (kernel_filename) {
        uint64_t lowaddr = 0;

        kernel_base = KERNEL_LOAD_ADDR;

        kernel_size = load_elf(kernel_filename, translate_kernel_address, NULL,
                               NULL, &lowaddr, NULL, 1, ELF_MACHINE, 0);
        if (kernel_size < 0) {
            kernel_size = load_image_targphys(kernel_filename, kernel_base,
                                              ram_size - kernel_base);
        }
        if (kernel_size < 0) {
            fprintf(stderr, "qemu: could not load kernel '%s'\n",
                    kernel_filename);
            exit(1);
        }

        /* load initrd */
        if (initrd_filename) {
            initrd_base = INITRD_LOAD_ADDR;
            initrd_size = load_image_targphys(initrd_filename, initrd_base,
                                              ram_size - initrd_base);
            if (initrd_size < 0) {
                fprintf(stderr, "qemu: could not load initial ram disk '%s'\n",
                        initrd_filename);
                exit(1);
            }
        } else {
            initrd_base = 0;
            initrd_size = 0;
        }
    } else {
        fprintf(stderr, "pSeries machine needs -kernel for now");
        exit(1);
    }

    /* Prepare the device tree */
    fdt = spapr_create_fdt(&fdt_size, ram_size, cpu_model, envs, spapr,
                           initrd_base, initrd_size, kernel_cmdline,
                           rtas_addr, rtas_size, pteg_shift + 7);
    assert(fdt != NULL);

    cpu_physical_memory_write(fdt_addr, fdt, fdt_size);

    qemu_free(fdt);

    envs[0]->gpr[3] = fdt_addr;
    envs[0]->gpr[5] = 0;
    envs[0]->hreset_vector = kernel_base;
}

static QEMUMachine spapr_machine = {
    .name = "pseries",
    .desc = "pSeries Logical Partition (PAPR compliant)",
    .init = ppc_spapr_init,
    .max_cpus = MAX_CPUS,
    .no_vga = 1,
    .no_parallel = 1,
};

static void spapr_machine_init(void)
{
    qemu_register_machine(&spapr_machine);
}

machine_init(spapr_machine_init);
