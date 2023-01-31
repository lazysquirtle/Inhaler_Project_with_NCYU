#ifndef _NRF52_TWI_FUNC_H
#define _NRF52_TWI_FUNC_H

#include <stdint.h>
#include "nrf_drv_twi.h"

uint8_t nrf52_twi_read(nrf_drv_twi_t const *p , uint8_t addr, uint8_t reg, uint8_t *data, uint8_t size);
uint8_t nrf52_twi_write(nrf_drv_twi_t const *p , uint8_t addr, uint8_t reg, uint8_t *data, uint8_t size);

#endif