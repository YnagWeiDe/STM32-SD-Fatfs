#include "beep.h"

int melody[] = {50, 50, 50, 50, 200, 200, 200, 400, 400, 500, 500, 500};

// 宏定义同步修改
#define BEEP_PIN PBout(5)

void BEEP_Init(void)
{   
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	 //使能A端口时钟
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;	 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化GPIOD3,6
    GPIO_SetBits(GPIOB,GPIO_Pin_5);	
}

void Beep_PlayFrequency(uint16_t frequency)
{
    uint32_t half_period_us;
    if(frequency != 1000)
    {
        half_period_us = 500000U / frequency;
        BEEP_PIN = 1;
        delay_us(half_period_us);
        BEEP_PIN = 0;
        delay_us(half_period_us);
    }
    else
    {
        delay_us(1000);
    }
}

void Beep_PlayDuration(uint16_t duration_ms)
{
    BEEP_PIN = 1;
    delay_ms(duration_ms);
    BEEP_PIN = 0;
    delay_ms(duration_ms);
}

void Beep_PlaySuccess(void)
{
    int i;
    for(i = 0; i < 12; i++)
    {
        Beep_PlayDuration(melody[i]);
    }
}

void Beep_PlayFailure(void)
{
    int i;
    for(i = 11; i >= 0; i--)
    {
        Beep_PlayDuration(melody[i]);
    }
}

void play_music(void)
{
    //              低7  1   2   3   4   5   6   7  高1 高2 高3 高4 高5 静音
    const uint16_t tone_table[] = {247,262,294,330,349,392,440,494,523,587,659,698,784,1000};
    // 红尘情歌音符序列（每个值对应 tone_table[] 的下标）
    const uint8_t note_sequence[] = {
        5,5,6,8,7,6,5,6,13,13,
        5,5,6,8,7,6,5,3,13,13,
        2,2,3,5,3,5,6,3,2,1,
        6,6,5,6,5,3,6,5,13,13,

        5,5,6,8,7,6,5,6,13,13,
        5,5,6,8,7,6,5,3,13,13,
        2,2,3,5,3,5,6,3,2,1,
        6,6,5,6,5,3,6,1,	

        13,8,9,10,10,9,8,10,9,8,6,
        13,6,8,9,9,8,6,9,8,6,5,
        13,2,3,5,5,3,5,5,6,8,7,6,
        6,10,9,9,8,6,5,6,8
    };
    // 每个音符的节拍长度
    const uint8_t note_duration[] = {
        2,4,2,2,2,2,2,8,4,4,
        2,4,2,2,2,2,2,8,4,4,
        2,4,2,4,2,2,4,2,2,8,
        2,4,2,2,2,2,2,8,4,4,

        2,4,2,2,2,2,2,8,4,4,
        2,4,2,2,2,2,2,8,4,4,
        2,4,2,4,2,2,4,2,2,8,
        2,4,2,2,2,2,2,8,

        4,2,2,2,4,2,2,2,2,2,8,
        4,2,2,2,4,2,2,2,2,2,8,
        4,2,2,2,4,2,2,5,2,6,2,4,
        2,2,2,4,2,4,2,2,12
    };

    const uint32_t delay_factor = 10; // 控制整体播放速度的延时因子
    const uint16_t note_count = sizeof(note_sequence) / sizeof(note_sequence[0]);
    uint16_t note_idx, repeat_idx;

    for(note_idx = 0; note_idx < note_count; note_idx++) {
        uint8_t tone_idx = note_sequence[note_idx];
        if (tone_idx >= sizeof(tone_table) / sizeof(tone_table[0])) {
            continue;
        }
        uint16_t freq = tone_table[tone_idx];
        uint16_t duration = note_duration[note_idx];

        uint32_t repeat_count = ((uint32_t)duration) * freq / delay_factor;
        for(repeat_idx = 0; repeat_idx < repeat_count; repeat_idx++) {
            Beep_PlayFrequency(freq);
        }
    }
}

