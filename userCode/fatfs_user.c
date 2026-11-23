/**
 * @file    fatfs_user.c
 * @brief   基于 SDIO + FatFs 的应用层文件操作封装。
 *
 * 使用流程：
 * 1. MX_SDIO_SD_Init() 初始化 SDIO 外设。
 * 2. MX_FATFS_Init() 注册 SD 卡 DiskIO 驱动。
 * 3. FATFS_User_Mount() 挂载文件系统。
 * 4. 调用本文件提供的文件、目录和容量操作接口。
 *
 * 本文件不直接操作 SDIO、DMA 或 SD 卡命令，底层读写由 FatFs、
 * sd_diskio.c 和 bsp_driver_sd.c 完成。
 */
#include "fatfs_user.h"

#include <stdio.h>
#include <string.h>

#include "fatfs.h"

#define FATFS_USER_PATH_MAX 256

/* 递归遍历时使用的完整路径缓冲区大小。 */
static FRESULT FATFS_User_ListDirectory(const char *path,
                                        FATFS_User_FileCallback callback,
                                        void *context)
{
  /* DIR 保存 FatFs 目录遍历状态，FILINFO 保存当前目录项信息。 */
  DIR directory;
  FILINFO info;
  FRESULT result;
  char item_path[FATFS_USER_PATH_MAX];
  int callback_result;
  int path_length;

  /* 打开目录后，必须通过 f_closedir() 释放目录对象。 */
  result = f_opendir(&directory, path);
  if (result != FR_OK)
  {
    return result;
  }

  for (;;)
  {
    /* f_readdir() 每次读取一个目录项；fname 为空表示遍历结束。 */
    result = f_readdir(&directory, &info);
    if (result != FR_OK || info.fname[0] == '\0')
    {
      break;
    }

    /* 根目录通常以 '/' 结尾，普通子目录则需要手动补 '/'. */
    if (path[strlen(path) - 1] == '/')
    {
      path_length = snprintf(item_path, sizeof(item_path), "%s%s", path, info.fname);
    }
    else
    {
      path_length = snprintf(item_path, sizeof(item_path), "%s/%s", path, info.fname);
    }
    if ((path_length < 0) || (path_length >= (int)sizeof(item_path)))
    {
      result = FR_INVALID_NAME;
      break;
    }

    /* 先通知调用者，再根据属性决定是否递归进入子目录。 */
    callback_result = callback(item_path, &info, context);
    if (callback_result != 0)
    {
      result = FR_INT_ERR;
      break;
    }

    if ((info.fattrib & AM_DIR) != 0)
    {
      result = FATFS_User_ListDirectory(item_path, callback, context);
      if (result != FR_OK)
      {
        break;
      }
    }
  }

  /* 无论遍历成功还是中途失败，都要关闭目录。 */
  if (f_closedir(&directory) != FR_OK)
  {
    result = FR_DISK_ERR;
  }

  return result;
}

static int FATFS_User_PrintItem(const char *path, const FILINFO *info, void *context)
{
  /* 打印回调不需要用户上下文参数。 */
  (void)context;

  if ((info->fattrib & AM_DIR) != 0)
  {
    printf("[DIR ] %s\r\n", path);
  }
  else
  {
    printf("[FILE] %s (%lu bytes)\r\n", path, (unsigned long)info->fsize);
  }

  return 0;
}

FRESULT FATFS_User_Mount(void)
{
  FRESULT result;

  /* force_mount=1：立即检查磁盘并读取文件系统信息。 */
  printf("FatFs: mount start\r\n");
  result = f_mount(&SDFatFS, (TCHAR *)SDPath, 1);
  printf("FatFs: mount result=%d\r\n", result);

  return result;
}

FRESULT FATFS_User_WriteText(const char *path, const char *text)
{
  FRESULT result;
  FIL file;
  UINT written;

  if ((path == NULL) || (text == NULL))
  {
    return FR_INVALID_PARAMETER;
  }

  /* FA_CREATE_ALWAYS 会创建新文件，或清空已有文件后重新写入。 */
  result = f_open(&file, path, FA_CREATE_ALWAYS | FA_WRITE);
  if (result != FR_OK)
  {
    return result;
  }

  result = f_write(&file, text, (UINT)strlen(text), &written);
  /* f_write() 返回 FR_OK 不代表全部数据都写入，还要检查 written。 */
  if ((result == FR_OK) && (written != (UINT)strlen(text)))
  {
    result = FR_DISK_ERR;
  }

  /* 关闭文件会刷新 FatFs 缓冲区，必须检查关闭结果。 */
  if (f_close(&file) != FR_OK)
  {
    result = FR_DISK_ERR;
  }

  return result;
}

FRESULT FATFS_User_ReadText(const char *path, char *buffer, UINT buffer_size, UINT *read_size)
{
  FRESULT result;
  FIL file;
  UINT bytes_read;

  if ((path == NULL) || (buffer == NULL) || (read_size == NULL) || (buffer_size < 2))
  {
    return FR_INVALID_PARAMETER;
  }

  result = f_open(&file, path, FA_READ);
  if (result != FR_OK)
  {
    return result;
  }

  /* 预留 1 字节写入字符串结束符，避免返回的数据不能直接 printf。 */
  result = f_read(&file, buffer, buffer_size - 1, &bytes_read);
  buffer[bytes_read] = '\0';
  *read_size = bytes_read;

  if (f_close(&file) != FR_OK)
  {
    result = FR_DISK_ERR;
  }

  return result;
}

FRESULT FATFS_User_Append(const char *path, const void *data, UINT size)
{
  FRESULT result;
  FIL file;
  UINT written;

  if ((path == NULL) || ((data == NULL) && (size != 0)))
  {
    return FR_INVALID_PARAMETER;
  }

  /* FA_OPEN_APPEND 会定位到文件末尾；文件不存在时自动创建。 */
  result = f_open(&file, path, FA_OPEN_APPEND | FA_WRITE);
  if (result != FR_OK)
  {
    return result;
  }

  result = f_write(&file, data, size, &written);
  if ((result == FR_OK) && (written != size))
  {
    result = FR_DISK_ERR;
  }

  if (f_close(&file) != FR_OK)
  {
    result = FR_DISK_ERR;
  }

  return result;
}

FRESULT FATFS_User_GetSpace(DWORD *total_mb, DWORD *free_mb)
{
  FRESULT result;
  FATFS *file_system;
  DWORD free_clusters;
  DWORD total_sectors;
  DWORD free_sectors;

  if ((total_mb == NULL) || (free_mb == NULL))
  {
    return FR_INVALID_PARAMETER;
  }

  /* f_getfree() 返回空闲簇数以及当前卷的 FATFS 参数。 */
  result = f_getfree((TCHAR *)SDPath, &free_clusters, &file_system);
  if (result != FR_OK)
  {
    return result;
  }

  /* 每个簇包含 csize 个扇区，1 MB = 2048 个 512 字节扇区。 */
  total_sectors = (file_system->n_fatent - 2) * file_system->csize;
  free_sectors = free_clusters * file_system->csize;
  *total_mb = total_sectors / 2048;
  *free_mb = free_sectors / 2048;

  return FR_OK;
}

FRESULT FATFS_User_Format(void)
{
  /* f_mkfs() 需要一个至少能容纳一个扇区的工作缓冲区。 */
  static BYTE work_buffer[_MAX_SS];
  FRESULT result;

  /* 格式化前必须卸载当前文件系统，避免 FAT 缓冲区仍持有旧数据。 */
  result = f_mount(NULL, (TCHAR *)SDPath, 1);
  if (result != FR_OK)
  {
    return result;
  }

  /* FM_ANY 让 FatFs 根据 SD 卡容量选择 FAT 类型。 */
  result = f_mkfs((TCHAR *)SDPath, FM_ANY, 0, work_buffer, sizeof(work_buffer));
  if (result != FR_OK)
  {
    return result;
  }

  /* 格式化成功后重新挂载，后续 API 才能继续使用。 */
  return f_mount(&SDFatFS, (TCHAR *)SDPath, 1);
}

FRESULT FATFS_User_List(const char *path, FATFS_User_FileCallback callback, void *context)
{
  /* 公共接口负责参数检查，递归细节由静态函数处理。 */
  if ((path == NULL) || (callback == NULL))
  {
    return FR_INVALID_PARAMETER;
  }

  return FATFS_User_ListDirectory(path, callback, context);
}

FRESULT FATFS_User_PrintTree(const char *path)
{
  if (path == NULL)
  {
    return FR_INVALID_PARAMETER;
  }

  /* 使用内置打印回调，输出到 printf 当前重定向的 USART1。 */
  printf("FatFs: list start %s\r\n", path);
  return FATFS_User_List(path, FATFS_User_PrintItem, NULL);
}