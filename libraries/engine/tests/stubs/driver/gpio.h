#pragma once
#define GPIO_IS_VALID_GPIO(pin) ((pin) >= 0 && (pin) < 49 && ((pin) < 22 || (pin) > 25))
#define GPIO_IS_VALID_OUTPUT_GPIO(pin) GPIO_IS_VALID_GPIO(pin)
