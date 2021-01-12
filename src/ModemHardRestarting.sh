#!/bin/bash
echo "Setup mini PCIe boards..."
# switch off extension boards power
i2cset -f -y -m 0x40 0 0x20 1 0x40
usleep 13250
# switch on extension boards power
i2cset -f -y -m 0x40 0 0x20 1 0x0
echo "Setup mini PCIe boards... DONE."
