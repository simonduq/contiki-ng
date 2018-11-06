#include "contiki-conf.h"
#include "si1133.h"
#include "dev/i2c.h"
#include "em_cmu.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "clock.h"
#include "watchdog.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

#define X_ORDER_MASK            0x0070
#define Y_ORDER_MASK            0x0007
#define SIGN_MASK               0x0080
#define get_x_order(m)          ( (m & X_ORDER_MASK) >> 4)
#define get_y_order(m)          ( (m & Y_ORDER_MASK)      )
#define get_sign(m)             ( (m & SIGN_MASK) >> 7)

#define UV_INPUT_FRACTION       15
#define UV_OUTPUT_FRACTION      12
#define UV_NUMCOEFF             2

#define ADC_THRESHOLD           16000
#define INPUT_FRACTION_HIGH     7
#define INPUT_FRACTION_LOW      15
#define LUX_OUTPUT_FRACTION     12
#define NUMCOEFF_LOW            9
#define NUMCOEFF_HIGH           4

#define SI1133_CMD_RESET_CMD_CTR    0x00
#define SI1133_CMD_RESET            0x01
#define SI1133_CMD_NEW_ADDR         0x02
#define SI1133_CMD_FORCE_CH         0x11
#define SI1133_CMD_PAUSE_CH         0x12
#define SI1133_CMD_START            0x13
#define SI1133_CMD_PARAM_SET        0x80
#define SI1133_CMD_PARAM_QUERY      0x40

#define SI1133_REG_PART_ID          0x00
#define SI1133_REG_HW_ID            0x01
#define SI1133_REG_REV_ID           0x02
#define SI1133_REG_HOSTIN0          0x0A
#define SI1133_REG_COMMAND          0x0B
#define SI1133_REG_IRQ_ENABLE       0x0F
#define SI1133_REG_RESPONSE1        0x10
#define SI1133_REG_RESPONSE0        0x11
#define SI1133_REG_IRQ_STATUS       0x12

#define SI1133_PARAM_I2C_ADDR       0x00
#define SI1133_PARAM_CH_LIST        0x01
#define SI1133_PARAM_ADCCONFIG0     0x02
#define SI1133_PARAM_ADCSENS0       0x03
#define SI1133_PARAM_ADCPOST0       0x04
#define SI1133_PARAM_MEASCONFIG0    0x05
#define SI1133_PARAM_ADCCONFIG1     0x06
#define SI1133_PARAM_ADCSENS1       0x07
#define SI1133_PARAM_ADCPOST1       0x08
#define SI1133_PARAM_MEASCONFIG1    0x09
#define SI1133_PARAM_ADCCONFIG2     0x0A
#define SI1133_PARAM_ADCSENS2       0x0B
#define SI1133_PARAM_ADCPOST2       0x0C
#define SI1133_PARAM_MEASCONFIG2    0x0D
#define SI1133_PARAM_ADCCONFIG3     0x0E
#define SI1133_PARAM_ADCSENS3       0x0F
#define SI1133_PARAM_ADCPOST3       0x10
#define SI1133_PARAM_MEASCONFIG3    0x11
#define SI1133_PARAM_ADCCONFIG4     0x12
#define SI1133_PARAM_ADCSENS4       0x13
#define SI1133_PARAM_ADCPOST4       0x14
#define SI1133_PARAM_MEASCONFIG4    0x15
#define SI1133_PARAM_ADCCONFIG5     0x16
#define SI1133_PARAM_ADCSENS5       0x17
#define SI1133_PARAM_ADCPOST5       0x18
#define SI1133_PARAM_MEASCONFIG5    0x19
#define SI1133_PARAM_MEASRATE_H     0x1A
#define SI1133_PARAM_MEASRATE_L     0x1B
#define SI1133_PARAM_MEASCOUNT0     0x1C
#define SI1133_PARAM_MEASCOUNT1     0x1D
#define SI1133_PARAM_MEASCOUNT2     0x1E
#define SI1133_PARAM_THRESHOLD0_H   0x25
#define SI1133_PARAM_THRESHOLD0_L   0x26
#define SI1133_PARAM_THRESHOLD1_H   0x27
#define SI1133_PARAM_THRESHOLD1_L   0x28
#define SI1133_PARAM_THRESHOLD2_H   0x29
#define SI1133_PARAM_THRESHOLD2_L   0x2A
#define SI1133_PARAM_BURST          0x2B

#define SI1133_RSP0_CHIPSTAT_MASK   0xE0
#define SI1133_RSP0_COUNTER_MASK    0x1F
#define SI1133_RSP0_SLEEP           0x20

#define SI1133_OK                            0x0000
#define SI1133_ERROR_I2C_TRANSACTION_FAILED  0x0001
#define SI1133_ERROR_SLEEP_FAILED            0x0002


extern i2c_device_t lux_sens;

/* TODO: From Silabs Code */
typedef struct {
  int16_t     info;           /**< Info                              */
  uint16_t    mag;            /**< Magnitude                         */
} SI1133_Coeff_TypeDef;

typedef struct {
  uint8_t     irq_status;     /**< Interrupt status of the device    */
  int32_t     ch0;            /**< Channel 0 measurement data        */
  int32_t     ch1;            /**< Channel 1 measurement data        */
  int32_t     ch2;            /**< Channel 2 measurement data        */
  int32_t     ch3;            /**< Channel 3 measurement data        */
} SI1133_Samples_TypeDef;

typedef struct {
  SI1133_Coeff_TypeDef   coeff_high[4];   /**< High amplitude coeffs */
  SI1133_Coeff_TypeDef   coeff_low[9];    /**< Low amplitude coeffs  */
} SI1133_LuxCoeff_TypeDef;


static SI1133_LuxCoeff_TypeDef lk = {
  { {     0, 209 },           /**< coeff_high[0]   */
    {  1665, 93  },           /**< coeff_high[1]   */
    {  2064, 65  },           /**< coeff_high[2]   */
    { -2671, 234 } },         /**< coeff_high[3]   */
  { {     0, 0   },           /**< coeff_low[0]    */
    {  1921, 29053 },         /**< coeff_low[1]    */
    { -1022, 36363 },         /**< coeff_low[2]    */
    {  2320, 20789 },         /**< coeff_low[3]    */
    {  -367, 57909 },         /**< coeff_low[4]    */
    { -1774, 38240 },         /**< coeff_low[5]    */
    {  -608, 46775 },         /**< coeff_low[6]    */
    { -1503, 51831 },         /**< coeff_low[7]    */
    { -1886, 58928 } }        /**< coeff_low[8]    */
};

static SI1133_Coeff_TypeDef uk[2] = {
  { 1281, 30902 },            /**< coeff[0]        */
  { -638, 46301 }             /**< coeff[1]        */
};

static int32_t
SI1133_calcPolyInner(int32_t input, int8_t fraction, uint16_t mag, int8_t shift)
{
  int32_t value;

  if ( shift < 0 ) {
    value = ( (input << fraction) / mag) >> -shift;
  } else {
    value = ( (input << fraction) / mag) << shift;
  }

  return value;
}

static int32_t
SI1133_calcEvalPoly(int32_t x, int32_t y, uint8_t input_fraction, uint8_t output_fraction, uint8_t num_coeff, SI1133_Coeff_TypeDef *kp)
{
  uint8_t info, x_order, y_order, counter;
  int8_t sign, shift;
  uint16_t mag;
  int32_t output = 0, x1, x2, y1, y2;

  for ( counter = 0; counter < num_coeff; counter++ ) {
    info = kp->info;
    x_order = get_x_order(info);
    y_order = get_y_order(info);

    shift = ( (uint16_t) kp->info & 0xff00) >> 8;
    shift ^= 0x00ff;
    shift += 1;
    shift = -shift;

    mag = kp->mag;

    if ( get_sign(info) ) {
      sign = -1;
    } else {
      sign = 1;
    }

    if ( (x_order == 0) && (y_order == 0) ) {
      output += sign * mag << output_fraction;
    } else {
      if ( x_order > 0 ) {
        x1 = SI1133_calcPolyInner(x, input_fraction, mag, shift);
        if ( x_order > 1 ) {
          x2 = SI1133_calcPolyInner(x, input_fraction, mag, shift);
        } else {
          x2 = 1;
        }
      } else {
        x1 = 1;
        x2 = 1;
      }

      if ( y_order > 0 ) {
        y1 = SI1133_calcPolyInner(y, input_fraction, mag, shift);
        if ( y_order > 1 ) {
          y2 = SI1133_calcPolyInner(y, input_fraction, mag, shift);
        } else {
          y2 = 1;
        }
      } else {
        y1 = 1;
        y2 = 1;
      }

      output += sign * x1 * x2 * y1 * y2;
    }

    kp++;
  }

  if ( output < 0 ) {
    output = -output;
  }

  return output;
}

static int32_t
SI1133_getUv(int32_t uv, SI1133_Coeff_TypeDef *uk)
{
  int32_t uvi;

  uvi = SI1133_calcEvalPoly(0, uv, UV_INPUT_FRACTION, UV_OUTPUT_FRACTION, UV_NUMCOEFF, uk);

  return uvi;
}

static int32_t
SI1133_getLux(int32_t vis_high, int32_t vis_low, int32_t ir, SI1133_LuxCoeff_TypeDef *lk)
{
  int32_t lux;
  if((vis_high > ADC_THRESHOLD) || (ir > ADC_THRESHOLD) ) {
    lux = SI1133_calcEvalPoly(vis_high,
                              ir,
                              INPUT_FRACTION_HIGH,
                              LUX_OUTPUT_FRACTION,
                              NUMCOEFF_HIGH,
                              &(lk->coeff_high[0]) );
  } else {
    lux = SI1133_calcEvalPoly(vis_low,
                              ir,
                              INPUT_FRACTION_LOW,
                              LUX_OUTPUT_FRACTION,
                              NUMCOEFF_LOW,
                              &(lk->coeff_low[0]) );
  }
  return lux;
}


static uint32_t
SI1133_waitUntilSleep(void)
{
  uint32_t ret;
  uint8_t response;
  uint8_t count = 0;
  uint32_t retval;

  retval = 0;

  /* This loops until the Si1133 is known to be in its sleep state  */
  /* or if an i2c error occurs                                      */
  while( count < 5 ) {
    ret = i2c_read_register(&lux_sens, SI1133_REG_RESPONSE0, &response, 1);
    PRINTF("WaitSleep:%d (%d,%d)\n", response, count, (int) ret);
    if((response & SI1133_RSP0_CHIPSTAT_MASK) == SI1133_RSP0_SLEEP) {
      break;
    }

    if(ret != SI1133_OK) {
      retval = SI1133_ERROR_SLEEP_FAILED;
      break;
    }
    count++;
  }

  return retval;
}

uint32_t SI1133_paramSet(uint8_t address, uint8_t value)
{
  uint32_t retval;
  uint8_t buffer[2];
  uint8_t response_stored;
  uint8_t response;
  uint8_t count;

  PRINTF("Setting Param: %d = %d\n", 0x80 + (address & 0x3f), value);

  retval = SI1133_waitUntilSleep();
  if(retval != SI1133_OK) {
    return retval;
  }

  retval = i2c_read_register(&lux_sens, SI1133_REG_RESPONSE0,
                                 &response_stored, 1);
  response_stored &= SI1133_RSP0_COUNTER_MASK;

  buffer[0] = value;
  buffer[1] = 0x80 + (address & 0x3F);

  retval = i2c_write_register_buf(&lux_sens, SI1133_REG_HOSTIN0,
                                      (uint8_t*) buffer, 2);
  if(retval !=  I2C_BUS_STATUS_OK) {
    return retval;
  }

  /* Wait for command to finish */
  count = 0;
  /* Expect a change in the response register */
  while(count < 5) {
    retval = i2c_read_register(&lux_sens, SI1133_REG_RESPONSE0,
                                   &response, 1);
    if((response & SI1133_RSP0_COUNTER_MASK) != response_stored ) {
      break;
    } else {
      if(retval != I2C_BUS_STATUS_OK) {
        return retval;
      }
    }
    count++;
  }
  return SI1133_OK;
}

static uint32_t SI1133_sendCmd(uint8_t command)
{
  uint8_t response;
  uint8_t response_stored;
  uint8_t count = 0;
  uint32_t ret;

  /* Get the response register contents */
  ret = i2c_read_register(&lux_sens, SI1133_REG_RESPONSE0,
                              &response_stored, 1);
  if(ret != I2C_BUS_STATUS_OK) {
    return ret;
  }

  response_stored = response_stored & SI1133_RSP0_COUNTER_MASK;

  /* Double-check the response register is consistent */
  while ( count < 5 ) {
    ret = SI1133_waitUntilSleep();
    if ( ret != SI1133_OK ) {
      return ret;
    }
    /* Skip if the command is RESET COMMAND COUNTER */
    if ( command == SI1133_CMD_RESET_CMD_CTR ) {
      break;
    }

    ret = i2c_read_register(&lux_sens, SI1133_REG_RESPONSE0,
                                &response, 1);

    if((response & SI1133_RSP0_COUNTER_MASK) == response_stored) {
      break;
    } else {
      if(ret != SI1133_OK) {
        return ret;
      } else {
        response_stored = response & SI1133_RSP0_COUNTER_MASK;
      }
    }
    count++;
  }

  /* Send the command */
  ret = i2c_write_register(&lux_sens, SI1133_REG_COMMAND, command);
  if(ret != I2C_BUS_STATUS_OK) {
    return ret;
  }

  count = 0;
  /* Expect a change in the response register */
  while(count < 5) {
    /* Skip if the command is RESET COMMAND COUNTER */
    if (command == SI1133_CMD_RESET_CMD_CTR) {
      break;
    }
    ret = i2c_read_register(&lux_sens, SI1133_REG_RESPONSE0,
                                &response, 1);

    if((response & SI1133_RSP0_COUNTER_MASK) != response_stored) {
      break;
    } else {
      if(ret != I2C_BUS_STATUS_OK) {
        return ret;
      }
    }
    count++;
  }
  return SI1133_OK;
}


int si1133_init(void)
{
  int retval = 0;
  uint8_t id;
  CMU_ClockEnable(cmuClock_GPIO, true);

  /* enable Si1133 on the Sense 2 board */
  GPIO_PinModeSet(BOARD_ENV_ENABLE_PORT, BOARD_ENV_ENABLE_PIN,
                  gpioModePushPull, 1);

  clock_wait(CLOCK_SECOND);

  if(i2c_acquire(&lux_sens) > 0) {
    return 1;
  }

  retval = i2c_read_register(&lux_sens, SI1133_REG_PART_ID, &id, 1);

  PRINTF("SI1133: Part ID: %x (ret=%d)\n", id, retval);

  /* Perform the Reset Command */
  retval = i2c_write_register(&lux_sens, SI1133_REG_COMMAND,
                              SI1133_CMD_RESET);
  /* wait for reset to complete */
  clock_wait(CLOCK_SECOND);

  retval += SI1133_paramSet(SI1133_PARAM_CH_LIST, 0x0f);
  retval += SI1133_paramSet(SI1133_PARAM_ADCCONFIG0, 0x78);
  retval += SI1133_paramSet(SI1133_PARAM_ADCSENS0, 0x71);
  retval += SI1133_paramSet(SI1133_PARAM_ADCPOST0, 0x40);
  retval += SI1133_paramSet(SI1133_PARAM_ADCCONFIG1, 0x4d);
  retval += SI1133_paramSet(SI1133_PARAM_ADCSENS1, 0xe1);
  retval += SI1133_paramSet(SI1133_PARAM_ADCPOST1, 0x40);
  retval += SI1133_paramSet(SI1133_PARAM_ADCCONFIG2, 0x41);
  retval += SI1133_paramSet(SI1133_PARAM_ADCSENS2, 0xe1);
  retval += SI1133_paramSet(SI1133_PARAM_ADCPOST2, 0x50);
  retval += SI1133_paramSet(SI1133_PARAM_ADCCONFIG3, 0x4d);
  retval += SI1133_paramSet(SI1133_PARAM_ADCSENS3, 0x87);
  retval += SI1133_paramSet(SI1133_PARAM_ADCPOST3, 0x40);

  retval += i2c_write_register(&lux_sens, SI1133_REG_IRQ_ENABLE, 0x0f);

  if(i2c_release(&lux_sens) > 0) {
    return 1;
  }

  return retval;
}


int32_t
si1133_get_data(uint32_t *lux, uint32_t *uv)
{
  uint8_t buffer[13];
  SI1133_Samples_TypeDef samples;
  uint32_t retval = 0;
  uint8_t response;
  int count = 0;

  i2c_acquire(&lux_sens);

  /* force read */
  retval = SI1133_sendCmd(SI1133_CMD_FORCE_CH);

  /* how long conversion ? */
  clock_delay(CLOCK_SECOND / 5); /* 200 ms? */

  /* Check if the measurement finished, if not then wait */
  retval += i2c_read_register(&lux_sens, SI1133_REG_IRQ_STATUS,
                              &response, 1);

  while(response != 0x0F && count < 10) {
    clock_delay(5);
    watchdog_periodic();
    retval += i2c_read_register(&lux_sens, SI1133_REG_IRQ_STATUS,
                                &response, 1);
    count++;
  }

  /* Get the results */
  retval = i2c_read_register(&lux_sens, SI1133_REG_IRQ_STATUS,
                                 buffer, 13);
  samples.irq_status = buffer[0];

  samples.ch0 = buffer[1] << 16;
  samples.ch0 |= buffer[2] << 8;
  samples.ch0 |= buffer[3];
  if(samples.ch0 & 0x800000) {
    samples.ch0 |= 0xFF000000;
  }

  samples.ch1 = buffer[4] << 16;
  samples.ch1 |= buffer[5] << 8;
  samples.ch1 |= buffer[6];
  if(samples.ch1 & 0x800000) {
    samples.ch1 |= 0xFF000000;
  }

  samples.ch2 = buffer[7] << 16;
  samples.ch2 |= buffer[8] << 8;
  samples.ch2 |= buffer[9];
  if(samples.ch2 & 0x800000) {
    samples.ch2 |= 0xFF000000;
  }

  samples.ch3 = buffer[10] << 16;
  samples.ch3 |= buffer[11] << 8;
  samples.ch3 |= buffer[12];
  if(samples.ch3 & 0x800000) {
    samples.ch3 |= 0xFF000000;
  }


  PRINTF("IRQ Status: %d\n", samples.irq_status);

  *lux = SI1133_getLux(samples.ch1, samples.ch3, samples.ch2, &lk);
  *lux = (*lux * 1000) / (1 << LUX_OUTPUT_FRACTION);
  *uv = SI1133_getUv(samples.ch0, uk);
  *uv = (*uv * 1000) / (1 << UV_OUTPUT_FRACTION);

  i2c_release(&lux_sens);

  return 0;
}
