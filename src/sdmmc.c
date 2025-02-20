#include <stdio.h>
#include <malloc.h>
#include "sdmmc_sd.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "bsp.h"
#include "errno.h"

static FATFS sdmmc_fatfs;

/**
 * get total and free volume of sdcard
 *
 * capacity = cluster_cnt * sector_per_cluster * sector_size / 1MB
 */
static void sdmmc_get_capacity(void)
{
    FATFS *fs;
    DWORD free_cluster, free_sector, total_sector;
    WORD byte_per_sector;
    int free_capacity, total_capacity;

    // get volume info
    f_getfree("0:", &free_cluster, &fs);
    disk_ioctl(0, GET_SECTOR_SIZE, &byte_per_sector);

    // caculate size
    free_sector = free_cluster * fs->csize;
    free_capacity = free_sector / 1024 / 1024 * byte_per_sector;

    total_sector = (fs->n_fatent - 2) * fs->csize;
    total_capacity = total_sector / 1024 / 1024 * byte_per_sector;

    if (free_capacity > 4096)
        pr_info("sdmmc: free: %3.1fGB, total: %3.1fGB", 
                (float)free_capacity/1024, (float)total_capacity/1024);
    else if (total_capacity < 4096)
        pr_info("sdmmc: free: %dMB, total: %dMB", 
                free_capacity, total_capacity);
    else {
        pr_info("sdmmc: free: %dMB, total: %3.1fGB", 
                free_capacity, (float)total_capacity/1024);
    }
}


/**
 * mount fatfs
 */
void sdmmc_mount(void)
{
    char path[4];
    BYTE work[FF_MAX_SS];
    FRESULT fs_ret;

    FATFS_LinkDriver(&SD_Driver, path);
    fs_ret = f_mount(&sdmmc_fatfs, "0:", 1);

    if (fs_ret == FR_OK) {
        pr_info("sdmmc: fatfs mounted");
        sdmmc_get_capacity();
    } else {
        pr_info("sdmmc: failed to mount sdcard");
    }
}


/*
 * read kernel image from sdmmc
 */
int sdmmc_read_file(const char *file_name, unsigned char **file_obj, int *file_size)
{
    FRESULT fs_ret;
    FIL file;
    UINT bytes_read;

    // open file
    fs_ret = f_open(&file, file_name, FA_OPEN_EXISTING | FA_READ);
    if (fs_ret != FR_OK) {
        printf("Error: file doesn't exist\r\n");
        return -ENOENT;
    }

    // alloc buffer
    *file_obj = (unsigned char *)SDRAM_BASE_ADDR;
    if (*file_obj == NULL) {
        f_close(&file);
        printf("Error: memory alloc failed for file\r\n");
        return -ENOMEM;
    }

    // read
    fs_ret = f_read(&file, *file_obj, f_size(&file), &bytes_read);
    if (fs_ret != FR_OK || bytes_read != f_size(&file)) {
        printf("Error: failed in reading file\r\n");
        f_close(&file);
        return -EIO;
    }

    *file_size = f_size(&file);
    f_close(&file);
    return 0;
}