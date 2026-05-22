#include <stdlib.h>
#include <string.h>
#include "EPD_4in2b_V2.h"
#include "GUI_Paint.h"
#include "fonts.h"
#include "config.h"

int display_1(int mode, char *str1);
int display_reset();

int display(int count, char command[COMMAND_MAX_WORD_COUNT][COMMAND_MAX_WORD_SIZE])
{
    if (count == 0)
    {
        return -1;
    }

    if (!strcmp(command[0], "freeup"))
    {
        return display_1(0, (count == 2) ? command[1] : NULL); 
    }

    if (!strcmp(command[0], "reserve"))
    {
        return display_1(1, (count == 2) ? command[1] : NULL); 
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

int display_reset()
{
    if (display_init())
    {
        return -1;
    }

    display_exit();

    return 0;
}

int display_1(int mode, char *str1)
{
    int start_x = 20;
    
    int str1_len, str1_start_x;

    if (display_init())
    {
        return -1;
    }
    
    if (mode)
    {
        Paint_SelectImage(RYImage);
        start_x = 83;
    }
    else
    {
        Paint_SelectImage(BlackImage);
        start_x = 102;
    }
    
    Paint_DrawRectangle(20, 20, EPD_4IN2B_V2_WIDTH - 20, 129, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawString_EN(start_x, 40, (mode) ? "ZAJETE" : "WOLNE", &Adwaita64, BLACK, WHITE);

    if (str1 != NULL)
    {
        str1[21] = '\0'; // only 21 characters of Font24 fit across the screen width (with 20px padding)
        str1_len = strlen(str1) * 17; // 17 is width of one letter of font24
        str1_start_x = (EPD_4IN2B_V2_WIDTH - str1_len) / 2;

        Paint_SelectImage(BlackImage);
        Paint_DrawString_EN(str1_start_x, 149, str1, &Font24, WHITE, BLACK);
    }

	EPD_4IN2B_V2_Display(BlackImage, RYImage);
	DEV_Delay_ms(2000);
    
    display_exit();

    return 0;
}