#include "nrf_drv_twi.h"
#include "nrf_error.h"
#include "nrf_delay.h"
#include "string.h"

uint8_t nrf52_twi_read(nrf_drv_twi_t const *p , uint8_t addr, uint8_t reg, uint8_t *data, uint8_t size)
{
    ret_code_t ret;

    ret = nrf_drv_twi_tx(p, addr, &reg, 1, true);
    if(ret == NRF_SUCCESS) {
        ret = nrf_drv_twi_rx(p, addr, data, size);
    }
    if(ret != 0) {
        return ret;
    }

    return NRF_SUCCESS;
}

uint8_t nrf52_twi_write(nrf_drv_twi_t const *p , uint8_t addr, uint8_t reg, uint8_t *data, uint8_t size)
{
    ret_code_t ret;
    uint8_t buf[size+1];
    
    buf[0] = reg;    
    memcpy(&buf[1], data, size);

    ret = nrf_drv_twi_tx(p, addr, buf, size+1, false);
    if(ret != 0) {
        return ret;
    }

    return NRF_SUCCESS;
}
