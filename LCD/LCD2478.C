
#include  "LPC2400.h"
#include  "LCD2478.h"

#if COLOR_NUMBER == 16
const uint16 LCD_Palette_16[16]=						//����16ɫLCD��ɫ��
{	//---��(R)-- | ---��(G)--- | ---��(B)---
	(0x00 << 10) | (0x00 << 5) | (0x00 << 0),			//0�� ��ɫ
	(0x00 << 10) | (0x00 << 5) | (0x1f << 0),			//1�� ����
	(0x00 << 10) | (0x1f << 5) | (0x00 << 0),			//2�� ����
	(0x00 << 10) | (0x1f << 5) | (0x1f << 0),			//3�� ����
	(0x1f << 10) | (0x00 << 5) | (0x00 << 0),			//4�� ���
	(0x14 << 10) | (0x04 << 5) | (0x1e << 0),			//5�� ����
	(0x1f << 10) | (0x1f << 5) | (0x00 << 0),			//6�� ���
	(0x18 << 10) | (0x18 << 5) | (0x18 << 0),			//7�� ǳ��
	(0x10 << 10) | (0x11 << 5) | (0x10 << 0),			//8�� ���
	(0x06 << 10) | (0x12 << 5) | (0x14 << 0),			//9�� ǳ��
	(0x06 << 10) | (0x19 << 5) | (0x06 << 0),			//10�� ǳ��
	(0x08 << 10) | (0x1c << 5) | (0x1a << 0),			//11�� ǳ��
	(0x1f << 10) | (0x0c << 5) | (0x08 << 0),			//12�� ǳ��
	(0x14 << 10) | (0x0c << 5) | (0x1a << 0),			//13�� ǳ��
	(0x1e << 10) | (0x1c << 5) | (0x11 << 0),			//14�� ǳ��
	(0x1f << 10) | (0x1F << 5) | (0x1f << 0),			//15�� ��ɫ
};

__align(8) uint32 disp_buffer[LINE_SIZE*SCR_YSIZE];		//LCD��ʾ������
#else
//65536-color mode
__align(8) uint16 disp_buffer[LINE_SIZE*SCR_YSIZE];		//LCD��ʾ������
#endif

void  DelaynS(uint32  uiDly)
{  
    uint32  i;
    for(; uiDly > 0; uiDly--){
        for(i = 0; i < 2000; i++);
    }
}
/*********************************************************************************************************
** Function name :        lcd_Init
** Function Description : Initialize the LCD controler
** Input:                 none
** Output:                none
*********************************************************************************************************/
void Init_LCD2478 (void)
{
#if COLOR_NUMBER == 16
//16-color mode
uint32 i,*lcdpal;
#endif
//��������ʾ�йصĶ˿�
	PCONP |= 1 << 20;						//enable LCD controller power control bit
	PINSEL3  &= 0xF00000FF; 
	PINSEL3  |= 0x05555500;	//config lcd pin for TFT mode
	PINSEL4  &= 0xFFF00000; 
	PINSEL4  |= 0x000FFFF0;	//config lcd pin for TFT mode
	PINSEL10 = 0;									//disable ETM
	PINSEL11 = (4 << 1) |							//��ΪLCD��EXTINT�ܽŸ���
	                                                //����ʹ��444ģʽ���Ա�ʹ��EINT2�����ⲿ�ж�
			   (1 << 0);							//lcd port is enable
//��ʼ��Ƭ����ʾϵͳ
	DelaynS(5);
	LCD_UPBASE = (uint32)disp_buffer;	
	
	LCD_CTRL = 0;									//power disable
	DelaynS(10);
	LCD_CFG = LCD_CLKDIV;							//config pixel clock
	LCD_TIMH = ((LCD_HBP+5) << 24) |					//set horizontal timing
			   ((LCD_HFP) << 16) |						//HBPǰ�������������С�ˣ���ǰ�����ݲ���ʾ
			   (LCD_HSW << 8) |							//ʵ������Ч��,�����
			   (LCD_PPL << 2);							//HFP ��������ԭ��ͬ��
	LCD_TIMV = ((LCD_VBP-6) << 24) |					//set vertical timing
			   ((LCD_VFP) << 16) |
			   (LCD_VSW << 10) |
			   (LCD_LPP << 0);	
/*	LCD_TIMH = (45 << 24) |					//set horizontal timing
			   ((1056-800-46) << 16) |
			   (1 << 8) |
			   (49 << 2);
	LCD_TIMV = (22 << 24) |					//set vertical timing
			   ((635-23-480) << 16) |
			   (1 << 10) |
			   (29 << 0);	  */
	LCD_POL = (LCD_BCD << 26) |						//bypass pixel color divide
			  (LCD_CPL << 16) |						//clock per line
			  (LCD_IOE << 14) |						//LCDENAB is active high
			  (LCD_IPC << 13) |						//LCDDCLK is active falling edge
			  (LCD_IHS << 12) |						//HSYNC is active low
			  (LCD_IVS << 11) |						//VSYNC is active low
			  (LCD_CLKSEL << 5) |					//select HCLK
			  (LCD_PCD << 0);						//set PCD clk divided
	LCD_CTRL = (LCD_WATERMARK << 16) |				//water mark
			   (LCD_BEPO << 10) |					//pixel big-endian
			   (LCD_BEBO << 9) |					//data big-endian
			   (LCD_BGR << 8) |						//red and blue swapped
			   (LCD_TFT << 5) |						//select TFT LCD type
			   (LCD_BPP << 1) |						//select bpp number
			   (LCD_EN << 0);						//LCD enable
	LCD_INTMSK = 0;									//disable LCD interrupt
	CRSR_CTRL = 0;									//disable cursor
#if COLOR_NUMBER == 16
//16-color mode
	lcdpal = (uint32 *)(&LCD_PAL);
	for(i=0; i<8; i++)								//set LCD palette
	{
		*lcdpal++ = (LCD_Palette_16[i*2+0] << 16) + LCD_Palette_16[i*2+1];
	}
#endif
	DelaynS(10);
	LCD_CTRL |= LCD_POWER << 11;					//Power enable
	DelaynS(10);
}    
           
/*********************************************************************************************************
* End File
**********************************************************************/