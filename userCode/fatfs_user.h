#ifndef FATFS_USER_H
#define FATFS_USER_H

#include "ff.h"

/**
 * @brief  FatFs 目录项遍历回调函数。
 * @param  path      当前目录项的完整路径，例如 "0:/LOG/DATA.TXT"。
 * @param  info      当前目录项信息，包含属性、文件大小和文件名。
 * @param  context   用户自定义参数，原样传给回调函数。
 * @retval 0         继续遍历。
 * @retval 非 0      停止遍历，FATFS_User_List() 返回 FR_INT_ERR。
 */
typedef int (*FATFS_User_FileCallback)(const char *path, const FILINFO *info, void *context);

/**
 * @brief  挂载 SD 卡上的 FatFs 文件系统。
 * @note   文件读写、目录遍历等操作前必须成功调用一次。
 * @retval FR_OK      挂载成功。
 * @retval 其他值    FatFs 错误码，详见 FRESULT。
 */
FRESULT FATFS_User_Mount(void);

/**
 * @brief  创建或覆盖写入一个文本文件。
 * @param  path  文件路径，例如 "0:/CONFIG.TXT"。
 * @param  text  以 '\0' 结尾的文本字符串。
 * @retval FR_OK      写入成功。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_WriteText(const char *path, const char *text);

/**
 * @brief  读取文件内容并转换为以 '\0' 结尾的字符串。
 * @param  path        文件路径。
 * @param  buffer      接收数据的缓冲区。
 * @param  buffer_size 缓冲区总大小，至少为 2 字节。
 * @param  read_size   实际读取的字节数，不包含结尾的 '\0'。
 * @note   最多读取 buffer_size - 1 字节。
 * @retval FR_OK      读取成功。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_ReadText(const char *path, char *buffer, UINT buffer_size, UINT *read_size);

/**
 * @brief  向文件末尾追加二进制或文本数据。
 * @param  path 文件路径，不存在时会创建。
 * @param  data 待读取的数据缓冲区。
 * @param  size 写入字节数。
 * @retval FR_OK      写入成功。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_Append(const char *path, const void *data, UINT size);

/**
 * @brief  获取 SD 卡文件系统的总容量和剩余容量。
 * @param  total_mb 总容量，单位 MB。
 * @param  free_mb  剩余容量，单位 MB。
 * @retval FR_OK      获取成功。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_GetSpace(DWORD *total_mb, DWORD *free_mb);

/**
 * @brief  格式化 SD 卡并重新挂载文件系统。
 * @warning 此操作会删除 SD 卡上的全部文件，禁止在正常启动流程中自动调用。
 * @retval FR_OK      格式化并重新挂载成功。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_Format(void);

/**
 * @brief  递归遍历目录及其所有子目录。
 * @param  path     起始目录，例如 "0:/"。
 * @param  callback 每发现一个文件或目录时调用的回调函数。
 * @param  context  传给回调函数的用户参数。
 * @retval FR_OK      遍历完成。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_List(const char *path, FATFS_User_FileCallback callback, void *context);

/**
 * @brief  递归遍历目录，并通过 printf 打印目录树。
 * @param  path 起始目录，例如 "0:/"。
 * @retval FR_OK      遍历完成。
 * @retval 其他值    FatFs 错误码。
 */
FRESULT FATFS_User_PrintTree(const char *path);

#endif