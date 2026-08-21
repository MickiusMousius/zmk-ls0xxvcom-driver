# How is this driver different to the one I already get with ZMK/Zephyr?

It prevents display damage.

Sharp Memory LCDs require regular VCOM inversion to prevent permanent damage from DC bias buildup.

The original Zephyr implementation does not satisfy this requirement (As at September 2025), potentially causing electrolytic degradation of the liquid crystal material over time.

# Do I need to update to this driver right now?

Probably not.

Mainline Zephyr has been updated to address this issue & the ZMK dhas backported the fix.

ZMK 0.3 does not yet have the fix, so if you are using ZMK 0.3 you will need to use this driver for now.

You've likely been using the unpatched driver for a long time. Initial testing shows that 2-year-old displays have any visible damage quickly reversed after using the new driver for a few minutes.

# What is LCD DC Bias Damage?

DC bias occurs when a constant voltage (rather than an alternating voltage) is applied across the liquid crystal material in an LCD. In Sharp Memory LCDs, this happens when the VCOM (common voltage) polarity isn't regularly inverted.

## The Damage Mechanism

**Electrolytic Degradation:** When DC voltage is continuously applied:

- Ion migration occurs within the liquid crystal material
- Charged particles accumulate at one side of the crystal structure
- This buildup creates permanent chemical changes in the liquid crystal alignment
- The crystal's ability to switch between transparent and opaque states becomes impaired

## Visible Effects

- **Image Retention/Burn-in:** Static images become permanently "ghosted" on the display
- **Reduced Contrast:** The display becomes less able to show clear differences between light and dark areas
- **Flickering:** Damaged crystals may flicker or show inconsistent behavior
- **Complete Panel Failure:** In severe cases, sections of the display stop responding entirely

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
    reg = <0>;
    width = <144>;
    height = <168>;
  };
};
```

You will find this in your display's overlay file (e.g., `boards/vista508/vista508.overlay`).

The following four new lines have been added:

```c
serial-vcom-inversion;
serial-vcom-interval = <17>;
idle-vcom-interval = <100>;
dma-mode;
```

- The first line enables the inversion fix
- The second line (`serial-vcom-interval`) is the inversion interval in milliseconds used when the screen is **active and visible**. To eliminate flickering, an interval of 17-33ms should be used.
- The third line (`idle-vcom-interval`) is the inversion interval used when the screen is **blanked (idle)**. This defaults to 1000ms if omitted.
- The fourth line (`dma-mode`) enables **SPI Batching**. This allocates a single buffer in RAM to format the entire screen frame at once, allowing the DMA hardware to send it to the display in a single transaction while the CPU sleeps.

### Performance: Why `dma-mode` matters

Without `dma-mode`, the driver sends pixel data to the display one line at a time. This means the number of individual SPI transactions per frame is equal to your display's pixel height (e.g., 168 separate transactions for a 144x168 display). This forces the MCU to wake up and handle hardware interrupts for every single line.

Enabling `dma-mode` reduces this to **1 transaction per frame**, cutting the CPU interrupt overhead by over 99% regardless of your display size.

**Pros:**
* Drastically reduces active CPU time during display updates.
* Increases battery life by allowing the CPU to sleep while the DMA hardware handles the SPI transfer.
* Reduces bus contention if other devices share the SPI bus.

**Cons:**
* Increases static RAM (SRAM) usage by roughly `(Display Height) * (Display Width in Bytes + 2)` bytes. For a 144x168 display, this uses about 3.3 KB of RAM. (This is generally insignificant on modern microcontrollers like the nRF52840 which has 256 KB of RAM, but could be a factor on severely RAM-constrained chips).

### Power Analysis: Why `idle-vcom-interval` matters

When you see a faint pulsing at 1Hz, it's because the liquid crystals in the display are being inverted slowly. Speeding up the inversion reduces this visual flicker, but it means waking up the MCU more often.

It's a common misconception that the CPU uses power based only on how *long* it works. In modern microcontrollers like the nRF52, the actual work of toggling the VCOM bit takes less than **0.1 milliseconds**. The real power drain comes from the **wakeup penalty** (spinning up the high-frequency clock and regulators), which creates a current spike of roughly ~3-5 mA for about 1ms.

By dropping the VCOM frequency when the screen is blanked, you drastically increase the ratio of deep sleep:

![Time Spent in Deep Sleep vs VCOM Interval](doc/sleep_ratio.png)

Assuming deep sleep draws ~10µA and a wakeup spike averages 4mA for 1ms, here is the impact on your average VCOM power draw:

![Average VCOM Power Draw](doc/power_draw.png)

**The Sweet Spot:** We recommend an `idle-vcom-interval` of `<100>` (or 200). At 100ms, any faint pulsing on a blank screen blends together enough to mostly disappear, while cutting your background battery drain by more than half compared to running at 33ms constantly.

# Testing Performed So Far

I have tested this code using the most current ZMK release as of Septmberr 2025, here are the results of that testing:
 * LS027B7DH01A - Works a treat, greatly improves contrast
 * LS013B7DH05 - Works a treat, noticeable improvement in contrast, not as pronounced as above display type
 * LS011B7DH03 - Works well, some improvement in contrast but difference is marginal (the display being tested was much newer than the other two displays)

I have observed that old displays improve contrast & have less power off ghosting after running with this update.

Display flickering is noticeable on LS013B7DH05 & LS027B7DH01A at anything less than 33ms refresh interval when viewed off axis. Using an interval of 17ms produces perfect results with excellent contrast & no flickering.

There is no image corruption or apparent bus fighting when using this updated driver.

Higher refresh rates do produce higher energy consumption, however this was tested to be 80uA in the worst case.

# Attribution

The updated driver has been copied from the [coquette repository](https://ravy.dev/coquette/z-module-coquette) with express permission. The driver has been updated for use with Zephyr 3.5 whereas the coquette work has been done for use with Zephyr 4.1.

The original driver was taken from the [Zephyr project](https://github.com/zephyrproject-rtos/zephyr/blob/main/drivers/display/ls0xx.c).

Claude AI was used to proofread this readme.