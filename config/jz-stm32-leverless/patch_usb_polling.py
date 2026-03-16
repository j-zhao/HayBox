Import("env")
import os, re

# Patch USBComposite usb_x360w.c to set bInterval=1 (1ms/1000Hz polling).
# The library default is 4ms (250Hz), which causes noticeable input latency.

libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
target_file = os.path.join(libdeps, env["PIOENV"],
                           "USBComposite for STM32F1", "usb_x360w.c")

if os.path.isfile(target_file):
    with open(target_file, "r") as f:
        src = f.read()
    patched = re.sub(r"(\.bInterval\s*=\s*)[48](,)", r"\g<1>1\2", src)
    if patched != src:
        with open(target_file, "w") as f:
            f.write(patched)
        print("Patched usb_x360w.c: bInterval → 1ms (1000Hz)")
