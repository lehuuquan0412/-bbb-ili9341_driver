#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>


#define DRIVER_AUTHOR                   "quan0412 lehuuquan0412@gmail.com"
#define DRIVER_DESC                     "xpt2046 driver for TouchGFX"
#define DRIVER_LICENSE                  "GPL"

#define INPUT_DEV_NAME                      "xpt2046"

#define XPT2046_CMD_X                   0x90
#define XPT2046_CMD_Y                   0xD0

static const struct of_device_id xpt2046_ts_ids [] = {
    {.compatible = "xpt2046_ts",},
    {}
};

struct xpt2046_ts{
    struct gpio_desc *data_pin;
    struct input_dev *input;
    struct spi_device *spi;
    
    int irq;
};

static int xpt2046_read_touch(struct spi_device *spi, uint8_t cmd)
{
    uint8_t tx_buf[3] = {cmd, 0, 0};
    uint8_t rx_buf[3] = {0, 0, 0};
    struct spi_transfer t = {
        .tx_buf = tx_buf,
        .rx_buf = rx_buf,
        .len = 3,
    };
    spi_sync_transfer(spi, &t, 1);
    return ((rx_buf[1] << 8) | (rx_buf[2])) >> 3;
}

static void xpt2046_work_handler(struct work_struct *work)
{
    struct xpt2046_ts *data = container_of(work, struct xpt2046_ts, work);
    int x, y;
    x = xpt2046_read_touch(data->spi, XPT2046_CMD_X);
    y = xpt2046_read_touch(data->spi, XPT2046_CMD_Y);
    
    pr_info("Data x: %d, y: %d\n", x, y);

    input_report_abs(data->input, ABS_X, x);
    input_report_abs(data->input, ABS_Y, y);
    input_sync(data->input);
    enable_irq(data->irq);
    return;
}

static irqreturn_t xpt2046_irq_handler(int irq, void *dev_id)
{
    struct xpt2046_ts *data = dev_id;
    pr_info("Interrupt handle\n");
    disable_irq_nosync(data->irq);
    schedule_work(&data->work);
    return IRQ_HANDLED;
}

static int xpt2046_pdrv_probe(struct spi_device *pdev)
{
    struct xpt2046_ts *device;
    struct input_dev *i_dev;
    char dev_path[100];
    int ret;

    device = kmalloc(sizeof(*device), GFP_KERNEL);
    if (!device)
        return -ENOMEM;

    i_dev = devm_input_allocate_device(&pdev->dev);
    if (!i_dev)
        return -ENOMEM;

    snprintf(dev_path, sizeof(dev_path), "spi/%s", dev_name(&pdev->dev));

    i_dev->name = INPUT_DEV_NAME;
    i_dev->phys = dev_path;
    i_dev->id.bustype = BUS_SPI;

    set_bit(EV_ABS, i_dev->evbit);

    input_set_abs_params(i_dev, ABS_X, 0, 0xFFF, 0, 0);
    input_set_abs_params(i_dev, ABS_Y, 0, 0xFFF, 0, 0);

    INIT_WORK(&device->work, xpt2046_work_handler);

    ret = input_register_device(i_dev);
    if (ret)    
    {
        input_free_device(i_dev);
        return ret;
    }

    device->spi = pdev;
    device->input = i_dev;
    device->spi = pdev;
    device->data_pin = gpiod_get(&pdev->dev, "irq", GPIOD_IN);
    device->input = i_dev;
    device->irq = gpiod_to_irq(device->data_pin);

    ret = request_irq(device->irq, xpt2046_irq_handler, IRQF_TRIGGER_LOW|IRQF_ONESHOT, "xpt2046_data", device);
    if (ret)  
    {
        input_unregister_device(i_dev);
        input_free_device(i_dev);
        return ret;
    }

    spi_set_drvdata(pdev, device);
    pr_info("Success!!!\n");

    return 0;
}

static int xpt2046_pdrv_remove(struct spi_device *pdev)
{
    struct xpt2046_ts *device = spi_get_drvdata(pdev);

    input_unregister_device(device->input);
    input_free_device(device->input);
    free_irq(device->irq, device);
    gpiod_put(device->data_pin);

    pr_info("Goodbye !!!\n");
    return 0;
}

MODULE_DEVICE_TABLE(of, xpt2046_ts_ids);

static struct spi_driver xpt2046_spi_driver = {
    .probe = xpt2046_pdrv_probe,
    .remove = xpt2046_pdrv_remove,
    .driver = {
        .name = "tcs",
        .of_match_table = xpt2046_ts_ids,
        .owner = THIS_MODULE,
    },
};

module_spi_driver(xpt2046_spi_driver);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_VERSION("1.0");