# Copyright (c) 2025 eden ariel and ember ana
# SPDX-License-Identifier: EUPL-1.2

zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_DISPLAY_LS0XX_VCOM ls0xx_vcom.c)