#include "sitronix_ts_custom_func.h"
#include "sitronix_ts.h"

#define SWK_NO 0x0
#define GESTURE_LEFT 0xB4
#define GESTURE_RIGHT 0xB0
#define GESTURE_UP 0xBC
#define GESTURE_DOWN 0xB8
#define GESTURE_DOUBLECLICK 0xC0
#define GESTURE_SINGLECLICK 0xC8
#define GESTURE_O 0x6F
#define GESTURE_W 0x77
#define GESTURE_M 0x6D
#define GESTURE_E 0x65
#define GESTURE_S 0x73
#define GESTURE_V 0x76
#define GESTURE_Z 0x7A
#define GESTURE_C 0x63

#define WINDOW_L 200
#define WINDOW_R 600
#define WINDOW_T 200
#define WINDOW_B 600

#define KEY_GESTURE_U KEY_U
#define KEY_GESTURE_UP KEY_UP
#define KEY_GESTURE_DOWN KEY_DOWN
#define KEY_GESTURE_LEFT KEY_LEFT
#define KEY_GESTURE_RIGHT KEY_RIGHT
#define KEY_GESTURE_O KEY_O
#define KEY_GESTURE_E KEY_E
#define KEY_GESTURE_M KEY_M
#define KEY_GESTURE_L KEY_L
#define KEY_GESTURE_W KEY_W
#define KEY_GESTURE_S KEY_S
#define KEY_GESTURE_V KEY_V
#define KEY_GESTURE_C KEY_C
#define KEY_GESTURE_Z KEY_Z

#ifdef SITRONIX_GESTURE
typedef enum {
	G_NO = 0x0,
	G_ZOOM_IN = 0x2,
	G_ZOOM_OUT = 0x3,
	G_L_2_R = 0x4,
	G_R_2_L = 0x5,
	G_T_2_D = 0x6,
	G_D_2_T = 0x7,
	G_PALM = 0x8,
	G_SINGLE_TAB = 0x9,
} GESTURE_ID;
#endif

static atomic_t swk_flag = ATOMIC_INIT(0);

static void sitronix_swk_flag_set(int flag)
{
	atomic_set(&swk_flag, flag);
}

static void st_swk_enable(struct sitronix_ts_data *ts)
{
	int ret = 0;
	unsigned char buffer[2] = { 0 };
	ret = stx_i2c_read(ts->client, buffer, 1, MISC_CONTROL);
	if (ret == 1) {
		buffer[1] = buffer[0] | 0x80;
		buffer[0] = MISC_CONTROL;
		stx_i2c_write(ts->client, buffer, 2);

		msleep(50);
	}
}

static int sitronix_swk_flag_get(void)
{
	int flag = 0;
	flag = atomic_read(&swk_flag);
	return flag;
}

int sitronix_swk_enable(void)
{
	sitronix_swk_flag_set(1);
	st_swk_enable(&stx_gpts);

	return 0;
}

int sitronix_swk_disable(void)
{
	sitronix_swk_flag_set(0);

	return 0;
}

static int st_get_wakeup_status(struct sitronix_ts_data *ts, char *buffer)
{
	int ret = -1;

	ret = stx_i2c_read(ts->client, buffer, 1, SMART_WAKE_UP_REG);

	return ret;
}

int sitronix_swk_func(struct input_dev *dev)
{
	int gesture = 0;
	int gesture_id = 0;
	int ret = 0;
	char buf[2] = { 0 };

	if (sitronix_swk_flag_get() == 1) {
		ret = st_get_wakeup_status(&stx_gpts, buf);
		if (ret != 1) {
			STX_ERROR("sitronix_swk_func 1");
			return 1;
		}

		if (buf[0] == SWK_NO) {
			STX_ERROR("sitronix_swk_func 2");
			return 2;
		}
		gesture_id = buf[0];

		switch (gesture_id) {
		case GESTURE_LEFT:
			gesture = KEY_GESTURE_LEFT;
			break;
		case GESTURE_RIGHT:
			gesture = KEY_GESTURE_RIGHT;
			break;
		case GESTURE_UP:
			gesture = KEY_GESTURE_UP;
			break;
		case GESTURE_DOWN:
			gesture = KEY_GESTURE_DOWN;
			break;
		case GESTURE_DOUBLECLICK:
			gesture = KEY_POWER; //KEY_GESTURE_U;
			break;
		case GESTURE_SINGLECLICK:
			gesture = KEY_POWER;
			break;
		case GESTURE_O:
			gesture = KEY_GESTURE_O;
			break;
		case GESTURE_W:
			gesture = KEY_GESTURE_W;
			break;
		case GESTURE_M:
			gesture = KEY_GESTURE_M;
			break;
		case GESTURE_E:
			gesture = KEY_GESTURE_E;
			break;
		case GESTURE_S:
			gesture = KEY_GESTURE_S;
			break;
		case GESTURE_V:
			gesture = KEY_GESTURE_V;
			break;
		case GESTURE_Z:
			gesture = KEY_GESTURE_Z;
			break;
		case GESTURE_C:
			gesture = KEY_GESTURE_C;
			break;
		default:
			gesture = -1;
			break;
		}

		STX_INFO("sitronix_swk_func ,  gesture_id:0x%X, gesture = 0x%x",
			 gesture_id, gesture);
		if (gesture != -1) {
			input_report_key(dev, gesture, 1);
			input_sync(dev);
			input_report_key(dev, gesture, 0);
			input_sync(dev);
		}
		return 0;
	} else {
		return 3;
	}
}

int st_gesture_init(void)
{
	struct input_dev *input_dev = stx_gpts.input_dev; //tpd->dev;

	input_set_capability(input_dev, EV_KEY, KEY_F13);
	input_set_capability(input_dev, EV_KEY, KEY_F14);
	input_set_capability(input_dev, EV_KEY, KEY_POWER);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_U);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_UP);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_DOWN);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_LEFT);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_RIGHT);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_O);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_E);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_M);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_L);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_W);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_S);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_V);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_Z);
	input_set_capability(input_dev, EV_KEY, KEY_GESTURE_C);

	__set_bit(KEY_GESTURE_RIGHT, input_dev->keybit);
	__set_bit(KEY_GESTURE_LEFT, input_dev->keybit);
	__set_bit(KEY_GESTURE_UP, input_dev->keybit);
	__set_bit(KEY_GESTURE_DOWN, input_dev->keybit);
	__set_bit(KEY_GESTURE_U, input_dev->keybit);
	__set_bit(KEY_GESTURE_O, input_dev->keybit);
	__set_bit(KEY_GESTURE_E, input_dev->keybit);
	__set_bit(KEY_GESTURE_M, input_dev->keybit);
	__set_bit(KEY_GESTURE_W, input_dev->keybit);
	__set_bit(KEY_GESTURE_L, input_dev->keybit);
	__set_bit(KEY_GESTURE_S, input_dev->keybit);
	__set_bit(KEY_GESTURE_V, input_dev->keybit);
	__set_bit(KEY_GESTURE_C, input_dev->keybit);
	__set_bit(KEY_GESTURE_Z, input_dev->keybit);

	__set_bit(KEY_POWER, input_dev->keybit);

	return 0;
}

#ifdef SITRONIX_GESTURE
void sitronix_gesture_func(struct input_dev *input_dev, int id)
{
	if (id == G_PALM) {
		STX_INFO("Gesture for Palm to suspend ");
		input_report_key(input_dev, KEY_POWER, 1); // KEY_LEFT, 1);
		input_sync(input_dev);
		input_report_key(input_dev, KEY_POWER, 0);
		input_sync(input_dev);
	}
}
#endif

bool sitronix_cases_mode_check(int x, int y)
{
	bool rt = true;
#ifdef ST_CASES_SWITCH_MODE
	if (stx_gpts.cases_mode)
		if (x < WINDOW_L || x > WINDOW_R || y < WINDOW_T ||
		    y > WINDOW_B)
			rt = false;
#endif
	return rt;
}