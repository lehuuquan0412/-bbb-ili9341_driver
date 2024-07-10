
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "ili9341_user_lib.h"
#include "ili9341.h"

#define HIGH_16(x)     					((((uint16_t)x) >> 8U) & 0xFFU)
#define LOW_16(x)      					((((uint16_t)x) >> 0U) & 0xFFU)

bsp_lcd_t lcd_handle;
bsp_lcd_t *hlcd = &lcd_handle;

#define DB_SIZE 					(10UL * 1024UL)
uint8_t bsp_db[DB_SIZE];
uint8_t bsp_wb[DB_SIZE];

static int spi_write(uint8_t MODE, uint8_t *data, uint32_t len)
{
    uint8_t data_to_send[len + 1];
    int fd;
    int ret;

    data_to_send[0] = MODE;

    for (uint32_t i = 0; i < len; i++)
    {
        data_to_send[i + 1] = data[i];
    }

    fd = open(DEVICE_PATH, O_RDWR);

    ret = write(fd, data_to_send, len + 1);

    close(fd);

    return ret;
}

int lcd_write_data(uint8_t *data, uint32_t size)
{
    return spi_write(DATA_MODE, data, size);
}

int lcd_write_cmd(uint8_t cmd)
{
    return spi_write(CMD_MODE, &cmd, 1);
}

void lcd_config(void)
{
	uint8_t params[15];
	lcd_write_cmd(ILI9341_SWRESET);
	lcd_write_cmd(ILI9341_POWERB);
	params[0] = 0x00;
	params[1] = 0xD9;
	params[2] = 0x30;
	lcd_write_data(params, 3);

	lcd_write_cmd(ILI9341_POWER_SEQ);
	params[0]= 0x64;
	params[1]= 0x03;
	params[2]= 0X12;
	params[3]= 0X81;
	lcd_write_data(params, 4);

	lcd_write_cmd(ILI9341_DTCA);
	params[0]= 0x85;
	params[1]= 0x10;
	params[2]= 0x7A;
	lcd_write_data(params, 3);

	lcd_write_cmd(ILI9341_POWERA);
	params[0]= 0x39;
	params[1]= 0x2C;
	params[2]= 0x00;
	params[3]= 0x34;
	params[4]= 0x02;
	lcd_write_data(params, 5);

	lcd_write_cmd(ILI9341_PRC);
	params[0]= 0x20;
	lcd_write_data(params, 1);

	lcd_write_cmd(ILI9341_DTCB);
	params[0]= 0x00;
	params[1]= 0x00;
	lcd_write_data(params, 2);

	lcd_write_cmd(ILI9341_POWER1);
	params[0]= 0x1B;
	lcd_write_data(params, 1);

	lcd_write_cmd(ILI9341_POWER2);
	params[0]= 0x12;
	lcd_write_data(params, 1);

	lcd_write_cmd(ILI9341_VCOM1);
	params[0]= 0x08;
	params[1]= 0x26;
	lcd_write_data(params, 2);

	lcd_write_cmd(ILI9341_VCOM2);
	params[0]= 0XB7;
	lcd_write_data(params, 1);


	lcd_write_cmd(ILI9341_PIXEL_FORMAT);
	params[0]= 0x55; //select RGB565
	lcd_write_data(params, 1);

	lcd_write_cmd(ILI9341_FRMCTR1);
	params[0]= 0x00;
	params[1]= 0x1B;//frame rate = 70
	lcd_write_data(params, 2);

	lcd_write_cmd(ILI9341_DFC);    // Display Function Control
	params[0]= 0x0A;
	params[1]= 0xA2;
	lcd_write_data(params, 2);

	lcd_write_cmd(ILI9341_3GAMMA_EN);    // 3Gamma Function Disable
	params[0]= 0x02; //LCD_WR_DATA(0x00);
	lcd_write_data(params, 1);

	lcd_write_cmd(ILI9341_GAMMA);
	params[0]= 0x01;
	lcd_write_data(params, 1);

	lcd_write_cmd(ILI9341_PGAMMA);    //Set Gamma
	params[0]= 0x0F;
	params[1]= 0x1D;
	params[2]= 0x1A;
	params[3]= 0x0A;
	params[4]= 0x0D;
	params[5]= 0x07;
	params[6]= 0x49;
	params[7]= 0X66;
	params[8]= 0x3B;
	params[9]= 0x07;
	params[10]= 0x11;
	params[11]= 0x01;
	params[12]= 0x09;
	params[13]= 0x05;
	params[14]= 0x04;
	lcd_write_data(params, 15);

	lcd_write_cmd(ILI9341_NGAMMA);
	params[0]= 0x00;
	params[1]= 0x18;
	params[2]= 0x1D;
	params[3]= 0x02;
	params[4]= 0x0F;
	params[5]= 0x04;
	params[6]= 0x36;
	params[7]= 0x13;
	params[8]= 0x4C;
	params[9]= 0x07;
	params[10]= 0x13;
	params[11]= 0x0F;
	params[12]= 0x2E;
	params[13]= 0x2F;
	params[14]= 0x05;
	lcd_write_data(params, 15);

	lcd_write_cmd(ILI9341_SLEEP_OUT); //Exit Sleep
	delay_50ms();
	delay_50ms();
	lcd_write_cmd(ILI9341_DISPLAY_ON); //display on
}

void lcd_set_display_area(lcd_area_t *area)
{
	uint8_t params[4];
	/*Column address set(2Ah) */
	params[0] = HIGH_16(area->x1);
	params[1] = LOW_16(area->x1);
	params[2] = HIGH_16(area->x2);
	params[3] = LOW_16(area->x2);
	lcd_write_cmd(ILI9341_CASET);
	lcd_write_data(params, 4);

	params[0] = HIGH_16(area->y1);
	params[1] = LOW_16(area->y1);
	params[2] = HIGH_16(area->y2);
	params[3] = LOW_16(area->y2);
	lcd_write_cmd(ILI9341_RASET);
	lcd_write_data(params, 4);
}

void lcd_set_orientation(uint8_t orientation)
{
	uint8_t param;

	if(orientation == LANDSCAPE){
		param = MADCTL_MV | MADCTL_MY | MADCTL_BGR; /*Memory Access Control <Landscape setting>*/
	}else if(orientation == PORTRAIT){
		param = MADCTL_MY| MADCTL_MX| MADCTL_BGR;  /* Memory Access Control <portrait setting> */
	}

	lcd_write_cmd(ILI9341_MAC);    // Memory Access Control command
	lcd_write_data(&param, 1);
}

void lcd_buffer_init(bsp_lcd_t *lcd)
{
	lcd->draw_buffer1 = bsp_db;
	lcd->draw_buffer2 = bsp_wb;
	lcd->buff_to_draw = NULL;
	lcd->buff_to_flush = NULL;
}

uint32_t get_total_bytes(bsp_lcd_t *hlcd,uint32_t w , uint32_t h)
{
	uint8_t bytes_per_pixel = 2;
	if(hlcd->pixel_format == BSP_LCD_PIXEL_FMT_RGB565){
		bytes_per_pixel = 2;
	}
	return (w * h * bytes_per_pixel);
}

uint16_t convert_rgb888_to_rgb565(uint32_t rgb888)
{
    uint16_t r,g,b;
	r = (rgb888 >> 19) & 0x1FU;
	g = (rgb888 >> 10) & 0x3FU;
	b = (rgb888 >> 3)  & 0x1FU;
	return (uint16_t)((r << 11) | (g << 5) | b);
}

void make_area(lcd_area_t *area,uint32_t x_start, uint32_t x_width,uint32_t y_start,uint32_t y_height){

	uint16_t lcd_total_width,lcd_total_height;

	lcd_total_width =  BSP_LCD_ACTIVE_WIDTH - 1;
	lcd_total_height = BSP_LCD_ACTIVE_HEIGHT - 1;

	area->x1 = x_start;
	area->x2 = x_start + x_width -1;
	area->y1 = y_start;
	area->y2 = y_start + y_height -1;

	area->x2 = (area->x2 > lcd_total_width) ? lcd_total_width :area->x2;
	area->y2 = (area->y2 > lcd_total_height) ? lcd_total_height : area->y2;
}

uint32_t bytes_to_pixels(uint32_t nbytes, uint8_t pixel_format)
{
	(void)(pixel_format);
	return nbytes/2;
}

uint32_t pixels_to_bytes(uint32_t pixels, uint8_t pixel_format)
{
	(void)(pixel_format);
	return pixels * 2UL;
}

 void lcd_flush(bsp_lcd_t *hlcd)
{
	lcd_set_display_area(&hlcd->area);
	bsp_lcd_send_cmd_mem_write();
#if (USE_DMA == 0)
	bsp_lcd_write(hlcd->buff_to_flush, hlcd->write_length);
	hlcd->buff_to_flush = NULL;
#else
	lcd_write_dma((uint32_t)hlcd->buff_to_flush,hlcd->write_length);
#endif
}

uint32_t copy_to_draw_buffer( bsp_lcd_t *hlcd,uint32_t nbytes,uint32_t rgb888)
{
	uint16_t *fb_ptr = NULL;
	uint32_t npixels;
	hlcd->buff_to_draw = get_buff(hlcd);
	fb_ptr = (uint16_t*)hlcd->buff_to_draw;
	nbytes =  ((nbytes > DB_SIZE)?DB_SIZE:nbytes);
	npixels= bytes_to_pixels(nbytes,hlcd->pixel_format);
	if(hlcd->buff_to_draw != NULL){
		for(uint32_t i = 0 ; i < npixels ;i++){
			*fb_ptr = convert_rgb888_to_rgb565(rgb888);
			fb_ptr++;
		}
		hlcd->write_length = pixels_to_bytes(npixels,hlcd->pixel_format);
		hlcd->buff_to_flush = hlcd->buff_to_draw;
		hlcd->buff_to_draw = NULL;
		lcd_flush(hlcd);
		return pixels_to_bytes(npixels,hlcd->pixel_format);
	}

	return 0;
}

uint32_t get_total_bytes(bsp_lcd_t *hlcd,uint32_t w , uint32_t h)
{
	uint8_t bytes_per_pixel = 2;
	if(hlcd->pixel_format == BSP_LCD_PIXEL_FMT_RGB565){
		bytes_per_pixel = 2;
	}
	return (w * h * bytes_per_pixel);
}

uint8_t *get_buff(bsp_lcd_t *hlcd)
{
	uint32_t buf1 = (uint32_t)hlcd->draw_buffer1;
	uint32_t buf2 = (uint32_t)hlcd->draw_buffer2;

	if(hlcd->buff_to_draw == NULL && hlcd->buff_to_flush == NULL){
		return  hlcd->draw_buffer1;
	}else if((uint32_t)hlcd->buff_to_flush == buf1 && hlcd->buff_to_draw == NULL ){
		return  hlcd->draw_buffer2;
	}else if ((uint32_t)hlcd->buff_to_flush == buf2 && hlcd->buff_to_draw == NULL){
		return  hlcd->draw_buffer1;
	}

	return NULL;
}

/* BSP Functions */

void bsp_lcd_init(void)
{
    lcd_handle.orientation = BSP_LCD_ORIENTATION;
	lcd_handle.pixel_format = BSP_LCD_PIXEL_FMT;
	lcd_config();
	hlcd->area.x1 = 0;
	hlcd->area.x2 = BSP_LCD_ACTIVE_WIDTH-1;
	hlcd->area.y1 = 0;
	hlcd->area.y2 = BSP_LCD_ACTIVE_HEIGHT-1;
	lcd_set_display_area(&hlcd->area);
	lcd_set_orientation(hlcd->orientation);
	lcd_buffer_init(hlcd);
}

void bsp_lcd_set_orientation(int orientation)
{
    lcd_set_orientation(orientation);
}

int bsp_lcd_write(uint8_t *buffer, uint32_t nbytes)
{
    return spi_write(FRAME_16_B|DATA_MODE, buffer, nbytes);
}

void bsp_lcd_set_background_color(uint32_t rgb888)
{
	bsp_lcd_fill_rect(rgb888, 0, (BSP_LCD_ACTIVE_WIDTH), 0, (BSP_LCD_ACTIVE_HEIGHT));
}

void bsp_lcd_fill_rect(uint32_t rgb888, uint32_t x_start, uint32_t x_width,uint32_t y_start,uint32_t y_height)
{

	uint32_t total_bytes_to_write = 0;
	uint32_t bytes_sent_so_far = 0;
	uint32_t remaining_bytes = 0;;
	uint32_t npix;
	uint32_t pixels_sent = 0;
	uint32_t x1,y1;
	uint32_t pixel_per_line = x_width;

	if((x_start+x_width) > BSP_LCD_ACTIVE_WIDTH) return;
	if((y_start+y_height) > BSP_LCD_ACTIVE_HEIGHT) return;

	//1. calculate total number of bytes written in to DB
	total_bytes_to_write = get_total_bytes(hlcd,x_width,y_height);
	remaining_bytes = total_bytes_to_write;
	while(remaining_bytes){
		x1 = x_start+(pixels_sent % pixel_per_line);
		y1 = y_start+(pixels_sent / pixel_per_line);

		make_area(&hlcd->area, x1, x_width, y1, y_height);

		if(x1 != x_start){
			npix = x_start+x_width - x1;
		}else{
			npix = bytes_to_pixels(remaining_bytes, hlcd->pixel_format);
		}

		bytes_sent_so_far  +=  copy_to_draw_buffer(hlcd, pixels_to_bytes(npix,hlcd->pixel_format), rgb888);
		pixels_sent = bytes_to_pixels(bytes_sent_so_far, hlcd->pixel_format);
		remaining_bytes = total_bytes_to_write - bytes_sent_so_far;
    }
}

void bsp_lcd_set_display_area(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2)
{
    lcd_area_t area;
    area.x1 = x1;
    area.x2 = x2;
    area.y1 = y1;
    area.y2 = y2;
    lcd_set_display_area(&area);
}

void bsp_lcd_send_cmd_mem_write(void)
{
    lcd_write_cmd(ILI9341_GRAM);
}

void *bsp_lcd_get_draw_buffer1_addr(void)
{
    return (void*)hlcd->draw_buffer1;
}
void *bsp_lcd_get_draw_buffer2_addr(void)
{
	return (void*)hlcd->draw_buffer2;
}

uint16_t bsp_lcd_convert_rgb888_to_rgb565(uint32_t rgb888)
{
	return convert_rgb888_to_rgb565(rgb888);
}


