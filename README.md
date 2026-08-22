# How is this driver different to the one I already get with ZMK/Zephyr?

ZMK/main & newer Zephyr version support VCOM inversion in the ls0xx driver.

Older versions of ZMK (e.g. 0.3) do not satisfy the strict VCOM inversion requirements for Sharp Memory LCDs, potentially causing electrolytic degradation over time.

This custom driver prevents display damage for older ZMK versions and adds several performance and battery optimizations:

- **[Balanced VCOM Inversion](doc/vcom_inversion.md):** Maintains a strict 50% duty cycle to prevent DC bias capacitive buildup and permanent display damage.
- **[DMA Batching (`dma-mode`)](doc/dma_mode.md):** Assembles frames in RAM and transmits them in a single hardware DMA call, cutting active CPU interrupt overhead by over 99%.
- **[Dual VCOM Intervals](doc/inversion_intervals.md):** Allows configuring a fast VCOM refresh while the screen is active to prevent flicker, and a slow refresh while idle to save battery.
- **[Hardware Rotation (`rotate-180`)](doc/rotation.md):** Flips the display output natively with practically zero overhead, avoiding the massive CPU and RAM penalties of conventional software UI rotation.


# How to use the driver

Add the following to your `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: MickiusMousius
      url-base: https://github.com/MickiusMousius
  projects:
    - name: zmk-ls0xxvcom-driver
      remote: MickiusMousius
      revision: main
```

If this is all you do, the new VCOM inversion signals will not be getting sent to your LCD yet—you will need to update your driver configuration too.

```c
&vista508_spi {
  status = "okay";
  vista508: ls0xx@0 {
    compatible = "sharp,ls0xx-vcom";
    spi-max-frequency = <2000000>;
    serial-vcom-inversion;
    serial-vcom-interval = <17>;
    idle-vcom-interval = <100>;
    dma-mode;
    rotate-180;
    reg = <0>;
    width = <144>;
    height = <168>;
  };
};
```

The following five new lines have been added compared to the usual ls0xx driver:

```c
serial-vcom-inversion;
serial-vcom-interval = <17>;
idle-vcom-interval = <100>;
dma-mode;
rotate-180;
```

- The first line enables the inversion fix. Information on why you'd want this is [here](doc/vcom_inversion.md).
- The second line (`serial-vcom-interval`) is the inversion interval in milliseconds used when the screen is **active and visible**. To eliminate flickering, an interval of 17-33ms should be used.
- The third line (`idle-vcom-interval`) is the inversion interval used when the screen is **blanked (idle)**. This defaults to 1000ms if omitted. Information on choosing a suitable inversion interval can be found [here](doc/inversion_intervals.md).
- The fourth line (`dma-mode`) enables **SPI Batching**. This allocates a single buffer in RAM to format the entire screen frame at once, allowing the DMA hardware to send it to the display in a single transaction while the CPU sleeps. Information about DMA mode can be found [here](doc/dma_mode.md).
- The fifth line (`rotate-180`) flips the display output natively in the driver, avoiding costly software rotation. Learn why this is critical for performance and battery life [here](doc/rotation.md).


# Attribution

The updated driver has been copied from the [coquette repository](https://ravy.dev/coquette/z-module-coquette) with express permission. The driver has been updated for use with Zephyr 3.5 whereas the coquette work has been done for use with Zephyr 4.1.

The original driver was taken from the [Zephyr project](https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/display/ls0xx.c).

Claude AI was used to proofread this readme.