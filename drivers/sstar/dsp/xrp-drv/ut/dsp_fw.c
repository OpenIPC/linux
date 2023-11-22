/*
 * dsp_fw.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#include <linux/elf.h>
#include <linux/firmware.h>
#include <linux/highmem.h>
#include <cam_os_wrapper.h>
#include <linux/io.h>
#include <linux/of.h>
#include "dsp_ut.h"
#include "dsp_fw.h"
#include <linux/file.h>
struct dsp_ut_fw_Buf_s
{
    void * data;
    size_t size;
    size_t allocated_size;
};

// TODO (yongliang.li) address map construct
phys_addr_t dsp_ut_fw_TranslateToCpu(dsp_ut_core *dspUtCtx, Elf32_Phdr *phdr)
{
    phys_addr_t res  = ((phys_addr_t)~0ul);
    int         size = 1;
    int         i    = 0;
    for (i = 0; i < size; i++)
    {
        if (phdr->p_paddr >= dspUtCtx->addrMap[i].u32DspPhy
            && phdr->p_paddr < (dspUtCtx->addrMap[i].u32DspPhy + dspUtCtx->addrMap[i].u32Len))
        {
            res = dspUtCtx->addrMap[i].cpuPhy + phdr->p_paddr - dspUtCtx->addrMap[i].u32DspPhy;
        }
    }
    return res;
}
unsigned int dsp_ut_fw_TranslateToDsp(dsp_ut_core *dspUtCtx, phys_addr_t phyAddr)
{
    unsigned int res  = ((unsigned int)~0ul);
    int          size = 1;
    int          i    = 0;
    for (i = 0; i < size; i++)
    {
        if (phyAddr > dspUtCtx->addrMap[i].cpuPhy
            && phyAddr < (dspUtCtx->addrMap[i].cpuPhy + dspUtCtx->addrMap[i].u32Len))
        {
            res = dspUtCtx->addrMap[i].u32DspPhy + dspUtCtx->addrMap[i].cpuPhy - phyAddr > dspUtCtx->addrMap[i].cpuPhy;
        }
    }
    return res;
}

int dsp_ut_fw_LoadSegmentToSysmem(dsp_ut_core *dspUtCtx, Elf32_Phdr *phdr)
{
    phys_addr_t  pa       = dsp_ut_fw_TranslateToCpu(dspUtCtx, phdr);
    struct page *page     = pfn_to_page(__phys_to_pfn(pa));
    size_t       pageOffs = pa & ~PAGE_MASK;
    size_t       offs;

    for (offs = 0; offs < phdr->p_memsz; ++page)
    {
        void * p = kmap(page);
        size_t sz;

        if (!p)
        {
            return -ENOMEM;
        }

        pageOffs &= ~PAGE_MASK;
        sz = PAGE_SIZE - pageOffs;

        if (offs < phdr->p_filesz)
        {
            size_t copySz = sz;

            if (phdr->p_filesz - offs < copySz)
            {
                copySz = phdr->p_filesz - offs;
            }

            copySz = ALIGN(copySz, 4);
            memcpy(p + pageOffs, (void *)dspUtCtx->firmware->data + phdr->p_offset + offs, copySz);
            pageOffs += copySz;
            offs += copySz;
            sz -= copySz;
        }

        if (offs < phdr->p_memsz && sz)
        {
            if (phdr->p_memsz - offs < sz)
            {
                sz = phdr->p_memsz - offs;
            }

            sz = ALIGN(sz, 4);
            memset(p + pageOffs, 0, sz);
            pageOffs += sz;
            offs += sz;
        }

        kunmap(page);
    }

    // TODO (yongliang.li) dev attr for dma
    // dma_sync_single_for_device(xvp->dev, pa, phdr->p_memsz, DMA_TO_DEVICE);
    return 0;
}

inline bool dsp_ut_fw_SectionBad(dsp_ut_core *dspUtCtx, const Elf32_Shdr *shdr)
{
    return shdr->sh_offset > dspUtCtx->firmware->size || shdr->sh_size > dspUtCtx->firmware->size - shdr->sh_offset;
}

int dsp_ut_fwFindSymbol(dsp_ut_core *dspUtCtx, const char *name, void **paddr, size_t *psize)
{
    const Elf32_Ehdr *ehdr     = (Elf32_Ehdr *)dspUtCtx->firmware->data;
    const void *      shdrData = dspUtCtx->firmware->data + ehdr->e_shoff;
    const Elf32_Shdr *shSymtab = NULL;
    const Elf32_Shdr *shStrtab = NULL;
    const void *      symData;
    const void *      strData;
    const Elf32_Sym * esym;
    void *            addr = NULL;
    unsigned          i;

    if (ehdr->e_shoff == 0)
    {
        printk("%s: no section header in the firmware image", __func__);
        return -ENOENT;
    }

    if (ehdr->e_shoff > dspUtCtx->firmware->size
        || ehdr->e_shnum * ehdr->e_shentsize > dspUtCtx->firmware->size - ehdr->e_shoff)
    {
        printk("%s: bad firmware SHDR information", __func__);
        return -EINVAL;
    }

    /* find symbols and string sections */

    for (i = 0; i < ehdr->e_shnum; ++i)
    {
        const Elf32_Shdr *shdr = shdrData + i * ehdr->e_shentsize;

        switch (shdr->sh_type)
        {
            case SHT_SYMTAB:
                shSymtab = shdr;
                break;

            case SHT_STRTAB:
                shStrtab = shdr;
                break;

            default:
                break;
        }
    }

    if (!shSymtab || !shStrtab)
    {
        printk("%s: no symtab or strtab in the firmware image", __func__);
        return -ENOENT;
    }

    if (dsp_ut_fw_SectionBad(dspUtCtx, shSymtab))
    {
        printk("%s: bad firmware SYMTAB section information", __func__);
        return -EINVAL;
    }

    if (dsp_ut_fw_SectionBad(dspUtCtx, shStrtab))
    {
        printk("%s: bad firmware STRTAB section information", __func__);
        return -EINVAL;
    }

    /* iterate through all symbols, searching for the name */
    symData = dspUtCtx->firmware->data + shSymtab->sh_offset;
    strData = dspUtCtx->firmware->data + shStrtab->sh_offset;

    for (i = 0; i < shSymtab->sh_size; i += shSymtab->sh_entsize)
    {
        esym = symData + i;

        if (!(ELF_ST_TYPE(esym->st_info) == STT_OBJECT && esym->st_name < shStrtab->sh_size
              && strncmp(strData + esym->st_name, name, shStrtab->sh_size - esym->st_name) == 0))
        {
            continue;
        }

        if (esym->st_shndx > 0 && esym->st_shndx < ehdr->e_shnum)
        {
            const Elf32_Shdr *shdr         = shdrData + esym->st_shndx * ehdr->e_shentsize;
            Elf32_Off         inSectionOff = esym->st_value - shdr->sh_addr;

            if (dsp_ut_fw_SectionBad(dspUtCtx, shdr))
            {
                printk("%s: bad firmware section #%d information", __func__, esym->st_shndx);
                return -EINVAL;
            }

            if (esym->st_value < shdr->sh_addr || inSectionOff > shdr->sh_size
                || esym->st_size > shdr->sh_size - inSectionOff)
            {
                printk("%s: bad symbol information", __func__);
                return -EINVAL;
            }

            addr = (void *)dspUtCtx->firmware->data + shdr->sh_offset + inSectionOff;
            printk(
                "%s: found symbol, st_shndx = %d, "
                "sh_offset = 0x%08x, sh_addr = 0x%08x, "
                "st_value = 0x%08x, address = %p",
                __func__, esym->st_shndx, shdr->sh_offset, shdr->sh_addr, esym->st_value, addr);
        }
        else
        {
            printk("%s: unsupported section index in found symbol: 0x%x", __func__, esym->st_shndx);
            return -EINVAL;
        }

        break;
    }

    if (!addr)
    {
        return -ENOENT;
    }

    *paddr = addr;
    *psize = esym->st_size;
    return 0;
}

int dsp_ut_fwFixupSymbol(dsp_ut_core *dspUtCtx, const char *name, phys_addr_t v)
{
    unsigned int v32 = 0;
    void *       addr;
    size_t       sz;
    int          rc;
    rc = dsp_ut_fwFindSymbol(dspUtCtx, name, &addr, &sz);

    if (rc < 0)
    {
        printk("%s: symbol \"%s\" is not found\n", __func__, name);
        return rc;
    }

    if (sz != sizeof(unsigned int))
    {
        printk("%s: symbol \"%s\" has wrong size: %zu\n", __func__, name, sz);
        return -EINVAL;
    }

    v32 = (unsigned int)v;
    memcpy(addr, &v32, sz);
    return 0;
}

int dsp_ut_fw_ImplReadFile(struct file *file, void **buf, loff_t *size, loff_t max_size)

{
    loff_t  iSize, pos;
    ssize_t bytes = 0;
    int     ret;
    ret = deny_write_access(file);

    if (ret)
    {
        return ret;
    }

    iSize = i_size_read(file_inode(file));

    if (max_size > 0 && iSize > max_size)
    {
        ret = -EFBIG;
        goto out;
    }

    if (iSize <= 0)
    {
        ret = -EINVAL;
        goto out;
    }

    *buf = vmalloc(iSize);

    if (!*buf)
    {
        ret = -ENOMEM;
        goto out;
    }

    pos = 0;

    while (pos < iSize)
    {
#ifdef __aarch64__
        bytes = kernel_read(file, (char *)(*buf) + pos, iSize - pos, &pos);
#else
        bytes = kernel_read(file, pos, (char *)(*buf) + pos, iSize - pos);
#endif

        if (bytes < 0)
        {
            ret = bytes;
            goto out_free;
        }

        if (bytes == 0)
        {
            break;
        }

#ifndef __aarch64__
        pos += bytes;
#endif
    }

    if (pos != iSize)
    {
        ret = -EIO;
        goto out_free;
    }

    *size = pos;
out_free:

    if (ret < 0)
    {
        vfree(*buf);
        *buf = NULL;
    }

out:
    allow_write_access(file);
    fput(file);
    return ret;
}

int dsp_ut_fw_ImplReadFirmware(struct dsp_ut_fw_Buf_s *buf, const char *path)
{
    loff_t       size;
    int          rc    = -ENOENT;
    size_t       msize = INT_MAX;
    struct file *file;

    if (!path || !*path)
    {
        return -EINVAL;
    }

    file = filp_open(path, O_RDONLY, 0);

    if (IS_ERR(file))
    {
        return PTR_ERR(file);
    }

    if (!S_ISREG(file_inode(file)->i_mode))
    {
        return -EINVAL;
    }

    buf->size = 0;
    rc        = dsp_ut_fw_ImplReadFile(file, &buf->data, &size, msize);

    if (!rc)
    {
        buf->size = size;
    }

    return rc;
}
int dsp_ut_fw_fetchFirmware(const struct firmware **firmware_p, const char *name)
{
    struct firmware *       fw    = NULL;
    struct dsp_ut_fw_Buf_s *fwbuf = NULL;
    int                     ret;

    if (!firmware_p)
    {
        return -EINVAL;
    }

    if (!name || name[0] == '\0')
    {
        ret = -EINVAL;
        goto out;
    }

    fw = kzalloc(sizeof(*fw), GFP_KERNEL);

    if (!fw)
    {
        ret = -ENOMEM;
        goto out;
    }

    fwbuf = kzalloc(sizeof(*fwbuf), GFP_ATOMIC);

    if (!fwbuf)
    {
        ret = -ENOMEM;
        goto out;
    }

    ret = dsp_ut_fw_ImplReadFirmware(fwbuf, name);

    if (!ret)
    {
        fw->priv = fwbuf;
        fw->size = fwbuf->size;
        fw->data = fwbuf->data;
    }

out:

    if (ret < 0)
    {
        kfree(fw);
        fw = NULL;
    }

    *firmware_p = fw;
    return ret;
}

int dsp_ut_fw_Request(dsp_ut_core *dspUtCtx, char *fwPath)
{
    int s32Ret = 0;

    /* Need to pin this module until return */
    __module_get(THIS_MODULE);
    s32Ret = dsp_ut_fw_fetchFirmware(&dspUtCtx->firmware, fwPath);
    module_put(THIS_MODULE);

    if (s32Ret < 0)
    {
        printk("fetch firmware fail\n");
    }
    else
    {
        printk("fetch firmware succ\n");
    }

    if (dspUtCtx->firmware == NULL)
    {
        s32Ret = -1;
        goto nullfw;
    }

nullfw:
    return s32Ret;
}
int dsp_ut_fw_Load(dsp_ut_core *dspUtCtx)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)dspUtCtx->firmware->data;
    int         i;

    if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG))
    {
        printk("bad firmware ELF magic\n");
        return -EINVAL;
    }

    if (ehdr->e_type != ET_EXEC)
    {
        printk("bad firmware ELF type\n");
        return -EINVAL;
    }

    if (ehdr->e_machine != 94 /*EM_XTENSA*/)
    {
        printk("bad firmware ELF machine\n");
        return -EINVAL;
    }

    if (ehdr->e_phoff >= dspUtCtx->firmware->size
        || ehdr->e_phoff + ehdr->e_phentsize * ehdr->e_phnum > dspUtCtx->firmware->size)
    {
        printk("bad firmware ELF PHDR information\n");
        return -EINVAL;
    }

    dsp_ut_fwFixupSymbol(dspUtCtx, "xrp_dsp_comm_base",
                         dsp_ut_fw_TranslateToDsp(dspUtCtx, CamOsMemMiuToPhys(dspUtCtx->comm_phys)));

    for (i = 0; i < ehdr->e_phnum; ++i)
    {
        Elf32_Phdr *phdr = (void *)dspUtCtx->firmware->data + ehdr->e_phoff + i * ehdr->e_phentsize;
        phys_addr_t pa;
        int         rc;

        rc = 0;

        /* Only load non-empty loadable segments, R/W/X */
        if (!(phdr->p_type == PT_LOAD && (phdr->p_flags & (PF_X | PF_R | PF_W)) && phdr->p_memsz > 0))
        {
            continue;
        }

        if (phdr->p_offset >= dspUtCtx->firmware->size || phdr->p_offset + phdr->p_filesz > dspUtCtx->firmware->size)
        {
            printk("bad firmware ELF program header entry %d\n", i);
            return -EINVAL;
        }

        pa = dsp_ut_fw_TranslateToCpu(dspUtCtx, phdr);

        if (pa == (phys_addr_t)OF_BAD_ADDR)
        {
            printk("device address 0x%08x could not be mapped to host physical address\n", (unsigned int)phdr->p_paddr);
            return -EINVAL;
        }

        printk("loading segment %d (device 0x%08x) to physical %pap\n", i, (unsigned int)phdr->p_paddr, &pa);

        if (pfn_valid(__phys_to_pfn(pa)))
        {
            rc = dsp_ut_fw_LoadSegmentToSysmem(dspUtCtx, phdr);
        }

        if (rc < 0)
        {
            return rc;
        }
    }

    return 0;
}

int dsp_ut_fw_Release(dsp_ut_core *dspUtCtx)
{
    if (!dspUtCtx->firmware)
    {
        printk("firmware == NULL\n");
        return 0;
    }
    vfree(dspUtCtx->firmware->data);
    kfree(dspUtCtx->firmware->priv);
    kfree(dspUtCtx->firmware);
    dspUtCtx->firmware = NULL;

    return 0;
}
