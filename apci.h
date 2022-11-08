#pragma once

#include "logging.h"
#include "eNET-types.h"

__u8  in8(int offset);
__u16 in16(int offset);
__u32 in32(int offset);
__u32 in(int offset);
TError out(int offset, __u32 value);
TError out8(int offset, __u8 value);
TError out16(int offset, __u16 value);
TError out32(int offset, __u32 value);
