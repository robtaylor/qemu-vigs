/*
 * Block driver for the VMDK format
 *
 * Copyright (c) 2004 Fabrice Bellard
 * Copyright (c) 2005 Filip Navara
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
 */

#include "qemu-common.h"
#include "block_int.h"
#include "module.h"

#define VMDK3_MAGIC (('C' << 24) | ('O' << 16) | ('W' << 8) | 'D')
#define VMDK4_MAGIC (('K' << 24) | ('D' << 16) | ('M' << 8) | 'V')

typedef struct {
    uint32_t version;
    uint32_t flags;
    uint32_t disk_sectors;
    uint32_t granularity;
    uint32_t l1dir_offset;
    uint32_t l1dir_size;
    uint32_t file_sectors;
    uint32_t cylinders;
    uint32_t heads;
    uint32_t sectors_per_track;
} VMDK3Header;

typedef struct {
    uint32_t version;
    uint32_t flags;
    int64_t capacity;
    int64_t granularity;
    int64_t desc_offset;
    int64_t desc_size;
    int32_t num_gtes_per_gte;
    int64_t rgd_offset;
    int64_t gd_offset;
    int64_t grain_offset;
    char filler[1];
    char check_bytes[4];
} __attribute__((packed)) VMDK4Header;

#define L2_CACHE_SIZE 16

typedef struct VmdkExtent {
    BlockDriverState *file;
    bool flat;
    int64_t sectors;
    int64_t end_sector;
    int64_t l1_table_offset;
    int64_t l1_backup_table_offset;
    uint32_t *l1_table;
    uint32_t *l1_backup_table;
    unsigned int l1_size;
    uint32_t l1_entry_sectors;

    unsigned int l2_size;
    uint32_t *l2_cache;
    uint32_t l2_cache_offsets[L2_CACHE_SIZE];
    uint32_t l2_cache_counts[L2_CACHE_SIZE];

    unsigned int cluster_sectors;
} VmdkExtent;

typedef struct BDRVVmdkState {
    uint32_t parent_cid;
    int num_extents;
    /* Extent array with num_extents entries, ascend ordered by address */
    VmdkExtent *extents;
} BDRVVmdkState;

typedef struct VmdkMetaData {
    uint32_t offset;
    unsigned int l1_index;
    unsigned int l2_index;
    unsigned int l2_offset;
    int valid;
} VmdkMetaData;

static int vmdk_probe(const uint8_t *buf, int buf_size, const char *filename)
{
    uint32_t magic;

    if (buf_size < 4)
        return 0;
    magic = be32_to_cpu(*(uint32_t *)buf);
    if (magic == VMDK3_MAGIC ||
        magic == VMDK4_MAGIC) {
        return 100;
    } else {
        const char *p = (const char *)buf;
        const char *end = p + buf_size;
        while (p < end) {
            if (*p == '#') {
                /* skip comment line */
                while (p < end && *p != '\n') {
                    p++;
                }
                p++;
                continue;
            }
            if (*p == ' ') {
                while (p < end && *p == ' ') {
                    p++;
                }
                /* skip '\r' if windows line endings used. */
                if (p < end && *p == '\r') {
                    p++;
                }
                /* only accept blank lines before 'version=' line */
                if (p == end || *p != '\n') {
                    return 0;
                }
                p++;
                continue;
            }
            if (end - p >= strlen("version=X\n")) {
                if (strncmp("version=1\n", p, strlen("version=1\n")) == 0 ||
                    strncmp("version=2\n", p, strlen("version=2\n")) == 0) {
                    return 100;
                }
            }
            if (end - p >= strlen("version=X\r\n")) {
                if (strncmp("version=1\r\n", p, strlen("version=1\r\n")) == 0 ||
                    strncmp("version=2\r\n", p, strlen("version=2\r\n")) == 0) {
                    return 100;
                }
            }
            return 0;
        }
        return 0;
    }
}

#define CHECK_CID 1

#define SECTOR_SIZE 512
#define DESC_SIZE 20*SECTOR_SIZE	// 20 sectors of 512 bytes each
#define HEADER_SIZE 512   			// first sector of 512 bytes

static void vmdk_free_extents(BlockDriverState *bs)
{
    int i;
    BDRVVmdkState *s = bs->opaque;

    for (i = 0; i < s->num_extents; i++) {
        qemu_free(s->extents[i].l1_table);
        qemu_free(s->extents[i].l2_cache);
        qemu_free(s->extents[i].l1_backup_table);
    }
    qemu_free(s->extents);
}

static uint32_t vmdk_read_cid(BlockDriverState *bs, int parent)
{
    char desc[DESC_SIZE];
    uint32_t cid;
    const char *p_name, *cid_str;
    size_t cid_str_size;

    /* the descriptor offset = 0x200 */
    if (bdrv_pread(bs->file, 0x200, desc, DESC_SIZE) != DESC_SIZE)
        return 0;

    if (parent) {
        cid_str = "parentCID";
        cid_str_size = sizeof("parentCID");
    } else {
        cid_str = "CID";
        cid_str_size = sizeof("CID");
    }

    if ((p_name = strstr(desc,cid_str)) != NULL) {
        p_name += cid_str_size;
        sscanf(p_name,"%x",&cid);
    }

    return cid;
}

static int vmdk_write_cid(BlockDriverState *bs, uint32_t cid)
{
    char desc[DESC_SIZE], tmp_desc[DESC_SIZE];
    char *p_name, *tmp_str;

    /* the descriptor offset = 0x200 */
    if (bdrv_pread(bs->file, 0x200, desc, DESC_SIZE) != DESC_SIZE)
        return -1;

    tmp_str = strstr(desc,"parentCID");
    pstrcpy(tmp_desc, sizeof(tmp_desc), tmp_str);
    if ((p_name = strstr(desc,"CID")) != NULL) {
        p_name += sizeof("CID");
        snprintf(p_name, sizeof(desc) - (p_name - desc), "%x\n", cid);
        pstrcat(desc, sizeof(desc), tmp_desc);
    }

    if (bdrv_pwrite_sync(bs->file, 0x200, desc, DESC_SIZE) < 0)
        return -1;
    return 0;
}

static int vmdk_is_cid_valid(BlockDriverState *bs)
{
#ifdef CHECK_CID
    BDRVVmdkState *s = bs->opaque;
    BlockDriverState *p_bs = bs->backing_hd;
    uint32_t cur_pcid;

    if (p_bs) {
        cur_pcid = vmdk_read_cid(p_bs,0);
        if (s->parent_cid != cur_pcid)
            // CID not valid
            return 0;
    }
#endif
    // CID valid
    return 1;
}

static int vmdk_snapshot_create(const char *filename, const char *backing_file)
{
    int snp_fd, p_fd;
    int ret;
    uint32_t p_cid;
    char *p_name, *gd_buf, *rgd_buf;
    const char *real_filename, *temp_str;
    VMDK4Header header;
    uint32_t gde_entries, gd_size;
    int64_t gd_offset, rgd_offset, capacity, gt_size;
    char p_desc[DESC_SIZE], s_desc[DESC_SIZE], hdr[HEADER_SIZE];
    static const char desc_template[] =
    "# Disk DescriptorFile\n"
    "version=1\n"
    "CID=%x\n"
    "parentCID=%x\n"
    "createType=\"monolithicSparse\"\n"
    "parentFileNameHint=\"%s\"\n"
    "\n"
    "# Extent description\n"
    "RW %u SPARSE \"%s\"\n"
    "\n"
    "# The Disk Data Base \n"
    "#DDB\n"
    "\n";

    snp_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_BINARY | O_LARGEFILE, 0644);
    if (snp_fd < 0)
        return -errno;
    p_fd = open(backing_file, O_RDONLY | O_BINARY | O_LARGEFILE);
    if (p_fd < 0) {
        close(snp_fd);
        return -errno;
    }

    /* read the header */
    if (lseek(p_fd, 0x0, SEEK_SET) == -1) {
        ret = -errno;
        goto fail;
    }
    if (read(p_fd, hdr, HEADER_SIZE) != HEADER_SIZE) {
        ret = -errno;
        goto fail;
    }

    /* write the header */
    if (lseek(snp_fd, 0x0, SEEK_SET) == -1) {
        ret = -errno;
        goto fail;
    }
    if (write(snp_fd, hdr, HEADER_SIZE) == -1) {
        ret = -errno;
        goto fail;
    }

    memset(&header, 0, sizeof(header));
    memcpy(&header,&hdr[4], sizeof(header)); // skip the VMDK4_MAGIC

    if (ftruncate(snp_fd, header.grain_offset << 9)) {
        ret = -errno;
        goto fail;
    }
    /* the descriptor offset = 0x200 */
    if (lseek(p_fd, 0x200, SEEK_SET) == -1) {
        ret = -errno;
        goto fail;
    }
    if (read(p_fd, p_desc, DESC_SIZE) != DESC_SIZE) {
        ret = -errno;
        goto fail;
    }

    if ((p_name = strstr(p_desc,"CID")) != NULL) {
        p_name += sizeof("CID");
        sscanf(p_name,"%x",&p_cid);
    }

    real_filename = filename;
    if ((temp_str = strrchr(real_filename, '\\')) != NULL)
        real_filename = temp_str + 1;
    if ((temp_str = strrchr(real_filename, '/')) != NULL)
        real_filename = temp_str + 1;
    if ((temp_str = strrchr(real_filename, ':')) != NULL)
        real_filename = temp_str + 1;

    snprintf(s_desc, sizeof(s_desc), desc_template, p_cid, p_cid, backing_file,
             (uint32_t)header.capacity, real_filename);

    /* write the descriptor */
    if (lseek(snp_fd, 0x200, SEEK_SET) == -1) {
        ret = -errno;
        goto fail;
    }
    if (write(snp_fd, s_desc, strlen(s_desc)) == -1) {
        ret = -errno;
        goto fail;
    }

    gd_offset = header.gd_offset * SECTOR_SIZE;     // offset of GD table
    rgd_offset = header.rgd_offset * SECTOR_SIZE;   // offset of RGD table
    capacity = header.capacity * SECTOR_SIZE;       // Extent size
    /*
     * Each GDE span 32M disk, means:
     * 512 GTE per GT, each GTE points to grain
     */
    gt_size = (int64_t)header.num_gtes_per_gte * header.granularity * SECTOR_SIZE;
    if (!gt_size) {
        ret = -EINVAL;
        goto fail;
    }
    gde_entries = (uint32_t)(capacity / gt_size);  // number of gde/rgde
    gd_size = gde_entries * sizeof(uint32_t);

    /* write RGD */
    rgd_buf = qemu_malloc(gd_size);
    if (lseek(p_fd, rgd_offset, SEEK_SET) == -1) {
        ret = -errno;
        goto fail_rgd;
    }
    if (read(p_fd, rgd_buf, gd_size) != gd_size) {
        ret = -errno;
        goto fail_rgd;
    }
    if (lseek(snp_fd, rgd_offset, SEEK_SET) == -1) {
        ret = -errno;
        goto fail_rgd;
    }
    if (write(snp_fd, rgd_buf, gd_size) == -1) {
        ret = -errno;
        goto fail_rgd;
    }

    /* write GD */
    gd_buf = qemu_malloc(gd_size);
    if (lseek(p_fd, gd_offset, SEEK_SET) == -1) {
        ret = -errno;
        goto fail_gd;
    }
    if (read(p_fd, gd_buf, gd_size) != gd_size) {
        ret = -errno;
        goto fail_gd;
    }
    if (lseek(snp_fd, gd_offset, SEEK_SET) == -1) {
        ret = -errno;
        goto fail_gd;
    }
    if (write(snp_fd, gd_buf, gd_size) == -1) {
        ret = -errno;
        goto fail_gd;
    }
    ret = 0;

fail_gd:
    qemu_free(gd_buf);
fail_rgd:
    qemu_free(rgd_buf);
fail:
    close(p_fd);
    close(snp_fd);
    return ret;
}

static int vmdk_parent_open(BlockDriverState *bs)
{
    char *p_name;
    char desc[DESC_SIZE];

    /* the descriptor offset = 0x200 */
    if (bdrv_pread(bs->file, 0x200, desc, DESC_SIZE) != DESC_SIZE)
        return -1;

    if ((p_name = strstr(desc,"parentFileNameHint")) != NULL) {
        char *end_name;

        p_name += sizeof("parentFileNameHint") + 1;
        if ((end_name = strchr(p_name,'\"')) == NULL)
            return -1;
        if ((end_name - p_name) > sizeof (bs->backing_file) - 1)
            return -1;

        pstrcpy(bs->backing_file, end_name - p_name + 1, p_name);
    }

    return 0;
}

/* Create and append extent to the extent array. Return the added VmdkExtent
 * address. return NULL if allocation failed. */
static VmdkExtent *vmdk_add_extent(BlockDriverState *bs,
                           BlockDriverState *file, bool flat, int64_t sectors,
                           int64_t l1_offset, int64_t l1_backup_offset,
                           uint32_t l1_size,
                           int l2_size, unsigned int cluster_sectors)
{
    VmdkExtent *extent;
    BDRVVmdkState *s = bs->opaque;

    s->extents = qemu_realloc(s->extents,
                              (s->num_extents + 1) * sizeof(VmdkExtent));
    extent = &s->extents[s->num_extents];
    s->num_extents++;

    memset(extent, 0, sizeof(VmdkExtent));
    extent->file = file;
    extent->flat = flat;
    extent->sectors = sectors;
    extent->l1_table_offset = l1_offset;
    extent->l1_backup_table_offset = l1_backup_offset;
    extent->l1_size = l1_size;
    extent->l1_entry_sectors = l2_size * cluster_sectors;
    extent->l2_size = l2_size;
    extent->cluster_sectors = cluster_sectors;

    if (s->num_extents > 1) {
        extent->end_sector = (*(extent - 1)).end_sector + extent->sectors;
    } else {
        extent->end_sector = extent->sectors;
    }
    bs->total_sectors = extent->end_sector;
    return extent;
}

static int vmdk_init_tables(BlockDriverState *bs, VmdkExtent *extent)
{
    int ret;
    int l1_size, i;

    /* read the L1 table */
    l1_size = extent->l1_size * sizeof(uint32_t);
    extent->l1_table = qemu_malloc(l1_size);
    ret = bdrv_pread(extent->file,
                    extent->l1_table_offset,
                    extent->l1_table,
                    l1_size);
    if (ret < 0) {
        goto fail_l1;
    }
    for (i = 0; i < extent->l1_size; i++) {
        le32_to_cpus(&extent->l1_table[i]);
    }

    if (extent->l1_backup_table_offset) {
        extent->l1_backup_table = qemu_malloc(l1_size);
        ret = bdrv_pread(extent->file,
                        extent->l1_backup_table_offset,
                        extent->l1_backup_table,
                        l1_size);
        if (ret < 0) {
            goto fail_l1b;
        }
        for (i = 0; i < extent->l1_size; i++) {
            le32_to_cpus(&extent->l1_backup_table[i]);
        }
    }

    extent->l2_cache =
        qemu_malloc(extent->l2_size * L2_CACHE_SIZE * sizeof(uint32_t));
    return 0;
 fail_l1b:
    qemu_free(extent->l1_backup_table);
 fail_l1:
    qemu_free(extent->l1_table);
    return ret;
}

static int vmdk_open_vmdk3(BlockDriverState *bs, int flags)
{
    int ret;
    uint32_t magic;
    VMDK3Header header;
    VmdkExtent *extent;

    ret = bdrv_pread(bs->file, sizeof(magic), &header, sizeof(header));
    if (ret < 0) {
        goto fail;
    }
    extent = vmdk_add_extent(bs,
                             bs->file, false,
                             le32_to_cpu(header.disk_sectors),
                             le32_to_cpu(header.l1dir_offset) << 9,
                             0, 1 << 6, 1 << 9,
                             le32_to_cpu(header.granularity));
    ret = vmdk_init_tables(bs, extent);
    if (ret) {
        /* vmdk_init_tables cleans up on fail, so only free allocation of
         * vmdk_add_extent here. */
        goto fail;
    }
    return 0;
 fail:
    vmdk_free_extents(bs);
    return ret;
}

static int vmdk_open_vmdk4(BlockDriverState *bs, int flags)
{
    int ret;
    uint32_t magic;
    uint32_t l1_size, l1_entry_sectors;
    VMDK4Header header;
    BDRVVmdkState *s = bs->opaque;
    VmdkExtent *extent;

    ret = bdrv_pread(bs->file, sizeof(magic), &header, sizeof(header));
    if (ret < 0) {
        goto fail;
    }
    l1_entry_sectors = le32_to_cpu(header.num_gtes_per_gte)
                        * le64_to_cpu(header.granularity);
    l1_size = (le64_to_cpu(header.capacity) + l1_entry_sectors - 1)
                / l1_entry_sectors;
    extent = vmdk_add_extent(bs, bs->file, false,
                          le64_to_cpu(header.capacity),
                          le64_to_cpu(header.gd_offset) << 9,
                          le64_to_cpu(header.rgd_offset) << 9,
                          l1_size,
                          le32_to_cpu(header.num_gtes_per_gte),
                          le64_to_cpu(header.granularity));
    if (extent->l1_entry_sectors <= 0) {
        ret = -EINVAL;
        goto fail;
    }
    /* try to open parent images, if exist */
    ret = vmdk_parent_open(bs);
    if (ret) {
        goto fail;
    }
    s->parent_cid = vmdk_read_cid(bs, 1);
    ret = vmdk_init_tables(bs, extent);
    if (ret) {
        goto fail;
    }
    return 0;
 fail:
    vmdk_free_extents(bs);
    return ret;
}

static int vmdk_open(BlockDriverState *bs, int flags)
{
    uint32_t magic;

    if (bdrv_pread(bs->file, 0, &magic, sizeof(magic)) != sizeof(magic)) {
        return -EIO;
    }

    magic = be32_to_cpu(magic);
    if (magic == VMDK3_MAGIC) {
        return vmdk_open_vmdk3(bs, flags);
    } else if (magic == VMDK4_MAGIC) {
        return vmdk_open_vmdk4(bs, flags);
    } else {
        return -EINVAL;
    }
}

static int get_whole_cluster(BlockDriverState *bs,
                VmdkExtent *extent,
                uint64_t cluster_offset,
                uint64_t offset,
                bool allocate)
{
    /* 128 sectors * 512 bytes each = grain size 64KB */
    uint8_t  whole_grain[extent->cluster_sectors * 512];

    /* we will be here if it's first write on non-exist grain(cluster).
     * try to read from parent image, if exist */
    if (bs->backing_hd) {
        int ret;

        if (!vmdk_is_cid_valid(bs))
            return -1;

        /* floor offset to cluster */
        offset -= offset % (extent->cluster_sectors * 512);
        ret = bdrv_read(bs->backing_hd, offset >> 9, whole_grain,
                extent->cluster_sectors);
        if (ret < 0) {
            return -1;
        }

        /* Write grain only into the active image */
        ret = bdrv_write(extent->file, cluster_offset, whole_grain,
                extent->cluster_sectors);
        if (ret < 0) {
            return -1;
        }
    }
    return 0;
}

static int vmdk_L2update(VmdkExtent *extent, VmdkMetaData *m_data)
{
    /* update L2 table */
    if (bdrv_pwrite_sync(
                extent->file,
                ((int64_t)m_data->l2_offset * 512)
                    + (m_data->l2_index * sizeof(m_data->offset)),
                &(m_data->offset),
                sizeof(m_data->offset)
            ) < 0) {
        return -1;
    }
    /* update backup L2 table */
    if (extent->l1_backup_table_offset != 0) {
        m_data->l2_offset = extent->l1_backup_table[m_data->l1_index];
        if (bdrv_pwrite_sync(
                    extent->file,
                    ((int64_t)m_data->l2_offset * 512)
                        + (m_data->l2_index * sizeof(m_data->offset)),
                    &(m_data->offset), sizeof(m_data->offset)
                ) < 0) {
            return -1;
        }
    }

    return 0;
}

static uint64_t get_cluster_offset(BlockDriverState *bs,
                                    VmdkExtent *extent,
                                    VmdkMetaData *m_data,
                                    uint64_t offset, int allocate)
{
    unsigned int l1_index, l2_offset, l2_index;
    int min_index, i, j;
    uint32_t min_count, *l2_table, tmp = 0;
    uint64_t cluster_offset;

    if (m_data)
        m_data->valid = 0;

    l1_index = (offset >> 9) / extent->l1_entry_sectors;
    if (l1_index >= extent->l1_size) {
        return 0;
    }
    l2_offset = extent->l1_table[l1_index];
    if (!l2_offset) {
        return 0;
    }
    for (i = 0; i < L2_CACHE_SIZE; i++) {
        if (l2_offset == extent->l2_cache_offsets[i]) {
            /* increment the hit count */
            if (++extent->l2_cache_counts[i] == 0xffffffff) {
                for (j = 0; j < L2_CACHE_SIZE; j++) {
                    extent->l2_cache_counts[j] >>= 1;
                }
            }
            l2_table = extent->l2_cache + (i * extent->l2_size);
            goto found;
        }
    }
    /* not found: load a new entry in the least used one */
    min_index = 0;
    min_count = 0xffffffff;
    for (i = 0; i < L2_CACHE_SIZE; i++) {
        if (extent->l2_cache_counts[i] < min_count) {
            min_count = extent->l2_cache_counts[i];
            min_index = i;
        }
    }
    l2_table = extent->l2_cache + (min_index * extent->l2_size);
    if (bdrv_pread(
                extent->file,
                (int64_t)l2_offset * 512,
                l2_table,
                extent->l2_size * sizeof(uint32_t)
            ) != extent->l2_size * sizeof(uint32_t)) {
        return 0;
    }

    extent->l2_cache_offsets[min_index] = l2_offset;
    extent->l2_cache_counts[min_index] = 1;
 found:
    l2_index = ((offset >> 9) / extent->cluster_sectors) % extent->l2_size;
    cluster_offset = le32_to_cpu(l2_table[l2_index]);

    if (!cluster_offset) {
        if (!allocate)
            return 0;

        // Avoid the L2 tables update for the images that have snapshots.
        cluster_offset = bdrv_getlength(extent->file);
        bdrv_truncate(
            extent->file,
            cluster_offset + (extent->cluster_sectors << 9)
        );

        cluster_offset >>= 9;
        tmp = cpu_to_le32(cluster_offset);
        l2_table[l2_index] = tmp;

        /* First of all we write grain itself, to avoid race condition
         * that may to corrupt the image.
         * This problem may occur because of insufficient space on host disk
         * or inappropriate VM shutdown.
         */
        if (get_whole_cluster(
                bs, extent, cluster_offset, offset, allocate) == -1)
            return 0;

        if (m_data) {
            m_data->offset = tmp;
            m_data->l1_index = l1_index;
            m_data->l2_index = l2_index;
            m_data->l2_offset = l2_offset;
            m_data->valid = 1;
        }
    }
    cluster_offset <<= 9;
    return cluster_offset;
}

static VmdkExtent *find_extent(BDRVVmdkState *s,
                                int64_t sector_num, VmdkExtent *start_hint)
{
    VmdkExtent *extent = start_hint;

    if (!extent) {
        extent = &s->extents[0];
    }
    while (extent < &s->extents[s->num_extents]) {
        if (sector_num < extent->end_sector) {
            return extent;
        }
        extent++;
    }
    return NULL;
}

static int vmdk_is_allocated(BlockDriverState *bs, int64_t sector_num,
                             int nb_sectors, int *pnum)
{
    BDRVVmdkState *s = bs->opaque;

    int64_t index_in_cluster, n, ret;
    uint64_t offset;
    VmdkExtent *extent;

    extent = find_extent(s, sector_num, NULL);
    if (!extent) {
        return 0;
    }
    if (extent->flat) {
        n = extent->end_sector - sector_num;
        ret = 1;
    } else {
        offset = get_cluster_offset(bs, extent, NULL, sector_num * 512, 0);
        index_in_cluster = sector_num % extent->cluster_sectors;
        n = extent->cluster_sectors - index_in_cluster;
        ret = offset ? 1 : 0;
    }
    if (n > nb_sectors)
        n = nb_sectors;
    *pnum = n;
    return ret;
}

static int vmdk_read(BlockDriverState *bs, int64_t sector_num,
                    uint8_t *buf, int nb_sectors)
{
    BDRVVmdkState *s = bs->opaque;
    int ret;
    uint64_t n, index_in_cluster;
    VmdkExtent *extent = NULL;
    uint64_t cluster_offset;

    while (nb_sectors > 0) {
        extent = find_extent(s, sector_num, extent);
        if (!extent) {
            return -EIO;
        }
        cluster_offset = get_cluster_offset(
                            bs, extent, NULL, sector_num << 9, 0);
        index_in_cluster = sector_num % extent->cluster_sectors;
        n = extent->cluster_sectors - index_in_cluster;
        if (n > nb_sectors)
            n = nb_sectors;
        if (!cluster_offset) {
            // try to read from parent image, if exist
            if (bs->backing_hd) {
                if (!vmdk_is_cid_valid(bs))
                    return -1;
                ret = bdrv_read(bs->backing_hd, sector_num, buf, n);
                if (ret < 0)
                    return -1;
            } else {
                memset(buf, 0, 512 * n);
            }
        } else {
            if(bdrv_pread(bs->file, cluster_offset + index_in_cluster * 512, buf, n * 512) != n * 512)
                return -1;
        }
        nb_sectors -= n;
        sector_num += n;
        buf += n * 512;
    }
    return 0;
}

static int vmdk_write(BlockDriverState *bs, int64_t sector_num,
                     const uint8_t *buf, int nb_sectors)
{
    BDRVVmdkState *s = bs->opaque;
    VmdkExtent *extent = NULL;
    int n;
    int64_t index_in_cluster;
    uint64_t cluster_offset;
    static int cid_update = 0;
    VmdkMetaData m_data;

    if (sector_num > bs->total_sectors) {
        fprintf(stderr,
                "(VMDK) Wrong offset: sector_num=0x%" PRIx64
                " total_sectors=0x%" PRIx64 "\n",
                sector_num, bs->total_sectors);
        return -1;
    }

    while (nb_sectors > 0) {
        extent = find_extent(s, sector_num, extent);
        if (!extent) {
            return -EIO;
        }
        cluster_offset = get_cluster_offset(
                                bs,
                                extent,
                                &m_data,
                                sector_num << 9, 1);
        if (!cluster_offset) {
            return -1;
        }
        index_in_cluster = sector_num % extent->cluster_sectors;
        n = extent->cluster_sectors - index_in_cluster;
        if (n > nb_sectors) {
            n = nb_sectors;
        }

        if (bdrv_pwrite(bs->file,
                        cluster_offset + index_in_cluster * 512,
                        buf, n * 512)
                != n * 512) {
            return -1;
        }
        if (m_data.valid) {
            /* update L2 tables */
            if (vmdk_L2update(extent, &m_data) == -1) {
                return -1;
            }
        }
        nb_sectors -= n;
        sector_num += n;
        buf += n * 512;

        // update CID on the first write every time the virtual disk is opened
        if (!cid_update) {
            vmdk_write_cid(bs, time(NULL));
            cid_update++;
        }
    }
    return 0;
}

static int vmdk_create(const char *filename, QEMUOptionParameter *options)
{
    int fd, i;
    VMDK4Header header;
    uint32_t tmp, magic, grains, gd_size, gt_size, gt_count;
    static const char desc_template[] =
        "# Disk DescriptorFile\n"
        "version=1\n"
        "CID=%x\n"
        "parentCID=ffffffff\n"
        "createType=\"monolithicSparse\"\n"
        "\n"
        "# Extent description\n"
        "RW %" PRId64 " SPARSE \"%s\"\n"
        "\n"
        "# The Disk Data Base \n"
        "#DDB\n"
        "\n"
        "ddb.virtualHWVersion = \"%d\"\n"
        "ddb.geometry.cylinders = \"%" PRId64 "\"\n"
        "ddb.geometry.heads = \"16\"\n"
        "ddb.geometry.sectors = \"63\"\n"
        "ddb.adapterType = \"ide\"\n";
    char desc[1024];
    const char *real_filename, *temp_str;
    int64_t total_size = 0;
    const char *backing_file = NULL;
    int flags = 0;
    int ret;

    // Read out options
    while (options && options->name) {
        if (!strcmp(options->name, BLOCK_OPT_SIZE)) {
            total_size = options->value.n / 512;
        } else if (!strcmp(options->name, BLOCK_OPT_BACKING_FILE)) {
            backing_file = options->value.s;
        } else if (!strcmp(options->name, BLOCK_OPT_COMPAT6)) {
            flags |= options->value.n ? BLOCK_FLAG_COMPAT6: 0;
        }
        options++;
    }

    /* XXX: add support for backing file */
    if (backing_file) {
        return vmdk_snapshot_create(filename, backing_file);
    }

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY | O_LARGEFILE,
              0644);
    if (fd < 0)
        return -errno;
    magic = cpu_to_be32(VMDK4_MAGIC);
    memset(&header, 0, sizeof(header));
    header.version = 1;
    header.flags = 3; /* ?? */
    header.capacity = total_size;
    header.granularity = 128;
    header.num_gtes_per_gte = 512;

    grains = (total_size + header.granularity - 1) / header.granularity;
    gt_size = ((header.num_gtes_per_gte * sizeof(uint32_t)) + 511) >> 9;
    gt_count = (grains + header.num_gtes_per_gte - 1) / header.num_gtes_per_gte;
    gd_size = (gt_count * sizeof(uint32_t) + 511) >> 9;

    header.desc_offset = 1;
    header.desc_size = 20;
    header.rgd_offset = header.desc_offset + header.desc_size;
    header.gd_offset = header.rgd_offset + gd_size + (gt_size * gt_count);
    header.grain_offset =
       ((header.gd_offset + gd_size + (gt_size * gt_count) +
         header.granularity - 1) / header.granularity) *
        header.granularity;

    /* swap endianness for all header fields */
    header.version = cpu_to_le32(header.version);
    header.flags = cpu_to_le32(header.flags);
    header.capacity = cpu_to_le64(header.capacity);
    header.granularity = cpu_to_le64(header.granularity);
    header.num_gtes_per_gte = cpu_to_le32(header.num_gtes_per_gte);
    header.desc_offset = cpu_to_le64(header.desc_offset);
    header.desc_size = cpu_to_le64(header.desc_size);
    header.rgd_offset = cpu_to_le64(header.rgd_offset);
    header.gd_offset = cpu_to_le64(header.gd_offset);
    header.grain_offset = cpu_to_le64(header.grain_offset);

    header.check_bytes[0] = 0xa;
    header.check_bytes[1] = 0x20;
    header.check_bytes[2] = 0xd;
    header.check_bytes[3] = 0xa;

    /* write all the data */
    ret = qemu_write_full(fd, &magic, sizeof(magic));
    if (ret != sizeof(magic)) {
        ret = -errno;
        goto exit;
    }
    ret = qemu_write_full(fd, &header, sizeof(header));
    if (ret != sizeof(header)) {
        ret = -errno;
        goto exit;
    }

    ret = ftruncate(fd, le64_to_cpu(header.grain_offset) << 9);
    if (ret < 0) {
        ret = -errno;
        goto exit;
    }

    /* write grain directory */
    lseek(fd, le64_to_cpu(header.rgd_offset) << 9, SEEK_SET);
    for (i = 0, tmp = le64_to_cpu(header.rgd_offset) + gd_size;
         i < gt_count; i++, tmp += gt_size) {
        ret = qemu_write_full(fd, &tmp, sizeof(tmp));
        if (ret != sizeof(tmp)) {
            ret = -errno;
            goto exit;
        }
    }

    /* write backup grain directory */
    lseek(fd, le64_to_cpu(header.gd_offset) << 9, SEEK_SET);
    for (i = 0, tmp = le64_to_cpu(header.gd_offset) + gd_size;
         i < gt_count; i++, tmp += gt_size) {
        ret = qemu_write_full(fd, &tmp, sizeof(tmp));
        if (ret != sizeof(tmp)) {
            ret = -errno;
            goto exit;
        }
    }

    /* compose the descriptor */
    real_filename = filename;
    if ((temp_str = strrchr(real_filename, '\\')) != NULL)
        real_filename = temp_str + 1;
    if ((temp_str = strrchr(real_filename, '/')) != NULL)
        real_filename = temp_str + 1;
    if ((temp_str = strrchr(real_filename, ':')) != NULL)
        real_filename = temp_str + 1;
    snprintf(desc, sizeof(desc), desc_template, (unsigned int)time(NULL),
             total_size, real_filename,
             (flags & BLOCK_FLAG_COMPAT6 ? 6 : 4),
             total_size / (int64_t)(63 * 16));

    /* write the descriptor */
    lseek(fd, le64_to_cpu(header.desc_offset) << 9, SEEK_SET);
    ret = qemu_write_full(fd, desc, strlen(desc));
    if (ret != strlen(desc)) {
        ret = -errno;
        goto exit;
    }

    ret = 0;
exit:
    close(fd);
    return ret;
}

static void vmdk_close(BlockDriverState *bs)
{
    vmdk_free_extents(bs);
}

static int vmdk_flush(BlockDriverState *bs)
{
    return bdrv_flush(bs->file);
}


static QEMUOptionParameter vmdk_create_options[] = {
    {
        .name = BLOCK_OPT_SIZE,
        .type = OPT_SIZE,
        .help = "Virtual disk size"
    },
    {
        .name = BLOCK_OPT_BACKING_FILE,
        .type = OPT_STRING,
        .help = "File name of a base image"
    },
    {
        .name = BLOCK_OPT_COMPAT6,
        .type = OPT_FLAG,
        .help = "VMDK version 6 image"
    },
    { NULL }
};

static BlockDriver bdrv_vmdk = {
    .format_name	= "vmdk",
    .instance_size	= sizeof(BDRVVmdkState),
    .bdrv_probe		= vmdk_probe,
    .bdrv_open      = vmdk_open,
    .bdrv_read		= vmdk_read,
    .bdrv_write		= vmdk_write,
    .bdrv_close		= vmdk_close,
    .bdrv_create	= vmdk_create,
    .bdrv_flush		= vmdk_flush,
    .bdrv_is_allocated	= vmdk_is_allocated,

    .create_options = vmdk_create_options,
};

static void bdrv_vmdk_init(void)
{
    bdrv_register(&bdrv_vmdk);
}

block_init(bdrv_vmdk_init);
