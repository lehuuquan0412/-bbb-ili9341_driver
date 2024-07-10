#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <ili9341.h>

#define DRIVER_AUTHOR                   "quan0412 lehuuquan0412@gmail.com"
#define DRIVER_DESC                     "ili9341 driver for LCD-TFT"
#define DRIVER_LICENSE                  "GPL"

#define DEVICE_NAME                     "ili9341"
#define DEVICE_CLASS                    "ili9341_class"

#define BSP_LCD_WIDTH  		            240
#define BSP_LCD_HEIGHT 		            320


#define	BSP_LCD_PIXEL_FMT_L8 		1
#define	BSP_LCD_PIXEL_FMT_RGB565	2
#define BSP_LCD_PIXEL_FMT_RGB666    3
#define	BSP_LCD_PIXEL_FMT_RGB888	4
#define BSP_LCD_PIXEL_FMT 			BSP_LCD_PIXEL_FMT_RGB565

#define PORTRAIT  0
#define LANDSCAPE 1
#define BSP_LCD_ORIENTATION   PORTRAIT


#if(BSP_LCD_ORIENTATION == PORTRAIT)
	#define  BSP_LCD_ACTIVE_WIDTH 			BSP_LCD_WIDTH
	#define  BSP_LCD_ACTIVE_HEIGHT  		BSP_LCD_HEIGHT
#elif(BSP_LCD_ORIENTATION == LANDSCAPE)
	#define  BSP_LCD_ACTIVE_WIDTH 			BSP_LCD_HEIGHT
	#define  BSP_LCD_ACTIVE_HEIGHT 			BSP_LCD_WIDTH
#endif


static const struct of_device_id ili9341_dt_ids [] = {
    {.compatible = "ili9341",},
    {}
};

struct ili9341_device
{
    struct spi_device *spi;
    struct device *spi_device_t;
    struct gpio_desc *dcx_pin;
    struct gpio_desc *rsx_pin;
    struct cdev cdev;
    struct class *class;
    dev_t dev;
}

static int          ili9341_open(struct inode *inode, struct file *file);
static ssize_t      ili9341_write(struct file *file, const char __user *buf, size_t size, loff_t *poss);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = ili9341_open,
    .write = ili9341_write,
};

static char *my_devnode(struct device *dev, umode_t *mode) {
    if (mode) {
        *mode = 0666; // Set the permissions to 0666
    }
    return NULL;
}

static int ili9341_send_data_8b(struct ili9341_device *device, uint8_t *data, uint32_t len)
{
    device->spi->bits_per_word = 8;
    spi_setup(device->spi);
    return spi_write(device->spi, data, len);
}

static int ili9341_send_data_16b(struct ili9341_device *device, uint8_t *data, uint32_t len)
{
	device->spi->bits_per_word = 16;
	spi_setup(device->spi);
	uint16_t *data_to_send = (uint16_t *)data;
 	return spi_write(device->spi, data_to_send, len);
}

static void ili9341_send_cmd(struct ili9341_device *device, uint8_t cmd)
{
    gpiod_set_value(device->dcx_pin, 0);
	device->spi->bits_per_word = 8;
	spi_setup(device->spi);
    spi_write(device->spi, &cmd, 1);
    gpiod_set_value(device->dcx_pin, 1);
}

static int ili9341_open(struct inode *inode, struct file *file)
{
    struct ili9341_device *ili9341 = container_of(inode->i_cdev, struct ili9341_device, cdev);
    file->private_data = ili9341;
    return 0;
}

static ssize_t ili9341_write(struct file *file, const char __user *buf, size_t size, loff_t *poss)
{
	struct ili9341_device *ili9341 = file->private_data;
	uint8_t *kbuf;
	uint8_t type_data;
	int ret;

	if (size < 2) 			return size;

	if (copy_from_user(&type_data, buf, 1))
	{
		return -EFAULT;
	}

	buf += 1;

	kbuf = kmalloc(size - 1, GFP_KERNEL);
	if (!kbuf)				return -ENOMEM;

	if (copy_from_user(kbuf, buf, size - 1))
	{
		kfree(kbuf);
		return -EFAULT;
	}

	if ((type_data & 1) == SPI_SEND_CMD)
	{
		ret = ili9341_send_cmd(ili9341, *kbuf);
	}else
	{
		if (((type_data & FRAME_16_B) >> 1) == 0) 		ret = ili9341_send_data_8b(ili9341, kbuf, size - 1);
		else ret = ili9341_send_data_16b(ili9341, kbuf, size - 1);
	}

	kfree(kbuf);

	if (ret < 0){
		return ret;
	} 					
	return size;
}


static void ili9341_reset(struct ili9341_device *device)
{
    gpiod_set_value(device->rsx_pin, 0);
    udelay(5);
    gpiod_set_value(device->rsx_pin, 1);
    udelay(5);
}

static void ili9341_init(struct ili9341_device *device)
{
    uint8_t params[15];
	ili9341_send_cmd(device->spi, ILI9341_SWRESET);
	ili9341_send_cmd(device->spi, ILI9341_POWERB);
	params[0] = 0x00;
	params[1] = 0xD9;
	params[2] = 0x30;
	ili9341_send_data_8b(device->spi, params, 3);

	ili9341_send_cmd(device->spi, ILI9341_POWER_SEQ);
	params[0]= 0x64;
	params[1]= 0x03;
	params[2]= 0X12;
	params[3]= 0X81;
	ili9341_send_data_8b(device->spi, params, 4);

	ili9341_send_cmd(device->spi, ILI9341_DTCA);
	params[0]= 0x85;
	params[1]= 0x10;
	params[2]= 0x7A;
	ili9341_send_data_8b(device->spi, params, 3);

	ili9341_send_cmd(device->spi, ILI9341_POWERA);
	params[0]= 0x39;
	params[1]= 0x2C;
	params[2]= 0x00;
	params[3]= 0x34;
	params[4]= 0x02;
	ili9341_send_data_8b(device->spi, params, 5);

	ili9341_send_cmd(device->spi, ILI9341_PRC);
	params[0]= 0x20;
	ili9341_send_data_8b(device->spi, params, 1);

	ili9341_send_cmd(device->spi, ILI9341_DTCB);
	params[0]= 0x00;
	params[1]= 0x00;
	ili9341_send_data_8b(device->spi, params, 2);

	ili9341_send_cmd(device->spi, ILI9341_POWER1);
	params[0]= 0x1B;
	ili9341_send_data_8b(device->spi, params, 1);

	ili9341_send_cmd(device->spi, ILI9341_POWER2);
	params[0]= 0x12;
	ili9341_send_data_8b(device->spi, params, 1);

	ili9341_send_cmd(device->spi, ILI9341_VCOM1);
	params[0]= 0x08;
	params[1]= 0x26;
	ili9341_send_data_8b(device->spi, params, 2);

	ili9341_send_cmd(device->spi, ILI9341_VCOM2);
	params[0]= 0XB7;
	ili9341_send_data_8b(device->spi, params, 1);


	ili9341_send_cmd(device->spi, ILI9341_PIXEL_FORMAT);
	params[0]= 0x55; //select RGB565
	ili9341_send_data_8b(device->spi, params, 1);

	ili9341_send_cmd(device->spi, ILI9341_FRMCTR1);
	params[0]= 0x00;
	params[1]= 0x1B;//frame rate = 70
	ili9341_send_data_8b(device->spi, params, 2);

	ili9341_send_cmd(device->spi, ILI9341_DFC);    // Display Function Control
	params[0]= 0x0A;
	params[1]= 0xA2;
	ili9341_send_data_8b(device->spi, params, 2);

	ili9341_send_cmd(device->spi, ILI9341_3GAMMA_EN);    // 3Gamma Function Disable
	params[0]= 0x02; //LCD_WR_DATA(0x00);
	ili9341_send_data_8b(device->spi, params, 1);

	ili9341_send_cmd(device->spi, ILI9341_GAMMA);
	params[0]= 0x01;
	ili9341_send_data_8b(device->spi, params, 1);

	ili9341_send_cmd(device->spi, ILI9341_PGAMMA);    //Set Gamma
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
	ili9341_send_data_8b(device->spi, params, 15);

	ili9341_send_cmd(device->spi, ILI9341_NGAMMA);
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
	ili9341_send_data_8b(device->spi, params, 15);

	ili9341_send_cmd(device->spi, ILI9341_SLEEP_OUT); //Exit Sleep
	udelay(10);
	ili9341_send_cmd(device->spi, ILI9341_DISPLAY_ON); //display on
}

void ili9341_set_orientation(struct ili9341_device *device, uint8_t orientation)
{
	uint8_t param;

	if(orientation == LANDSCAPE){
		param = MADCTL_MV | MADCTL_MY | MADCTL_BGR; /*Memory Access Control <Landscape setting>*/
	}else if(orientation == PORTRAIT){
		param = MADCTL_MY| MADCTL_MX| MADCTL_BGR;  /* Memory Access Control <portrait setting> */
	}

	ili9341_send_cmd(device->spi, ILI9341_MAC);    // Memory Access Control command
	ili9341_send_data_8b(device->spi, &param, 1);
}

void ili9341_set_display_area(struct ili9341_device *device)
{
	uint8_t params[4];

	params[0] = (0 >> 8) & 0xFF;
	params[1] = (0 >> 0) & 0xFF;
	params[2] = (BSP_LCD_ACTIVE_WIDTH >> 8) & 0xFF;
	params[3] = (BSP_LCD_ACTIVE_WIDTH >> 0) & 0xFF;
	ili9341_send_cmd(device->spi, ILI9341_CASET);
	ili9341_send_data_8b(device->spi, params, 4);

	params[2] = (BSP_LCD_ACTIVE_HEIGHT >> 8) & 0xFF;
	params[3] = (BSP_LCD_ACTIVE_HEIGHT >> 0) & 0xFF;

	ili9341_send_cmd(device->spi, ILI9341_RASET);
	ili9341_send_data_8b(device->spi, params, 4);
}	

void ili9341_configure(struct ili9341_device *device)
{
	ili9341_reset(device);
	ili9341_init(device);
	ili9341_set_display_area(device);
	ili9341_set_orientation(device);
}


static int ili9341_pdrv_probe(struct spi_device *device)
{
	struct ili9341_device *ili9341;
	int ret;

	ili9341 = kmalloc(sizeof(*ili9341), GFP_KERNEL);

	if (!ili9341)
	{
		pr_err("kmalloc failed\n");
		return -1;
	}

	ili9341->spi = device;
	spi_setup(device);

	ili9341->dcx_pin = gpiod_get(&device->dev, "dcx", GPIOD_OUT_LOW);
	gpiod_set_value(ili9341->dcx_pin, 1);

	ili9341->rsx_pin = gpiod_get(&device->dev, "rsx", GPIOD_OUT_LOW);
	gpiod_set_value(ili9341->rsx_pin, 1);

	ili9341_configure(ili9341);

	ret = alloc_chrdev_region(&ili9341->dev, 0, 1, DEVICE_NAME);
	if (ret < 0)
	{
		pr_err("Failed to allocate character device region\n");
        return ret;
	}

	cdev_init(&ili9341->cdev, &fops);
	ili9341->cdev.owner = THIS_MODULE;
	
	ret = cdev_add(&ili9341->cdev, ili9341->dev, 1);
	if (ret)
	{
		pr_err("Failed to add character device\n");
		unregister_chrdev_region(ili9341->dev, 1);
		return ret;
	}

	ili9341->class = class_create(THIS_MODULE, DEVICE_CLASS);
	if (IS_ERR(ili9341->class))
	{
		pr_err("Failed to create class\n");
		cdev_del(&ili9341->cdev);
		unregister_chrdev_region(&ili9341->dev, 1);
		return PTR_ERR(ili9341->class);
	}

	ili9341->class->devnode = my_devnode;
	ili9341->spi_device_t = device_create(ili9341->class, NULL, ili9341->dev, NULL, DEVICE_NAME);

	spi_set_drvdata(device, ili9341);

	pr_info("Success !!!\n");
	return 0;
}

static int ili9341_pdrv_remove(struct spi_device *device)
{
	struct ili9341_device *ili9341 = spi_get_drvdata(device);

	device_destroy(ili9341->class, ili9341->dev);
	class_destroy(ili9341->class);
	cdev_del(&ili9341->cdev);
	unregister_chrdev_region(ili9341->dev, 1);

	gpiod_set_value(ili9341->dcx_pin, 0);
	gpiod_set_value(ili9341->rsx_pin, 0);

	gpiod_put(ili9341->dcx_pin);
	gpiod_put(ili9341->rsx_pin);

	pr_info("Goodbye @@@\n");
	return 0;
}

MODULE_DEVICE_TABLE(of, ili9341_dt_ids);

static struct spi_driver ili9341_spi_driver = {
	.probe = ili9341_pdrv_probe,
	.remove = ili9341_pdrv_remove,
	.driver = {
		.name = "ili9341",
		.of_match_table = ili9341_dt_ids,
		.owner = THIS_MODULE,
	},
};

module_spi_driver(ili9341_spi_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_VERSION("1.0");