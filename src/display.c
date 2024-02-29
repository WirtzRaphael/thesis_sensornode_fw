// project files
#include "display.h"

// McuLib
#include "McuUtility.h"
#include "McuSSD1306.h"
#include "McuGDisplaySSD1306.h"
#include "McuFontDisplay.h"
#include "McuGFont.h"
#include "McuFontCour10Normal.h"
#include "McuFontHelv12Normal.h"


void display_init(void) {
    McuSSD1306_Init();
    McuGDisplaySSD1306_Init();
}


/*!
 * \brief Shows one line of text centered on the display, with a border around it
 * \param text0 Line one of text
 * author : erich styger
 */
void Show1Liner(const unsigned char *text0) {
  PGFONT_Callbacks font = McuFontHelv12Normal_GetFont();
  McuFontDisplay_PixelDim x, y;
  McuFontDisplay_PixelDim charHeight, totalHeight;

  McuGDisplaySSD1306_Clear();
  McuFontDisplay_GetFontHeight(font, &charHeight, &totalHeight);

  x = McuGDisplaySSD1306_GetWidth()/2 - McuFontDisplay_GetStringWidth((unsigned char*)text0, font, NULL)/2;
  y = McuGDisplaySSD1306_GetHeight()/2 - charHeight/2; /* 1 line */
  McuFontDisplay_WriteString((unsigned char*)text0, McuGDisplaySSD1306_COLOR_BLUE, &x, &y, font);

  McuGDisplaySSD1306_DrawBox(0, 0, McuGDisplaySSD1306_GetWidth()-1, McuGDisplaySSD1306_GetHeight()-1, 1, McuGDisplaySSD1306_COLOR_BLUE);
  McuGDisplaySSD1306_DrawBox(2, 2, McuGDisplaySSD1306_GetWidth()-1-4, McuGDisplaySSD1306_GetHeight()-1-4, 1, McuGDisplaySSD1306_COLOR_BLUE);
  McuGDisplaySSD1306_UpdateFull();
}

/*!
 * \brief Shows two lines of text centered on the display, with a border around it
 * \param text0 Line one of text
 * \param text1 Line two of text
 *  author : erich styger
 */
void Show2Liner(const unsigned char *text0, const unsigned char *text1) {
  PGFONT_Callbacks font = McuFontHelv12Normal_GetFont();
  McuFontDisplay_PixelDim x, y;
  McuFontDisplay_PixelDim charHeight, totalHeight;

  McuGDisplaySSD1306_Clear();
  McuFontDisplay_GetFontHeight(font, &charHeight, &totalHeight);

  x = McuGDisplaySSD1306_GetWidth()/2 - McuFontDisplay_GetStringWidth((unsigned char*)text0, font, NULL)/2;
  y = McuGDisplaySSD1306_GetHeight()/2 - (2*charHeight)/2; /* 2 lines */
  McuFontDisplay_WriteString((unsigned char*)text0, McuGDisplaySSD1306_COLOR_BLUE, &x, &y, font);

  x = McuGDisplaySSD1306_GetWidth()/2 - McuFontDisplay_GetStringWidth((unsigned char*)text1, font, NULL)/2;
  y += McuGDisplaySSD1306_GetHeight()/2 - (2*charHeight)/2;
  McuFontDisplay_WriteString((unsigned char*)text1, McuGDisplaySSD1306_COLOR_BLUE, &x, &y, font);

  McuGDisplaySSD1306_DrawBox(0, 0, McuGDisplaySSD1306_GetWidth()-1, McuGDisplaySSD1306_GetHeight()-1, 1, McuGDisplaySSD1306_COLOR_BLUE);
  McuGDisplaySSD1306_DrawBox(2, 2, McuGDisplaySSD1306_GetWidth()-1-4, McuGDisplaySSD1306_GetHeight()-1-4, 1, McuGDisplaySSD1306_COLOR_BLUE);
  McuGDisplaySSD1306_UpdateFull();
}

void display_status_float(float temperature1, float temperature2) {
    unsigned char buf1[16], buf2[16], buf_tmp[16];

    McuUtility_strcpy(buf1, sizeof(buf1), (unsigned char*)"T1: ");
    McuUtility_NumFloatToStr(buf_tmp, sizeof(buf_tmp), temperature1, 2);
    McuUtility_strcat(buf1, sizeof(buf1), buf_tmp);
    McuUtility_strcat(buf1, sizeof(buf1), (unsigned char*)"C");
    
    McuUtility_strcpy(buf2, sizeof(buf2), (unsigned char*)"T2: ");
    McuUtility_NumFloatToStr(buf_tmp, sizeof(buf_tmp), temperature2, 2);
    McuUtility_strcat(buf2, sizeof(buf2), buf_tmp);
    McuUtility_strcat(buf2, sizeof(buf2), (unsigned char*)"C");

    Show2Liner(buf1, buf2);
}

void display_status_uint16(uint16_t sensor1, uint16_t sensor2) {
    unsigned char buf1[16], buf2[16], buf_tmp[16];

    McuUtility_strcpy(buf1, sizeof(buf1), (unsigned char*)"T1: ");
    McuUtility_Num16uToStr(buf_tmp, sizeof(buf_tmp), sensor1);
    McuUtility_strcat(buf1, sizeof(buf1), buf_tmp);
    McuUtility_strcat(buf1, sizeof(buf1), (unsigned char*)"");
    
    McuUtility_strcpy(buf2, sizeof(buf2), (unsigned char*)"T2: ");
    McuUtility_Num16uToStr(buf_tmp, sizeof(buf_tmp), sensor2);
    McuUtility_strcat(buf2, sizeof(buf2), buf_tmp);
    McuUtility_strcat(buf2, sizeof(buf2), (unsigned char*)"");

    Show2Liner(buf1, buf2);
}