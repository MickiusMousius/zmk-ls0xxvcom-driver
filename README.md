# z-boards

zephyr board definitions, drivers, and shell for coquette

## revisions

- `coquette@mariposa` is the final butterfly body board design - this doesn't cover the two prototypes, those may be added later to support our prototype versions
- `coquette@nightingale` is the rounded bird-body board design that replaces mariposa

## shell

yes there's a shell

## ti lp5814

i'm just starting work on this, the readme is also my notes

### shell commands to test

enable device:
```
i2c write_byte i2c1 0x2c 0x0 0x1
```

disable device:
```
i2c write_byte i2c1 0x2c 0x0 0x0
```

check engine status:
```
i2c read_byte i2c1 0x2c 0x40
```

reset undervoltage lockout/por:
```
i2c write_byte i2c1 0x2c 0x13 0x1
```

reset thermal shutdown:
```
i2c write_byte i2c1 0x2c 0x13 0x2
```

max current 25.5mA (should be default, do not set 51mA!):
```
i2c write_byte i2c1 0x2c 0x1 0x0
```

set dot current for all four channels to 18mA:
```
i2c write_byte i2c1 0x2c 0x14 0xb5
i2c write_byte i2c1 0x2c 0x15 0xb5
i2c write_byte i2c1 0x2c 0x16 0xb5
i2c write_byte i2c1 0x2c 0x17 0xb5
```

enable all four leds:
```
i2c write_byte i2c1 0x2c 0x02 0x0f
```

set all four channels to exponential pwm with animation disabled:
```
i2c write_byte i2c1 0x2c 0x04 0xf0
```

save config:
```
i2c write_byte i2c1 0x2c 0x0f 0x55
```

set LEDs to 100% of configured current (18mA with settings above):
```
i2c write_byte i2c1 0x2c 0x18 0xff
i2c write_byte i2c1 0x2c 0x19 0xff
i2c write_byte i2c1 0x2c 0x1a 0xff
i2c write_byte i2c1 0x2c 0x1b 0xff
```

