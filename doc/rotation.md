# Hardware Rotation (`rotate-180`)

Depending on how your keyboard's PCB is designed and how the display is physically mounted, the image on your screen might appear upside down by default. The ZMK LS0XX driver provides a `rotate-180` device tree property to flip the display output seamlessly at the driver level.

## Why use driver-level rotation?

You might be tempted to use a graphics library like LVGL to rotate your UI elements (e.g., using `lv_canvas_set_angle`). However, performing rotation at the software UI level introduces significant downsides compared to using the driver's native `rotate-180` feature.

### 1. Performance and CPU Overhead
Software rotation requires the MCU to perform mathematical transformations (matrix multiplication, pixel coordinate recalculation) for every pixel rendered on the screen. This drastically increases the CPU time required to render each frame. In contrast, driver-level rotation simply reverses the order in which the memory buffer is flushed to the display (or how the SPI transaction is structured), adding virtually zero computational overhead.

### 2. Power Consumption
Because software rotation forces the CPU to stay active longer to calculate pixel transformations, it consumes significantly more power. For a wireless, battery-powered keyboard, minimizing active CPU time is essential for maximizing battery life. Driver-level rotation avoids this extra work, allowing the MCU to return to deep sleep much faster.

### 3. Code Complexity and RAM Usage
Implementing rotation in your UI code requires managing transformed coordinates, updating canvas objects, and potentially allocating additional RAM for rotation buffers (since in-place rotation is often not supported or very inefficient in graphics libraries). Using the simple `rotate-180;` flag in your device tree keeps your UI code clean, straightforward, and memory-efficient.

## Theoretical Comparison: LVGL vs Driver Rotation (vista508)

To illustrate the difference, here is a theoretical comparison of rotating the entire UI 180 degrees on a **vista508** (144x168 resolution) display:

### RAM Usage
- **LVGL Software Rotation:** Graphics libraries generally require a secondary destination buffer to process rotation without visual tearing. Furthermore, many transformation algorithms require expanding 1-bit monochrome data into 8-bit index arrays to process efficiently. An 8-bit rotation buffer for a 144x168 canvas consumes **~24 KB of RAM**. Even if highly optimized to a strict 1-bit double buffer, it still consumes an extra **~3 KB**.
- **Driver `rotate-180`:** The driver rotates the display natively by simply reversing the indexing order when packing the final DMA buffer. This requires **0 extra bytes of RAM**.

### Compute (CPU Time)
- **LVGL Software Rotation:** To rotate a 144x168 canvas, LVGL must calculate coordinate transformations and read/write all **24,192 pixels** individually in memory. Depending on the MCU clock speed (e.g., 64 MHz on nRF52840), this memory thrashing can easily add several milliseconds of active CPU time to every single frame update.
- **No Rotation (0 Degrees):** The driver uses a highly optimized `memcpy` to pack the 18 bytes of pixel data per line into the DMA buffer. Across 168 lines, this takes roughly **~2,000 clock cycles** (less than 0.05 ms at 64 MHz).
- **Driver `rotate-180`:** Because 1-bit monochrome displays require the actual bits within each byte to be flipped left-to-right, the driver cannot just use `memcpy`. It uses an inline loop that processes 4-byte words at a time, applying a bit-reversal algorithm (`ls0xx_bitreverse32`). For 18 bytes per line, this requires 4 word reversals and 2 byte reversals per line. Across 168 lines, this evaluates to roughly **~18,500 clock cycles**. At 64 MHz, this adds only about **~0.25 ms** of extra compute time per frame compared to no rotation—a tiny penalty that is still orders of magnitude faster and vastly more battery-efficient than LVGL software rotation.
