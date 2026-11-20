

#include "stm32f4xx_hal.h"
#include <map>

class GPIO {
    public:
    enum class Port {
        B,
    };
    GPIO(Port port, uint16_t pin) {
        
        GPIO_InitTypeDef g = {0};

        __HAL_RCC_GPIOB_CLK_ENABLE();

        g.Pin   = GPIO_PIN_0 | GPIO_PIN_7 | GPIO_PIN_14;
        g.Mode  = GPIO_MODE_OUTPUT_PP;
        g.Pull  = GPIO_NOPULL;
        g.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(GPIOB, &g);

        g.Pin  = GPIO_PIN_13;
        g.Mode = GPIO_MODE_INPUT;
        g.Pull = GPIO_PULLDOWN;
        HAL_GPIO_Init(GPIOC, &g);
    }
    private:
        std::map<Port, GPIO_TypeDef*> portMapping;
};
