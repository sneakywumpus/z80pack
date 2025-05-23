/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2020 Damien P. George
 * Copyright (c) 2024-2025 Thomas Eberhardt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef _STDIO_MSC_USB_TUSB_CONFIG_H
#define _STDIO_MSC_USB_TUSB_CONFIG_H

#include "stdio_msc_usb.h"

#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE)

#if STDIO_MSC_USB_DISABLE_STDIO
#define CFG_TUD_CDC             (2)
#define STDIO_MSC_USB_CONSOLE2_ITF 0
#define STDIO_MSC_USB_PRINTER_ITF 1
#else
#define CFG_TUD_CDC             (3)
#define STDIO_MSC_USB_CONSOLE_ITF 0
#define STDIO_MSC_USB_CONSOLE2_ITF 1
#define STDIO_MSC_USB_PRINTER_ITF 2
#endif

// CDC FIFO size of TX and RX
#ifndef CFG_TUD_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_RX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif
#ifndef CFG_TUD_CDC_TX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif

// CDC Endpoint transfer buffer size, more is faster
#ifndef CFG_TUD_CDC_EP_BUFSIZE
#define CFG_TUD_CDC_EP_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif

#define CFG_TUD_MSC             (1)
#define CFG_TUD_MSC_EP_BUFSIZE  (4096)

// We use a vendor specific interface but with our own driver
// Vendor driver only used for Microsoft OS 2.0 descriptor
#if !STDIO_MSC_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#define CFG_TUD_VENDOR            (0)
#else
#define CFG_TUD_VENDOR            (1)
#define CFG_TUD_VENDOR_RX_BUFSIZE  (256)
#define CFG_TUD_VENDOR_TX_BUFSIZE  (256)
#endif

#endif
