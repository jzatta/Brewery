#pragma once
#include "rxtx.hpp"
#include "stm32f4xx_hal.h"

/* HAL polling UART implementing the RxTx interface (no virtuals). */
class Uart : public RxTx<Uart> {
public:
    explicit Uart(UART_HandleTypeDef* h) : h_(h) {}

private:
    friend class RxTx<Uart>;

    bool writeImpl(const uint8_t* data, size_t len, uint32_t timeout_ms)
    {
        while (len) {
            const uint16_t n = len > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(len);
            if (HAL_UART_Transmit(h_, const_cast<uint8_t*>(data), n, timeout_ms) != HAL_OK)
                return false;
            data += n;
            len  -= n;
        }
        return true;
    }

    size_t readImpl(uint8_t* buf, size_t len, uint32_t timeout_ms)
    {
        size_t i = 0;
        while (i < len && HAL_UART_Receive(h_, &buf[i], 1, timeout_ms) == HAL_OK)
            ++i;
        return i;
    }

    void flushImpl()
    {
        uint8_t b;
        __HAL_UART_CLEAR_OREFLAG(h_);
        while (HAL_UART_Receive(h_, &b, 1, 0) == HAL_OK) {}
    }

    uint32_t millisImpl() { return HAL_GetTick(); }

    UART_HandleTypeDef* h_;
};
