#include <stdlib.h>
#include <string.h>
#include "EPD_4in2_V2.h"
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
        display_red((count >= 2) ? command[1] : "", (count == 3) ? command[2] : ""); 
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

UBYTE *BlackImage;
UWORD Imagesize = ((EPD_4IN2_V2_WIDTH % 8 == 0)? (EPD_4IN2_V2_WIDTH / 8 ): (EPD_4IN2_V2_WIDTH / 8 + 1)) * EPD_4IN2_V2_HEIGHT;

int display_init()
{
    if (DEV_Module_Init())
    {
        return -1;
    }

    EPD_4IN2_V2_Init();
    EPD_4IN2_V2_Clear();
    DEV_Delay_ms(500);
    
    //Create a new image cache

    if ((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL)
    {
        return -1;
    }

    Paint_NewImage(BlackImage, EPD_4IN2_V2_WIDTH, EPD_4IN2_V2_HEIGHT, 0, WHITE);
    
    return 0;
}

void display_exit()
{
    EPD_4IN2_V2_Init();
    EPD_4IN2_V2_Sleep();
    free(BlackImage);
    BlackImage = NULL;
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
    //EPD_4IN2_V2_Init();
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    Paint_DrawString_EN(10, 10, "green", &Font24, WHITE, BLACK);

    // EPD_4IN2_V2_Display(BlackImage);
	EPD_4IN2_V2_Display(BlackImage);
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

    //1.Select Image
    //EPD_4IN2_V2_Init();
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);

    // 2.Drawing on the image
    Paint_DrawString_EN(10, 10, "red", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(10, 38, str1, &Font24, WHITE, BLACK);
    Paint_DrawString_EN(10, 66, str2, &Font24, WHITE, BLACK);

    // EPD_4IN2_V2_Display(BlackImage);
	EPD_4IN2_V2_Display(BlackImage);
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