#include "wav_player.h"

#include <stdio.h>
#include <string.h>

#include "i2s.h"

#define WAV_PLAYER_HALF_SAMPLES 2048U
#define WAV_PLAYER_BUFFER_SAMPLES (WAV_PLAYER_HALF_SAMPLES * 2U)
#define WAV_PLAYER_FORMAT_PCM 1U
#define WAV_PLAYER_CHANNELS 2U
#define WAV_PLAYER_BITS_PER_SAMPLE 16U
#define WAV_PLAYER_SAMPLE_RATE 44100U

static FIL s_wav_file;
static uint16_t s_dma_buffer[WAV_PLAYER_BUFFER_SAMPLES];
static volatile uint8_t s_completed_halves;
static volatile uint8_t s_is_playing;
static volatile FRESULT s_result = FR_OK;
static DWORD s_data_remaining;
static int8_t s_stop_after_half = -1;
static uint8_t s_last_data_half;
static uint8_t s_has_audio_data;
static uint8_t s_file_open;

/**
 * @brief  将两个小端字节组合成一个 16 位无符号数。
 * @param  data 指向至少 2 字节的输入数据。
 * @retval 按 little-endian 解码得到的 16 位数值。
 */
static uint16_t WAV_ReadLE16(const BYTE *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

/**
 * @brief  将四个小端字节组合成一个 FatFs DWORD。
 * @param  data 指向至少 4 字节的输入数据。
 * @retval 按 little-endian 解码得到的 32 位数值。
 */
static DWORD WAV_ReadLE32(const BYTE *data)
{
  return (DWORD)data[0]
       | ((DWORD)data[1] << 8)
       | ((DWORD)data[2] << 16)
       | ((DWORD)data[3] << 24);
}

/**
 * @brief  从当前 WAV 文件位置精确读取指定字节数。
 * @param  buffer 接收数据的缓冲区。
 * @param  size   必须读取的字节数。
 * @retval FR_OK            成功读取全部字节。
 * @retval FatFs 错误码    底层文件读取失败。
 * @retval FR_INVALID_OBJECT 文件提前结束，未读满指定长度。
 */
static FRESULT WAV_ReadExact(void *buffer, UINT size)
{
  UINT bytes_read;
  FRESULT result = f_read(&s_wav_file, buffer, size, &bytes_read);

  if (result != FR_OK)
  {
    return result;
  }

  return (bytes_read == size) ? FR_OK : FR_INVALID_OBJECT;
}

/**
 * @brief  解析当前已打开 WAV 文件的 RIFF 区块并验证音频格式。
 * @param  data_offset 成功时写入 PCM data 区块在文件中的起始偏移。
 * @param  data_size   成功时写入 PCM 音频数据的字节数。
 * @note   本播放器仅接受 PCM、双声道、16-bit、44100 Hz，且 data 长度须为 4 的倍数。
 * @retval FR_OK                文件头有效且格式受支持。
 * @retval FR_INVALID_OBJECT    WAV/RIFF 区块缺失、截断或结构无效。
 * @retval FR_INVALID_PARAMETER 文件有效，但音频格式不受支持。
 * @retval 其他 FRESULT         文件定位或读取失败。
 */
static FRESULT WAV_ParseHeader(DWORD *data_offset, DWORD *data_size)
{
  BYTE riff_header[12];
  BYTE chunk_header[8];
  BYTE format_data[16];
  DWORD file_size;
  DWORD riff_end;
  DWORD chunk_position;
  DWORD chunk_size;
  DWORD chunk_data_position;
  DWORD next_chunk_position;
  uint16_t audio_format = 0;
  uint16_t channels = 0;
  uint16_t bits_per_sample = 0;
  DWORD sample_rate = 0;
  uint8_t found_format = 0;
  uint8_t found_data = 0;
  FRESULT result;

  file_size = f_size(&s_wav_file);
  if (file_size < sizeof(riff_header))
  {
    return FR_INVALID_OBJECT;
  }

  if (f_lseek(&s_wav_file, 0) != FR_OK)
  {
    return FR_DISK_ERR;
  }

  result = WAV_ReadExact(riff_header, sizeof(riff_header));
  if (result != FR_OK)
  {
    return result;
  }

  if ((memcmp(&riff_header[0], "RIFF", 4) != 0)
      || (memcmp(&riff_header[8], "WAVE", 4) != 0))
  {
    return FR_INVALID_OBJECT;
  }

  chunk_size = WAV_ReadLE32(&riff_header[4]);
  if ((chunk_size > file_size - 8U) || (chunk_size < 4U))
  {
    return FR_INVALID_OBJECT;
  }
  riff_end = chunk_size + 8U;
  chunk_position = sizeof(riff_header);

  while ((chunk_position <= riff_end)
      && ((riff_end - chunk_position) >= sizeof(chunk_header))
      && ((!found_format) || (!found_data)))
  {
    if (f_lseek(&s_wav_file, chunk_position) != FR_OK)
    {
      return FR_DISK_ERR;
    }

    result = WAV_ReadExact(chunk_header, sizeof(chunk_header));
    if (result != FR_OK)
    {
      return result;
    }

    chunk_size = WAV_ReadLE32(&chunk_header[4]);
    chunk_data_position = chunk_position + sizeof(chunk_header);
    if ((chunk_data_position > riff_end)
        || (chunk_size > riff_end - chunk_data_position))
    {
      return FR_INVALID_OBJECT;
    }

    next_chunk_position = chunk_data_position + chunk_size + (chunk_size & 1U);
    if (next_chunk_position > riff_end)
    {
      return FR_INVALID_OBJECT;
    }

    if (memcmp(chunk_header, "fmt ", 4) == 0)
    {
      if (chunk_size < sizeof(format_data))
      {
        return FR_INVALID_OBJECT;
      }

      result = WAV_ReadExact(format_data, sizeof(format_data));
      if (result != FR_OK)
      {
        return result;
      }

      audio_format = WAV_ReadLE16(&format_data[0]);
      channels = WAV_ReadLE16(&format_data[2]);
      sample_rate = WAV_ReadLE32(&format_data[4]);
      bits_per_sample = WAV_ReadLE16(&format_data[14]);
      found_format = 1;
    }
    else if (memcmp(chunk_header, "data", 4) == 0)
    {
      *data_offset = chunk_data_position;
      *data_size = chunk_size;
      found_data = 1;
    }

    chunk_position = next_chunk_position;
  }

  if ((!found_format) || (!found_data))
  {
    return FR_INVALID_OBJECT;
  }

  if ((audio_format != WAV_PLAYER_FORMAT_PCM)
      || (channels != WAV_PLAYER_CHANNELS)
      || (sample_rate != WAV_PLAYER_SAMPLE_RATE)
      || (bits_per_sample != WAV_PLAYER_BITS_PER_SAMPLE)
      || ((*data_size % 4U) != 0U))
  {
    return FR_INVALID_PARAMETER;
  }

  return FR_OK;
}

/**
 * @brief  从 WAV 文件读取 PCM 数据并填充 DMA 缓冲区的一半。
 * @param  half DMA 半区编号，只能是 0 或 1。
 * @note   未使用的缓冲区尾部填零；本函数会更新剩余数据长度和最后有效半区。
 * @retval FR_OK             填充成功。
 * @retval FR_INVALID_OBJECT 文件数据长度异常或文件未按预期读满。
 * @retval 其他 FRESULT     FatFs 读取错误。
 */
static FRESULT WAV_FillHalf(uint8_t half)
{
  BYTE *buffer = (BYTE *)&s_dma_buffer[half * WAV_PLAYER_HALF_SAMPLES];
  UINT buffer_bytes = WAV_PLAYER_HALF_SAMPLES * sizeof(uint16_t);
  UINT bytes_to_read;
  UINT bytes_read = 0;
  FRESULT result;

  memset(buffer, 0, buffer_bytes);
  bytes_to_read = (s_data_remaining < buffer_bytes) ? s_data_remaining : buffer_bytes;
  bytes_to_read &= ~3U;

  if (bytes_to_read != 0U)
  {
    result = f_read(&s_wav_file, buffer, bytes_to_read, &bytes_read);
    if (result != FR_OK)
    {
      return result;
    }
    if ((bytes_read % 4U) != 0U)
    {
      return FR_INVALID_OBJECT;
    }

    s_data_remaining -= bytes_read;
    if (bytes_read != 0U)
    {
      s_last_data_half = half;
      s_has_audio_data = 1;
    }

    if (bytes_read < bytes_to_read)
    {
      s_data_remaining = 0;
    }
  }

  if ((s_data_remaining == 0U) && (s_stop_after_half < 0) && s_has_audio_data)
  {
    s_stop_after_half = (int8_t)s_last_data_half;
  }

  return FR_OK;
}

/**
 * @brief  打开 WAV 文件、解析格式、预填 DMA 缓冲区并启动 I2S DMA 播放。
 * @param  path SD 卡上的文件路径，例如 "0:/MUSIC.WAV"。
 * @note   调用前必须初始化 I2S2 并挂载 FatFs；新调用会先停止并关闭先前播放。
 *         支持格式为 PCM、双声道、16-bit、44100 Hz。
 * @retval FR_OK                 文件与格式有效，I2S DMA 已启动。
 * @retval FR_INVALID_PARAMETER  path 为空，或 WAV 格式不受支持。
 * @retval FR_INVALID_OBJECT     WAV 文件结构错误或没有音频数据。
 * @retval 其他 FRESULT          打开、定位或读取文件失败。
 * @retval FR_INT_ERR            I2S DMA 未能启动。
 */
FRESULT WAV_Player_Start(const char *path)
{
  FRESULT result;
  DWORD data_offset;
  DWORD data_size;

  if (path == NULL)
  {
    return FR_INVALID_PARAMETER;
  }

  WAV_Player_Stop();
  result = f_open(&s_wav_file, path, FA_READ);
  if (result != FR_OK)
  {
    s_result = result;
    return result;
  }
  s_file_open = 1;

  result = WAV_ParseHeader(&data_offset, &data_size);
  if (result != FR_OK)
  {
    WAV_Player_Stop();
    s_result = result;
    return result;
  }

  if (f_lseek(&s_wav_file, data_offset) != FR_OK)
  {
    WAV_Player_Stop();
    s_result = FR_DISK_ERR;
    return s_result;
  }

  s_data_remaining = data_size;
  s_stop_after_half = -1;
  s_has_audio_data = 0;
  s_completed_halves = 0;
  s_result = FR_OK;

  result = WAV_FillHalf(0);
  if (result == FR_OK)
  {
    result = WAV_FillHalf(1);
  }
  if ((result != FR_OK) || (!s_has_audio_data))
  {
    WAV_Player_Stop();
    s_result = (result == FR_OK) ? FR_INVALID_OBJECT : result;
    return s_result;
  }

  s_is_playing = 1;
  if (HAL_I2S_Transmit_DMA(&hi2s2, s_dma_buffer, WAV_PLAYER_BUFFER_SAMPLES) != HAL_OK)
  {
    WAV_Player_Stop();
    s_result = FR_INT_ERR;
    return s_result;
  }

  printf("WAV: playing %s, PCM stereo 16-bit 44100 Hz\r\n", path);
  return FR_OK;
}

/**
 * @brief  处理 DMA 完成事件并为已播放的半区补充下一段音频。
 * @note   必须在主循环或任务中频繁调用，不能在中断中调用，因为内部会访问 FatFs。
 *         如果调用不及时，DMA 可能在下一半数据准备好之前重复播放旧内容。
 * @retval 无；读取错误会停止播放，并可通过 WAV_Player_GetResult() 查询结果。
 */
void WAV_Player_Process(void)
{
  uint8_t completed_halves;
  uint32_t interrupt_state;
  uint8_t half;
  FRESULT result;

  interrupt_state = __get_PRIMASK();
  __disable_irq();
  completed_halves = s_completed_halves;
  s_completed_halves = 0;
  if (interrupt_state == 0U)
  {
    __enable_irq();
  }

  for (half = 0; half < 2U; ++half)
  {
    if ((completed_halves & (1U << half)) == 0U)
    {
      continue;
    }

    if ((s_stop_after_half >= 0) && (s_stop_after_half == (int8_t)half))
    {
      WAV_Player_Stop();
      s_result = FR_OK;
      printf("WAV: playback complete\r\n");
      return;
    }

    result = WAV_FillHalf(half);
    if (result != FR_OK)
    {
      WAV_Player_Stop();
      s_result = result;
      printf("WAV: read error=%d\r\n", result);
      return;
    }
  }
}

/**
 * @brief  停止当前 I2S DMA 播放并关闭已打开的 WAV 文件。
 * @note   如果当前没有播放或文件已关闭，则对应资源清理步骤会跳过。
 * @retval 无。
 */
void WAV_Player_Stop(void)
{
  if (s_is_playing)
  {
    (void)HAL_I2S_DMAStop(&hi2s2);
    s_is_playing = 0;
  }

  s_completed_halves = 0;
  if (s_file_open)
  {
    (void)f_close(&s_wav_file);
    s_file_open = 0;
  }
}

/**
 * @brief  查询播放器当前是否正在传输音频。
 * @retval 非 0 正在播放；0 已停止或尚未开始。
 */
int WAV_Player_IsPlaying(void)
{
  return (s_is_playing != 0U);
}

/**
 * @brief  查询最近一次启动或播放处理的结果码。
 * @note   手动调用 WAV_Player_Stop() 不会修改此结果码。
 * @retval 最近记录的 FatFs FRESULT；I2S DMA 启动失败时为 FR_INT_ERR。
 */
FRESULT WAV_Player_GetResult(void)
{
  return s_result;
}

/**
 * @brief  HAL I2S DMA 半传输完成回调。
 * @param  hi2s 产生回调的 I2S 句柄。
 * @note   只记录 DMA 的前半缓冲已播放，不在中断中访问 FatFs。
 * @retval 无。
 */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if ((hi2s->Instance == SPI2) && s_is_playing)
  {
    s_completed_halves |= 0x01U;
  }
}

/**
 * @brief  HAL I2S DMA 全传输完成回调。
 * @param  hi2s 产生回调的 I2S 句柄。
 * @note   循环 DMA 模式下表示后半缓冲已播放；只记录事件，不访问 FatFs。
 * @retval 无。
 */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if ((hi2s->Instance == SPI2) && s_is_playing)
  {
    s_completed_halves |= 0x02U;
  }
}