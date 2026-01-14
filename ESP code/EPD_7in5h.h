/*****************************************************************************
* | File      	:   EPD_7in5h.h
* | Author      :   Waveshare team
* | Function    :   7.5inch e-paper (G)
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-08-07
* | Info        :
* -----------------------------------------------------------------------------
******************************************************************************/
#ifndef __EPD_7IN5H_H_
#define __EPD_7IN5H_H_

#include "DEV_Config.h"


// Commands

/*This register indicates that the user to start to transmit data
then write to SRAM. While data transmission is complete. Send Display refresh once transfer complete. */
#define DATA_START_TRANSMISSION_COMMAND 0x10

/* This command refreshes the display based on SRAM data and LUT.
After display refresh command, the BUSY_N signal becomes 0.
This command only activates when BUSY_N = 1 */
#define DISPLAY_REFRESH_COMMAND 0x12

// Display resolution
#define EPD_7IN5H_WIDTH       800
#define EPD_7IN5H_HEIGHT      480

// Color
#define  EPD_7IN5H_BLACK   0x0
#define  EPD_7IN5H_WHITE   0x1
#define  EPD_7IN5H_YELLOW  0x2
#define  EPD_7IN5H_RED     0x3

void EPD_7IN5H_Init(void);
void EPD_7IN5H_Clear(UBYTE color);
void EPD_7IN5H_Display(const UBYTE *Image);
void EPD_7IN5H_WriteSliceToRam(const UBYTE *Image, UWORD xstart, UWORD ystart, UWORD image_width, UWORD image_heigh);
void EPD_7IN5H_Sleep(void);
void EPD_7IN5H_TurnOnDisplay(void);
void EPD_7IN5H_SendCommand(UBYTE Reg);
void EPD_7IN5H_SendData(UBYTE Data);

#endif
