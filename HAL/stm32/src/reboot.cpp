#include "reboot.hpp"

#include "stdlib.hpp"

#include <libmaple/nvic.h>

void reboot_firmware() {
    nvic_sys_reset();
}

void reboot_bootloader() {
    // TODO: Set magic word at end of RAM to enter system bootloader after reset.
    // For now, just reset into firmware.
    nvic_sys_reset();
}
