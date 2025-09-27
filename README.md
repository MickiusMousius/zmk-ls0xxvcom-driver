# How is this driver different to the one I already get with ZMK/Zephyr?

It prevents display damage.

Sharp Memory LCDs require regular VCOM inversion to prevent permanent damage from DC bias buildup.

The original Zephyr implementation does not satisfy this requirement (As at September 2025), potentially causing electrolytic degradation of the liquid crystal material over time.

# Do I need to update to this driver right now?

Probably not.

There is a PR in the works with Zephyr to address this issue & the ZMK dev team is also aware of the issue. So long as you update your firmware once the fixes are back-ported, your display will *probably* be fine.

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
    spi-max-frequency = <1000000>;
    serial-vcom-inversion;
    serial-vcom-interval = <17>;
    reg = <0>;
    width = <144>;
    height = <168>;
  };
};
```

You will find this in your display's overlay file (e.g., `boards/vista508/vista508.overlay`).

The following two new lines have been added:

```c
serial-vcom-inversion;
serial-vcom-interval = <17>;
```

- The first line enables the inversion fix
- The second line is the number of milliseconds to wait between inversions

An interval of at most 1000ms should be used, this may produce some flickering. To eliminate flickering an interval of 17-33ms can be used.

A shorter refresh interval will produce a more contrasty image. It will also slightly increase power consumption.

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