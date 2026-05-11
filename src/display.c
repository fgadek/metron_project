#include <stdlib.h>
#include <string.h>
#include "EPD_4in2b_V2.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "config.h"

int display_green();
int display_red(char *str1, char *str2);
int display_reset();

int display(int count, char command[COMMAND_MAX_WORD_COUNT][COMMAND_MAX_WORD_SIZE])
{
    if (count == 0)
    {
        return -1;
    }

    if (!strcmp(command[0], "green"))
    {
        return display_green();
    }

    if (!strcmp(command[0], "red"))
    {
        display_red((count >= 2) ? command[1] : NULL, (count == 3) ? command[2] : NULL); 
        return 0;
    }

    if (!strcmp(command[0], "reset"))
    {
        return display_reset(); 
    }
    else
    {
        return -1;
    }
}

UBYTE *BlackImage, *RYImage;
UWORD Imagesize = ((EPD_4IN2B_V2_WIDTH % 8 == 0)? (EPD_4IN2B_V2_WIDTH / 8 ): (EPD_4IN2B_V2_WIDTH / 8 + 1)) * EPD_4IN2B_V2_HEIGHT;

int display_init()
{
    if (DEV_Module_Init())
    {
        return -1;
    }

    EPD_4IN2B_V2_Init();
    EPD_4IN2B_V2_Clear();
    DEV_Delay_ms(500);
    
    //Create a new image cache

    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        return -1;
    }
    if ((RYImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        return -1;
    }

    Paint_NewImage(BlackImage, EPD_4IN2B_V2_WIDTH, EPD_4IN2B_V2_HEIGHT, 0, WHITE);
    Paint_NewImage(RYImage, EPD_4IN2B_V2_WIDTH, EPD_4IN2B_V2_HEIGHT, 0, WHITE);
    
    Paint_SelectImage(RYImage);
    Paint_Clear(WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    return 0;
}

void display_exit()
{
    EPD_4IN2B_V2_Sleep();

    free(BlackImage);
    free(RYImage);

    BlackImage = NULL;
    RYImage = NULL;

    DEV_Delay_ms(2000); // important, at least 2s
    DEV_Module_Exit(); 
}

int display_green()
{
    if (display_init())
    {
        return -1;
    }

    //1.Select Image
    Paint_SelectImage(BlackImage);
    
    for(int i = 0; i < 15; i++) {
        Paint_DrawRectangle(i, i, EPD_4IN2B_V2_WIDTH - i, EPD_4IN2B_V2_HEIGHT - i, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }

    int S = 10;         
    int startX = 55;    // Obliczony środek dla 5 liter przy S=12
    int startY = 100;
    int gap = 10;
    

    // W
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S*4, startY, startX+S*5, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S*2, startY+S*2, startX+S*3, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);
    startX += (S*5 + gap);

    // O
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S*4, startY, startX+S*5, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);
    startX += (S*5 + gap);

    // L
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);
    startX += (S*5 + gap);

    // N
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S*4, startY, startX+S*5, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S, startY+S, startX+S*2, startY+S*2, 0, 1, 1);
    Paint_DrawRectangle(startX+S*2, startY+S*2, startX+S*3, startY+S*3, 0, 1, 1);
    Paint_DrawRectangle(startX+S*3, startY+S*3, startX+S*4, startY+S*4, 0, 1, 1);
    startX += (S*5 + gap);

    // E
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*2, startX+S*4, startY+S*3, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);

    EPD_4IN2B_V2_Display(BlackImage, RYImage);
	DEV_Delay_ms(2000);
    
    display_exit();
    
    return 0;
}

int display_red(char *str1, char *str2)
{
    if (display_init())
    {
        return -1;
    }
    
    Paint_SelectImage(RYImage);

    for(int i = 0; i < 15; i++) {
        Paint_DrawRectangle(i, i, EPD_4IN2B_V2_WIDTH - i, EPD_4IN2B_V2_HEIGHT - i, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    }

    //1.Select Image
    Paint_SelectImage(BlackImage);
    
    int S = 10;         
    int startX = 25;
    int startY = 100;
    int gap = 10;

    // 2.Drawing on the image
    
    // Z
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX+S*3, startY+S, startX+S*4, startY+S*2, 0, 1, 1);
    Paint_DrawRectangle(startX+S*2, startY+S*2, startX+S*3, startY+S*3, 0, 1, 1);
    Paint_DrawRectangle(startX+S, startY+S*3, startX+S*2, startY+S*4, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);
    startX += (S*5 + gap);

    // A
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S*4, startY, startX+S*5, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*2, startX+S*5, startY+S*3, 0, 1, 1);
    startX += (S*5 + gap);

    // J
    Paint_DrawRectangle(startX+S*3, startY, startX+S*4, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*4, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*3, startX+S, startY+S*4, 0, 1, 1);
    startX += (S*5 + gap);

    // E
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*2, startX+S*4, startY+S*3, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX+S*2, startY+S*5, startX+S*3, startY+S*6, 0, 1, 1);
    Paint_DrawRectangle(startX+S*3, startY+S*6, startX+S*4, startY+S*7, 0, 1, 1);
    startX += (S*5 + gap);

    // T
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX+S*2, startY, startX+S*3, startY+S*5, 0, 1, 1);
    startX += (S*5 + gap);

    // E
    Paint_DrawRectangle(startX, startY, startX+S, startY+S*5, 0, 1, 1);
    Paint_DrawRectangle(startX, startY, startX+S*5, startY+S, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*2, startX+S*4, startY+S*3, 0, 1, 1);
    Paint_DrawRectangle(startX, startY+S*4, startX+S*5, startY+S*5, 0, 1, 1);
    
    int str1_len, str2_len;
    int str1_startX, str2_startX;

    if (str1 != NULL)
    {
        str1[STR1_MAX_LENGTH] = '\0';
        str1_len = strlen(str1) * 17; // 17 is width of one letter of font24
        str1_startX = (EPD_4IN2B_V2_WIDTH - str1_len) / 2;
    }
    if (str2 != NULL)
    {
        str1[STR2_MAX_LENGTH] = '\0';
        str2_len = strlen(str2) * 14; // 14 is width of one letter of font20
        str2_startX = (EPD_4IN2B_V2_WIDTH - str2_len) / 2;
    }

    Paint_DrawString_EN(str1_startX, 185+5, str1, &Font24, WHITE, BLACK);
    Paint_DrawString_EN(str2_startX, 224+5, str2, &Font20, WHITE, BLACK);

    // EPD_4IN2_V2_Display(BlackImage);
	EPD_4IN2B_V2_Display(BlackImage, RYImage);
	DEV_Delay_ms(2000);
    
    display_exit();

    return 0;
}

int display_reset()
{
    if (display_init())
    {
        return -1;
    }

    display_exit();

    return 0;
}