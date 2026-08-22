# Why VCOM Inversion is Necessary

Sharp Memory LCDs, like many other liquid crystal displays, require regular VCOM (common voltage) inversion to prevent permanent damage from DC bias buildup.

## What is LCD DC Bias Damage?

DC bias occurs when a constant voltage (rather than an alternating voltage) is applied across the liquid crystal material in an LCD. In Sharp Memory LCDs, this happens when the VCOM polarity isn't regularly inverted.

### The Damage Mechanism

**Electrolytic Degradation:** When DC voltage is continuously applied:

- Ion migration occurs within the liquid crystal material.
- Charged particles accumulate at one side of the crystal structure.
- This buildup creates permanent chemical changes in the liquid crystal alignment.
- The crystal's ability to switch between transparent and opaque states becomes impaired.

### Visible Effects

- **Image Retention/Burn-in:** Static images become permanently "ghosted" on the display.
- **Reduced Contrast:** The display becomes less able to show clear differences between light and dark areas.
- **Flickering:** Damaged crystals may flicker or show inconsistent behavior.
- **Complete Panel Failure:** In severe cases, sections of the display stop responding entirely.

## The Importance of a Balanced Duty Cycle

Simply inverting the VCOM signal is not enough; the inversion must maintain a nearly perfect 50% duty cycle over time. A balanced duty cycle ensures that the time spent with a positive voltage across the liquid crystal exactly equals the time spent with a negative voltage. 

If the duty cycle is unbalanced (e.g., spending 60% of the time in one polarity and 40% in the other), a net DC bias will still accumulate over time. While the degradation will be slower than if no inversion was taking place, it will still eventually lead to the same permanent electrolytic damage described above. Maintaining a balanced duty cycle is critical for maximizing the longevity and visual quality of the display.

## Observed Benefits in Testing

Testing the VCOM inversion implementation with the most current ZMK release (as of September 2025) across several display models has yielded the following results:

- **LS027B7DH01A:** Greatly improved contrast and significantly reduced power-off ghosting.
- **LS013B7DH05:** Noticeable improvement in contrast, though not as pronounced as the LS027B7DH01A.
- **LS011B7DH03:** Shows some improvement in contrast, but the difference was marginal (likely because the display tested was much newer and had less accumulated DC bias).

Across all models, older displays show the most dramatic improvement in contrast and reduction in image retention (ghosting) after running with proper VCOM inversion for even a short period of time. There was no image corruption or apparent bus fighting observed when using the updated driver.


