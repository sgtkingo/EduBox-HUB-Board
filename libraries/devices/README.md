# EduBox devices

This library contains all concrete EduBox sensor and actuator implementations.
It depends on `edubox-engine`; hardware-driver dependencies are declared in
`library.json`.

Include every registered device with `#include <devices.hpp>`, or include an
individual device header to reduce coupling.
