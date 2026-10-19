#ifndef __WRAP_SENSOR__
#define __WRAP_SENSOR__

#include <mach/anyka_types.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <plat-anyka/cam_com_sensor.h>

/**
 * @brief millisecond delay
 * @author dengzhou
 * @date 2012-03-16
 * @param[in] minisecond minisecond delay number
 * @return T_VOID
 */
T_VOID mini_delay(T_U32 minisecond);

/**
 * @brief anyka specific printf
 * @author dengzhou
 * @date 2012-03-16
 * @param[in] level forbidden level
 * @param[in] mStr module string
 * @param[in] s format string
 * @return T_S32
 * @retval 0 is print ok, -1 is forbidden to print
 */
T_S32 akprintf(T_U8 level, T_pCSTR mStr, T_pCSTR s, ...);

/**
 * @brief write data to SCCB device
 *
 * write size length data to daddr's raddr register
 * @author dengzhou
 * @date 2012-03-16
 * @param[in] daddr SCCB device address
 * @param[in] raddr register address
 * @param[in] data write data's point
 * @param[in] size write data's length
 * @return T_BOOL return write success or failed
 * @retval AK_FALSE operate failed
 * @retval AK_TRUE operate success
 */
T_BOOL sccb_write_data(T_U8 daddr, T_U8 raddr, T_U8 *data, T_U32 size);
T_BOOL sccb_write_short(T_U8 daddr, T_U16 raddr, T_U8 *data, T_U32 size);
T_BOOL sccb_write_word(T_U8 daddr, T_U16 raddr, T_U16 *data, T_U32 size);

/**
 * @brief read data from SCCB device function
 *
 * read data from daddr's raddr register
 * @author dengzhou
 * @date 2012-03-16
 * @param[in] daddr SCCB device address
 * @param[in] raddr register address
 * @return T_U8
 * @retval read-back data
 */
T_U8 sccb_read_data(T_U8 daddr, T_U8 raddr);
T_U8 sccb_read_short(T_U8 daddr, T_U16 raddr);
T_U16 sccb_read_word(T_U8 daddr, T_U16 raddr);

/*@{*/
/**
 * @brief SCCB interface initialize function
 *
 * setup SCCB interface
 * @author dengzhou
 * @date 2012-03-16
 * @param[in] pin_scl the pin assigned to SCL
 * @param[in] pin_sda the pin assigned to SDA
 * @return T_VOID
 */
T_VOID sccb_init(T_U32 pin_scl, T_U32 pin_sda);


/**
 * @brief set client handle
 * @author dengzhou
 * @date 2012-03-19
 * @param[in] client handle of I2C open
 * @return T_VOID
 */
void sensor_set_handle(struct i2c_client *client);

/*
 * The camera_* drivers drive their sensor's AVDD, chip-enable and reset pins
 * through these names. In the 3.4 SDK they were the mach-ak39 GPIO calls on
 * pins from board data; here they are gpiolib on the pins aksensor.c took
 * from the sensor's device tree node. A pin the board does not wire is
 * INVALID_GPIO, and every operation on it does nothing, as the SDK's I2C
 * pins already were.
 */
#ifndef INVALID_GPIO
#define INVALID_GPIO		(-1)
#endif

#define GPIO_I2C_SCL		INVALID_GPIO
#define GPIO_I2C_SDA		INVALID_GPIO

T_U32 cam_getpin(T_CAMERA_PINTYPE pin_type);

#define GPIO_CAMERA_AVDD		cam_getpin(PIN_AVDD)
#define GPIO_CAMERA_CHIP_ENABLE		cam_getpin(PIN_POWER)
#define GPIO_CAMERA_RESET		cam_getpin(PIN_RESET)

#define GPIO_DIR_INPUT		0
#define GPIO_DIR_OUTPUT		1
#define GPIO_LEVEL_LOW		0
#define GPIO_LEVEL_HIGH		1

static inline void gpio_set_pin_dir(int pin, int dir)
{
	if (!gpio_is_valid(pin))
		return;
	if (dir == GPIO_DIR_OUTPUT)
		gpio_direction_output(pin, gpio_get_value(pin));
	else
		gpio_direction_input(pin);
}

static inline void gpio_set_pin_level(int pin, int level)
{
	if (gpio_is_valid(pin))
		gpio_set_value(pin, level);
}

/* The pad mux is the pinctrl driver's; claiming the GPIO already selects it. */
static inline void gpio_set_pin_as_gpio(int pin)
{
}

static inline int gpio_pin_get_ActiveLevel(int pin)
{
	return gpio_is_valid(pin) ? gpio_get_value(pin) : 0;
}

#endif
