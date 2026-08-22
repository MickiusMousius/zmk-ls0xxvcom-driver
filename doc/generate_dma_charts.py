#!/usr/bin/env python3
import matplotlib.pyplot as plt
import os

# Set up the plot style
try:
    plt.style.use('ggplot')
except:
    pass # fallback if ggplot is not available

# Chart 1: CPU Active Time
labels1 = ['Line-by-Line\n(No DMA)', 'Batch Mode\n(With DMA)']
values1 = [3360, 50]

fig, ax = plt.subplots(figsize=(6, 4))
bars = ax.bar(labels1, values1, color=['#e24a33', '#348abd'], width=0.5)
ax.set_ylabel('CPU Active Time (µs)')
ax.set_title('CPU Active Time per Frame Update\n(144x168 Display)')
ax.set_ylim(0, 4000)

for bar in bars:
    yval = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2, yval + 100, f'{yval} µs', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
plt.savefig('dma_cpu_time.png', dpi=150, bbox_inches='tight')
plt.close()

# Chart 2: Average Current Draw
labels2 = ['Line-by-Line\n(No DMA)', 'Batch Mode\n(With DMA)']
values2 = [4.5, 1.2]

fig, ax = plt.subplots(figsize=(6, 4))
bars = ax.bar(labels2, values2, color=['#e24a33', '#348abd'], width=0.5)
ax.set_ylabel('Current (mA)')
ax.set_title('Average Current Draw During\nSPI Transfer')
ax.set_ylim(0, 5.5)

for bar in bars:
    yval = bar.get_height()
    ax.text(bar.get_x() + bar.get_width()/2, yval + 0.15, f'{yval} mA', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
plt.savefig('dma_current_draw.png', dpi=150, bbox_inches='tight')
plt.close()

print("Charts successfully generated: dma_cpu_time.png and dma_current_draw.png")
