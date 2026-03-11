#include "comms/backend_init.hpp"
#include "config_defaults.hpp"
#include "core/CommunicationBackend.hpp"
#include "core/KeyboardMode.hpp"
#include "core/mode_selection.hpp"
#include "core/pinout.hpp"
#include "core/state.hpp"
#include "input/GpioButtonInput.hpp"
#include "reboot.hpp"
#include "stdlib.hpp"

#include <config.pb.h>

Config config = default_config;

// Button-to-pin mapping from stm32pintester Buttons.md.
// B0XX-style leverless layout — adjust after physical testing.
//
// Left hand (movement):
//   B1(PC0)   B2(PC2)   B3(PB8)
//   B4(PD2)   B5(PB11)  B6(PA2)
//   B7(PA0)   B8(PB15)  B9(PB13)  B10(PC5)
//   B11(PB15) B12(PB11)
//
// Right hand (attacks):
//   B13(PB0)  B14(PC8)  B15(PC9)  B16(PC10)  B17(PC11)
//   B18(PB9)  B19(PB7)  B20(PB5)  B21(PC3)
//   B22(PC12) B23(PA15) B24(PC6)  B25(PB14)
//   B26(PC7)  B27(PB4)
//
// Note: B5/B12 share PB11, B8/B11 share PB15 — only 25 unique pins for 27 buttons.
// Shared pins are mapped once (the first occurrence).

const GpioButtonMapping button_mappings[] = {
    // Left hand - movement
    { BTN_LF4, PC0  },  // B1  - Left
    { BTN_LF3, PA0  },  // B7  - Down
    { BTN_LF2, PC2  },  // B2  - Up
    { BTN_LF1, PB8  },  // B3  - Right

    // Left hand - thumbs
    { BTN_LT1, PB13 },  // B9  - ModX
    { BTN_LT2, PC5  },  // B10 - ModY

    // Left hand - extra
    { BTN_MB3, PD2  },  // B4
    { BTN_MB1, PB11 },  // B5  (shared with B12)
    { BTN_MB2, PA2  },  // B6

    // Right hand - top row
    { BTN_RT3, PC8  },  // B14
    { BTN_RT4, PC9  },  // B15
    { BTN_RT2, PC10 },  // B16
    { BTN_RT1, PC11 },  // B17
    { BTN_RT5, PB15 },  // B8  (shared with B11)

    // Right hand - attack buttons
    { BTN_RF1, PB9  },  // B18 - A (B)
    { BTN_RF2, PB7  },  // B19 - B (A)
    { BTN_RF3, PB5  },  // B20 - X (Y)
    { BTN_RF4, PC3  },  // B21 - Y (X)

    // Right hand - bottom rows
    { BTN_RF5, PB0  },  // B13
    { BTN_RF6, PC12 },  // B22
    { BTN_RF7, PA15 },  // B23
    { BTN_RF8, PC6  },  // B24

    // Remaining buttons
    { BTN_RF9, PB14 },  // B25
    { BTN_RF10, PC7 },  // B26
    { BTN_RF11, PB4 },  // B27
};
const size_t button_count = sizeof(button_mappings) / sizeof(GpioButtonMapping);

const Pinout pinout = {
    .joybus_data = 0,
    .nes_data = -1,
    .nes_clock = -1,
    .nes_latch = -1,
    .mux = -1,
    .nunchuk_detect = -1,
    .nunchuk_sda = -1,
    .nunchuk_scl = -1,
};

CommunicationBackend **backends = nullptr;
size_t backend_count;
KeyboardMode *current_kb_mode = nullptr;

void setup() {
    // Free PA15, PB3, PB4 from JTAG for GPIO use (keep SWD on PA13/PA14).
    afio_cfg_debug_ports(AFIO_DEBUG_SW_ONLY);

    // PA8 is the matrix row driver — drive LOW to activate the diode-to-ground matrix.
    pinMode(PA8, OUTPUT);
    digitalWrite(PA8, LOW);

    static InputState inputs;

    static GpioButtonInput gpio_input(button_mappings, button_count);
    gpio_input.UpdateInputs(inputs);

    // Check bootloader button hold as early as possible.
    if (inputs.rt2) {
        reboot_bootloader();
    }

    static InputSource *input_sources[] = { &gpio_input };
    size_t input_source_count = sizeof(input_sources) / sizeof(InputSource *);

    backend_count =
        initialize_backends(backends, inputs, input_sources, input_source_count, config, pinout);

    setup_mode_activation_bindings(config.game_mode_configs, config.game_mode_configs_count);
}

void loop() {
    select_mode(backends, backend_count, config);

    for (size_t i = 0; i < backend_count; i++) {
        backends[i]->SendReport();
    }

    if (current_kb_mode != nullptr) {
        current_kb_mode->SendReport(backends[0]->GetInputs());
    }
}
