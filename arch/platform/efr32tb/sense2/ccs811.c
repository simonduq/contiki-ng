/***************************************************************************//**
 * @file ccs811.c
 * @brief Driver for the Cambridge CMOS Sensors CCS811 gas and indoor air
 * quality sensor
 * @version 5.3.5
 *******************************************************************************
 * # License
 * <b>Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silicon Labs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "dev/i2c.h"
#include "em_cmu.h"
#include "em_i2c.h"
#include "ccs811.h"
#include "board.h"
#include "clock.h"

/**************************************************************************//**
* @defgroup CCS811 CCS811 - Indoor Air Quality Sensor
* @{
* @brief Driver for the Cambridge CMOS Sensors CCS811 gas and indoor air
* quality sensor
******************************************************************************/

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

extern i2c_device_t ccs811_dev;

static uint32_t CCS811_setAppStart(void);

/** @endcond */

/***************************************************************************//**
 * @brief
 *    Initializes the chip and performs firmware upgrade if required
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_init(void)
{
  uint8_t id;
  uint32_t status;

  /* Enable the sensor and wake it up */
  board_gas_sensor_enable(true);
  board_gas_sensor_wake(true);

  /* About 80 ms required to reliably start the device, wait a bit more */
  clock_delay(CLOCK_SECOND / 10);

  /* Check if the chip present and working by reading the hardware ID */
  status = CCS811_getHardwareID(&id);
  if ( (status != CCS811_OK) && (id != CCS811_HW_ID) ) {
    return CCS811_ERROR_INIT_FAILED;
  }

  /* /\* Go back to sleep *\/ */
  board_gas_sensor_wake(false);

  return CCS811_OK;
}

/***************************************************************************//**
 * @brief
 *    De-initializes the chip
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_deInit(void)
{
  /* Disable the sensor  */
  board_gas_sensor_wake(false);
  board_gas_sensor_enable(false);

  return CCS811_OK;
}

/***************************************************************************//**
 * @brief
 *    Reads Hardware ID from the CSS811 sensor
 *
 * @param[out] hardwareID
 *    The Hardware ID of the chip (should be 0x81)
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_getHardwareID(uint8_t *hardwareID)
{
  uint32_t result;

  result = CCS811_readMailbox(CCS811_ADDR_HW_ID, 1, hardwareID);

  return result;
}

/**************************************************************************//**
* @brief
*    Reads the status of the CSS811 sensor
*
* @param[out] status
*    The content of the CSS811 Status Register
*
* @return
*    Returns zero on OK, non-zero otherwise.
******************************************************************************/
uint32_t CCS811_getStatus(uint8_t *status)
{
  uint32_t result;

  result = CCS811_readMailbox(CCS811_ADDR_STATUS, 1, status);

  return result;
}

/***************************************************************************//**
 * @brief
 *    Checks if new measurement data available
 *
 * @return
 *    True if new data available, otherwise false
 ******************************************************************************/
bool CCS811_isDataAvailable(void)
{
  bool state;
  uint32_t status;
  uint8_t reg;

  state = false;
  /* Read the status register */
  status = CCS811_getStatus(&reg);

  /* Check if the DATA_READY bit is set */
  if ( (status == CCS811_OK) && ( (reg & 0x08) == 0x08) ) {
    state = true;
  }

  return state;
}

/***************************************************************************//**
 * @brief
 *    Dumps the registers of the CSS811
 *
 * @return
 *    None
 ******************************************************************************/
void CCS811_dumpRegisters(void)
{
  uint8_t buffer[8];

  CCS811_readMailbox(CCS811_ADDR_STATUS, 1, buffer);
  printf("STATUS       : %02X\r\n", buffer[0]);

  CCS811_readMailbox(CCS811_ADDR_MEASURE_MODE, 1, buffer);
  printf("MEASURE_MODE : %02X\r\n", buffer[0]);

  CCS811_readMailbox(CCS811_ADDR_ALG_RESULT_DATA, 4, buffer);
  printf("ALG_DATA     : %02X%02X  %02X%02X\r\n", buffer[0], buffer[1], buffer[2], buffer[3]);

  CCS811_readMailbox(CCS811_ADDR_RAW_DATA, 2, buffer);
  printf("RAW_DATA     : %02X%02X\r\n", buffer[0], buffer[1]);

  CCS811_readMailbox(CCS811_ADDR_ENV_DATA, 4, buffer);
  printf("ENV_DATA     : %02X%02X  %02X%02X\r\n", buffer[0], buffer[1], buffer[2], buffer[3]);

  CCS811_readMailbox(CCS811_ADDR_NTC, 4, buffer);
  printf("NTC          : %02X%02X  %02X%02X\r\n", buffer[0], buffer[1], buffer[2], buffer[3]);

  CCS811_readMailbox(CCS811_ADDR_THRESHOLDS, 4, buffer);
  printf("THRESHOLDS   : %02X%02X  %02X%02X\r\n", buffer[0], buffer[1], buffer[2], buffer[3]);

  CCS811_readMailbox(CCS811_ADDR_HW_ID, 1, buffer);
  printf("HW_ID        : %02X\r\n", buffer[0]);

  CCS811_readMailbox(CCS811_ADDR_HW_VERSION, 1, buffer);
  printf("HW_VERSION   : %02X\r\n", buffer[0]);

  CCS811_readMailbox(CCS811_ADDR_FW_BOOT_VERSION, 2, buffer);
  printf("BOOT_VERSION : %d.%d.%d\r\n", (buffer[0] >> 4) & 0xF, buffer[0] & 0xF, buffer[1]);

  CCS811_readMailbox(CCS811_ADDR_FW_APP_VERSION, 2, buffer);
  printf("APP_VERSION  : %d.%d.%d\r\n", (buffer[0] >> 4) & 0xF, buffer[0] & 0xF, buffer[1]);

  CCS811_readMailbox(CCS811_ADDR_ERR_ID, 1, buffer);
  printf("ERR_ID       : %02X\r\n", buffer[0]);

  return;
}

/***************************************************************************//**
 * @brief
 *    Switches the CSS811 chip from boot to application mode
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_startApplication(void)
{
  uint32_t result;
  uint8_t status;

  /* Read status */
  result = CCS811_readMailbox(CCS811_ADDR_STATUS, 1, &status);

  /* If no application firmware present in the CCS811 then return an error message */
  if ( (status & 0x10) != 0x10 ) {
    return CCS811_ERROR_APPLICATION_NOT_PRESENT;
  }

  /* Issue APP_START */
  result += CCS811_setAppStart();

  /* Check status again */
  result = CCS811_readMailbox(CCS811_ADDR_STATUS, 1, &status);

  /* If the chip firmware did not switch to application mode then return with error */
  if ( (status & 0x90) != 0x90 ) {
    return CCS811_ERROR_NOT_IN_APPLICATION_MODE;
  }

  return result;
}

/***************************************************************************//**
 * @brief
 *    Reads data from a specific Mailbox address
 *
 * @param[in] id
 *    The address of the Mailbox register
 *
 * @param[in] length
 *    The number of bytes to read
 *
 * @param[out] data
 *    The data read from the sensor
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_readMailbox(uint8_t id, uint8_t length, uint8_t *data)
{
  uint32_t retval;

  retval = CCS811_OK;

  board_gas_sensor_wake(true);

  i2c_acquire(&ccs811_dev);

  retval = i2c_read_register(&ccs811_dev, id, data, length);

  board_gas_sensor_wake(false);

  return retval;
}

/***************************************************************************//**
 * @brief
 *    Reads measurement data (eCO2 and TVOC) from the CSS811 sensor
 *
 * @param[out] eco2
 *    The eCO2 level read from the sensor
 *
 * @param[out] tvoc
 *    The TVOC level read from the sensor
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_getMeasurement(uint16_t *eco2, uint16_t *tvoc)
{
  uint8_t buffer[4];
  uint32_t retval;

  retval = CCS811_OK;

  *eco2 = 0;
  *tvoc = 0;

  /* Read four bytes from the ALG_RESULT_DATA mailbox register */

  /* BOARD_i2cBusSelect(BOARD_I2C_BUS_SELECT_GAS); */
  i2c_acquire(&ccs811_dev);

  retval = i2c_read_register(&ccs811_dev, CCS811_ADDR_ALG_RESULT_DATA, buffer, 4);

  /* Convert the read bytes to 16 bit values */
  *eco2 = ( (uint16_t) buffer[0] << 8) + (uint16_t) buffer[1];
  *tvoc = ( (uint16_t) buffer[2] << 8) + (uint16_t) buffer[3];

  i2c_release(&ccs811_dev);

  return retval;
}

/***************************************************************************//**
 * @brief
 *    Performs software reset on the CCS811
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_softwareReset(void)
{
  uint8_t i2c_write_data[5];
  uint32_t retval;

  retval = CCS811_OK;

  board_gas_sensor_wake(true);

  /* Write the 0x11 0xE5 0x72 0x8A key sequence to software reset register */
  /* The key sequence is used to prevent accidental reset                  */
  /*  i2c_write_data[0] = CCS811_ADDR_SW_RESET; */
  i2c_write_data[0] = 0x11;
  i2c_write_data[1] = 0xE5;
  i2c_write_data[2] = 0x72;
  i2c_write_data[3] = 0x8A;

  i2c_acquire(&ccs811_dev);
  retval = i2c_write_register_buf(&ccs811_dev, CCS811_ADDR_SW_RESET, i2c_write_data, 4);

  board_gas_sensor_wake(false);

  return retval;
}

/***************************************************************************//**
 * @brief
 *    Sets the measurement mode of the CSS811 sensor
 *
 * @param[in] measMode
 *    The desired measurement mode
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_setMeasureMode(uint8_t measMode)
{
  uint32_t retval;

  retval = CCS811_OK;

  board_gas_sensor_wake(true);

  /* Bits 7,6,2,1 and 0 are reserved, clear them */
  measMode = (measMode & 0x38);

  /* Write to the measurement mode register      */

  i2c_acquire(&ccs811_dev);

  retval = i2c_write_register(&ccs811_dev, CCS811_ADDR_MEASURE_MODE, measMode);

  i2c_release(&ccs811_dev);
  board_gas_sensor_wake(false);

  return retval;
}

/** @cond DO_NOT_INCLUDE_WITH_DOXYGEN */

/***************************************************************************//**
 * @brief
 *    Changes the mode of the CCS811 from Boot mode to running the application
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
static uint32_t CCS811_setAppStart(void)
{
  uint8_t buf;
  uint32_t retval;

  board_gas_sensor_wake(true);

  /* Perform a write with no data to the APP_START register to change the */
  /* state from boot mode to application mode                             */
  buf = CCS811_ADDR_APP_START;

  i2c_acquire(&ccs811_dev);

  retval = i2c_write(&ccs811_dev, &buf, 1);

  if(retval == I2C_BUS_STATUS_OK) {
    retval = CCS811_OK;
  } else {
    retval = CCS811_ERROR_NOT_IN_APPLICATION_MODE;
  }

  i2c_release(&ccs811_dev);
  board_gas_sensor_wake(false);

  return retval;
}

/** @endcond */

/***************************************************************************//**
 * @brief
 *    Writes temperature and humidity values to the environmental data regs
 *
 * @param[in] tempData
 *    The environmental temperature in milli-Celsius
 *
 * @param[in] rhData
 *    The relative humidity in milli-percent
 *
 * @return
 *    Returns zero on OK, non-zero otherwise
 ******************************************************************************/
uint32_t CCS811_setEnvData(int32_t tempData, uint32_t rhData)
{
  uint8_t i2c_write_data[4];
  uint8_t humidityRegValue;
  uint8_t temperatureRegValue;
  uint32_t retval;

  board_gas_sensor_wake(true);

  /* The CCS811 currently supports only 0.5% resolution        */
  /* If the fraction greater than 0.7 then round up the value  */
  /* Shift to the left by one to meet the required data format */
  if ( ( (rhData % 1000) / 100) > 7 ) {
    humidityRegValue = (rhData / 1000 + 1) << 1;
  } else {
    humidityRegValue = (rhData / 1000) << 1;
  }

  /* If the fraction is greater than 0.2 or less than 0.8 set the            */
  /* LSB bit, which is the most significant bit of the fraction 2^(-1) = 0.5 */
  if ( ( (rhData % 1000) / 100) > 2 && ( ( (rhData % 1000) / 100) < 8) ) {
    humidityRegValue |= 0x01;
  }

  /* Add +25 C to the temperature value                        */
  /* The CCS811 currently supports only 0.5C resolution        */
  /* If the fraction greater than 0.7 then round up the value  */
  /* Shift to the left by one to meet the required data format */
  tempData += 25000;
  if ( ( (tempData % 1000) / 100) > 7 ) {
    temperatureRegValue = (tempData / 1000 + 1) << 1;
  } else {
    temperatureRegValue = (tempData / 1000) << 1;
  }

  /* If the fraction is greater than 0.2 or less than 0.8 set the            */
  /* LSB bit, which is the most significant bit of the fraction 2^(-1) = 0.5 */
  if ( ( (tempData % 1000) / 100) > 2 && ( ( (tempData % 1000) / 100) < 8) ) {
    temperatureRegValue |= 0x01;
  }

  /* Write the correctly formatted values to the environmental data register */
  /* The LSB bytes of the humidity and temperature data are 0x00 because     */
  /* the current CCS811 supports only 0.5% and 0.5C resolution               */
  i2c_write_data[0] = humidityRegValue;
  i2c_write_data[1] = 0x00;
  i2c_write_data[2] = temperatureRegValue;
  i2c_write_data[3] = 0x00;

  i2c_acquire(&ccs811_dev);

  retval = CCS811_OK;

  retval = i2c_write_register_buf(&ccs811_dev, CCS811_ADDR_ENV_DATA,
                                  i2c_write_data, 4);

  i2c_release(&ccs811_dev);

  board_gas_sensor_wake(false);

  return retval;
}
/** @} (end defgroup CCS811) */
/** @} {end addtogroup TBSense_BSP} */
