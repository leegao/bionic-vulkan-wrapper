// MIT License
// Copyright (c) 2026 Lee Gao

// Design Doc: https://docs.google.com/document/d/1lyu7vRDGmXe3G0DedLf9aiBVK9pElZ9lzdGqAUd0B8o/edit?usp=sharing

[[vk::binding(0, 0)]]
StructuredBuffer<uint> input_rgba8_buffer;

[[vk::binding(1, 0)]]
RWStructuredBuffer<uint4> etc1_blocks;

#ifdef ENABLE_DIAGNOSTICS
// Not available on Mali/Adreno :(
__glsl_extension(GL_EXT_shader_realtime_clock) uint2 clockRealtime2x32EXT() {
  __target_switch {
  case spirv:
    return spirv_asm {
            OpCapability ShaderClockKHR;
            OpExtension "SPV_KHR_shader_clock";
            result : $$uint2 = OpReadClockKHR Device; 
        };

  case glsl:
    __intrinsic_asm "clockRealtime2x32EXT()";

  default:
    return 0;
  }
}

[[vk::binding(2, 0)]]
RWStructuredBuffer<uint4> elapsed_time;
#endif

struct PushConstants {
  int2 u_TextureDimensions;
};

[[vk::push_constant]]
PushConstants pushConstants;

static const uint alpha_modifier_table[4] = { 2, 5, 8, 14 };

static const uint etc1_mod_table[16] = {
  2, 8, 5, 17, 9, 29, 13, 42, 18, 60, 24, 80, 33, 106, 47, 183,
};

uint select_table_index(float c_target) {
  if (c_target < 12.5)
    return 0;
  if (c_target < 23.0)
    return 1;
  if (c_target < 35.5)
    return 2;
  if (c_target < 51.0)
    return 3;
  if (c_target < 70.0)
    return 4;
  if (c_target < 93.0)
    return 5;
  if (c_target < 144.5)
    return 6;
  return 7;
}

inline void get_table_modifiers(int idx, out int small, out int big) {
  small = etc1_mod_table[idx * 2];
  big = etc1_mod_table[idx * 2 + 1];
}

// 0   1  |  2   3
// 4   5  |  6   7
// -------+-------
// 8   9  | 10  11
// 12  13 | 14  15

// #define USE_FLAG_MODE_ARRAY

#ifdef USE_FLAG_MODE_ARRAY
struct FlagMode {
  uint idx[8];
  static const FlagMode VERTICAL_LEFT = FlagMode( { 0, 1, 4, 5, 8, 9, 12, 13 });
  static const FlagMode VERTICAL_RIGHT =
      FlagMode( { 2, 3, 6, 7, 10, 11, 14, 15 });
  static const FlagMode HORIZONTAL_TOP = FlagMode( { 0, 1, 2, 3, 4, 5, 6, 7 });
  static const FlagMode HORIZONTAL_BOTTOM =
      FlagMode( { 8, 9, 10, 11, 12, 13, 14, 15 });
  __subscript(uint n)->uint {
    get { return idx[n]; }
  }
};

#else
// For some reason, using a mode-indexed FlagMode causes slangc to
// compile this shader incorrectly...
struct FlagMode {
  uint mode;
  static const FlagMode HORIZONTAL_TOP = FlagMode(0);
  static const FlagMode HORIZONTAL_BOTTOM = FlagMode(1);
  static const FlagMode VERTICAL_LEFT = FlagMode(2);
  static const FlagMode VERTICAL_RIGHT = FlagMode(3);
  __subscript(uint n)->uint {
    get {
      if (mode == 0) {
        return n;
      } else if (mode == 1) {
        return n + 8;
      } else if (mode == 2) {
        return (((n >> 1) << 2) + (n & 1));
      } else {
        return (((n >> 1) << 2) + (n & 1)) + 2;
      }
    }
  }
};
#endif

struct Pixels {
  uint data[16];

  __subscript(uint n)->uint3 {
    get {
      return uint3(data[n] & 0xFF, (data[n] >> 8) & 0xFF,
                   (data[n] >> 16) & 0xFF);
    }
    set {
      data[n] =
          uint(newValue.x) | (uint(newValue.y) << 8) | (uint(newValue.z) << 16);
    }
  }

  Luminances Y() {
    Luminances Y;
    for (int i = 0; i < 16; ++i) {
      Y[i] = luminance(this[i]);
    }
    return Y;
  }
};

struct ArrayUint32x4 {
  uint data[4];

  __subscript(uint n)->uint {
    get {
      uint arrayIndex = n >> 2;
      uint byteOffset = (n & 3) << 3;
      return (data[arrayIndex] >> byteOffset) & 0xFF;
    }
    set {
      uint arrayIndex = n >> 2;
      uint byteOffset = (n & 3) << 3;
      uint mask = ~(0xFF << byteOffset);
      data[arrayIndex] =
          (data[arrayIndex] & mask) | ((newValue & 0xFF) << byteOffset);
    }
  }
};

typedef ArrayUint32x4 Luminances;
typedef ArrayUint32x4 Selectors;
typedef ArrayUint32x4 Alphas;

uint luminance(uint3 pixel) {
  return uint(0.299 * float(pixel.x) + 0.587 * float(pixel.y) +
              0.114 * float(pixel.z));
}

inline uint2 pack_etc1_individual_payload(uint3 c0, uint3 c1, uint t0, uint t1,
                                          bool horizontal,
                                          const Selectors selectors) {
  uint py = 0u;
  py |= (uint(c0.x) << 28) | (uint(c0.y) << 20) | (uint(c0.z) << 12);
  py |= (uint(c1.x) << 24) | (uint(c1.y) << 16) | (uint(c1.z) << 8);
  py |= (uint(t0) << 5) | (uint(t1) << 2);
  py |= (0u << 1);
  py |= uint(horizontal);

  uint px = 0u;

  for (var i = 0; i < 16; ++i) {
    int x = i % 4;
    int y = i / 4;
    int lp = x * 4 + y;

    var sel = selectors[i];
    uint lsb = uint(sel & 1);
    uint msb = uint((sel >> 1) & 1);

    px |= (lsb << uint(lp));
    px |= (msb << uint(16 + lp));
  }

  return uint2(px, py);
}

void get_avg_color(const Pixels pixels, const FlagMode sb, out uint3 color) {
  uint3 avg = 0u;
  for (var i = 0; i < 8; ++i) {
    int idx = sb[i];
    avg += pixels[idx];
  }
  avg /= 8;
  color.x = clamp(uint(round(float(avg.x) / 17)), 0, 15);
  color.y = clamp(uint(round(float(avg.y) / 17)), 0, 15);
  color.z = clamp(uint(round(float(avg.z) / 17)), 0, 15);
}

int opt_selector(int table_idx, int Y_idx, int Y_avg, out uint s) {
  int d, c;
  get_table_modifiers(table_idx, d, c);
  var T = c / 2 + d / 2;

  var e = Y_idx - Y_avg;
  uint sign_bit = (e < 0) ? 1 : 0;
  uint mag_bit = (abs(e) > T) ? 1 : 0;
  s = (sign_bit << 1) | mag_bit;

  return (mag_bit == 1 ? c : d) * (sign_bit == 1 ? -1 : 1);
}

inline uint get_table_index(const Luminances Y, const FlagMode sb) {
  var high_Y_sum = 0.0;
  var low_Y_sum = 0.0;
  var high_count = 0;
  var low_count = 0;

  var avg_Y = 0;
  for (var i = 0; i < 8; ++i) {
    int idx = sb[i];
    avg_Y += Y[idx];
  }
  avg_Y /= 8;

  for (var i = 0; i < 8; ++i) {
    var idx = sb[i];
    if (Y[idx] >= avg_Y) {
      high_Y_sum += float(Y[idx]);
      high_count++;
    } else {
      low_Y_sum += float(Y[idx]);
      low_count++;
    }
  }

  float high_avg =
      (high_count > 0) ? (high_Y_sum / float(high_count)) : float(avg_Y);
  float low_avg =
      (low_count > 0) ? (low_Y_sum / float(low_count)) : float(avg_Y);

  var spread = high_avg - low_avg;
  var c_target = 0.5 * spread;
  return select_table_index(c_target);
}

void process_subblock<let nb : int = 1>(const Pixels pixels, const Luminances Y,
                                        const FlagMode sb,
                                        inout uint3 packed_color,
                                        out uint best_table,
                                        out Selectors best_selectors) {
  get_avg_color(pixels, sb, packed_color);
  var Y_base = luminance(packed_color * 17);
  int table_idx = get_table_index(Y, sb);
  var initial_table_idx = clamp(table_idx, nb, 7 - nb);

  var base8 = packed_color * 17;

  int min_block_error = 2147483647;

  for (var t = initial_table_idx - nb; t <= initial_table_idx + nb; t++) {
    int current_table_error = 0;
    Selectors current_selectors;

    for (var p = 0; p < 8; p++) {
      var idx = sb[p];
      uint s;
      var modifier = opt_selector(t, Y[idx], Y_base, s);

      if (nb > 0) {
        var pixel = pixels[idx];
        var mod_color = clamp(base8 + modifier, 0, 255);
        var diff = mod_color - int3(pixel);
        var err = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        current_table_error += err;
      }
      current_selectors[idx] = s;
    }

    if (nb == 0 || current_table_error < min_block_error) {
      min_block_error = current_table_error;
      best_table = t;
      best_selectors = current_selectors;
    }
  }
}

uint2 encode_etc1(Pixels pixels) {
  Luminances Y = pixels.Y();

  var Y_top = Y[0] + Y[1] + Y[2] + Y[3] + Y[4] + Y[5] + Y[6] + Y[7];
  var Y_total =
      Y_top + Y[8] + Y[9] + Y[10] + Y[11] + Y[12] + Y[13] + Y[14] + Y[15];
  var Y_left = Y[0] + Y[1] + Y[4] + Y[5] + Y[8] + Y[9] + Y[12] + Y[13];
  var H_contrast = abs(Y_top - (Y_total - Y_top));
  var V_contrast = abs(Y_left - (Y_total - Y_left));

#ifdef USE_FLAG_MODE_ARRAY
  const bool horizontal = (H_contrast > V_contrast);
#else
  const bool horizontal = true;
#endif

  const FlagMode sb0 =
      horizontal ? FlagMode.HORIZONTAL_TOP : FlagMode.VERTICAL_LEFT;
  const FlagMode sb1 =
      horizontal ? FlagMode.HORIZONTAL_BOTTOM : FlagMode.VERTICAL_RIGHT;

  uint3 colors[2];
  uint tables[2];
  Selectors best_selectors;

  const int nb = 2; // consider +/- 1 additional neighboring table (3 total)
  // Don't unroll this loop, the inlining drastically increases reg pressure
  for (int block = 0; block < 2; block++) {
    process_subblock<nb>(pixels, Y, sb0, colors[block], tables[block],
                         best_selectors);
  }

  return pack_etc1_individual_payload(colors[0], colors[1], tables[0],
                                      tables[1], horizontal, best_selectors);
}

void encode_etc2_alpha(Alphas alphas, inout uint alpha_p0,
                       inout uint alpha_p1) {
  var min_a = 255;
  var max_a = 0;
  for (var i = 0; i < 16; ++i) {
    var a = alphas[i];
    min_a = min(min_a, a);
    max_a = max(max_a, a);
  }

  var base = min_a;
  var range = max_a - min_a;
  var multiplier = clamp(range / 22, 0u, 15u);

  alpha_p0 = (base & 0xFF) | ((multiplier & 0xF) << 8) | (0u << 12);
  alpha_p1 = 0;

  for (int i = 0; i < 16; ++i) {
    var a = uint(alphas[i]);
    var diff = int(a) - int(base);
    var best_idx = 0;
    var min_err = 1000;
    for (var m = 0; m < 4; ++m) {
      var val = alpha_modifier_table[m] * int(multiplier);
      var err = abs<int>(diff - val);
      if (err < min_err) {
        min_err = err;
        best_idx = m;
      }
    }
    if (i < 10)
      alpha_p0 |= (uint(best_idx) << (16 + i * 3));
    else
      alpha_p1 |= (uint(best_idx) << ((i - 10) * 3));
  }
}

uint flip_endian_encode(uint v) {
  uint4 words = uint4(v) >> uint4(0, 8, 16, 24);
  words &= 0xffu;
  return (words.x << 24u) | (words.y << 16u) | (words.z << 8u) |
         (words.w << 0u);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void main(uint3 gl_GlobalInvocationID: SV_DispatchThreadID) {
  int2 block_pos = int2(gl_GlobalInvocationID.xy);
  int2 tex_coord = block_pos * 4;

#ifdef ENABLE_DIAGNOSTICS
  uint2 startTime = clockRealtime2x32EXT();
#endif

  if (any(tex_coord >= pushConstants.u_TextureDimensions))
    return;

  Pixels pixels;
  Alphas alphas;
  uint width = pushConstants.u_TextureDimensions.x;
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      var tex_element =
          input_rgba8_buffer[(tex_coord.y + y) * width + (tex_coord.x + x)];
      uint a = (tex_element >> 24) & 0xFF;
      uint b = (tex_element >> 16) & 0xFF;
      uint g = (tex_element >> 8) & 0xFF;
      uint r = (tex_element >> 0) & 0xFF;
      pixels[y * 4 + x] = uint3(r, g, b);
      alphas[y * 4 + x] = a;
    }
  }

  uint alpha_p0 = 0;
  uint alpha_p1 = 0;
  encode_etc2_alpha(alphas, alpha_p0, alpha_p1);

  uint2 packed = encode_etc1(pixels);

  uint color_p0 = flip_endian_encode(packed.y);
  uint color_p1 = flip_endian_encode(packed.x);

  uint buffer_index =
      block_pos.y * (pushConstants.u_TextureDimensions.x / 4) + block_pos.x;
  etc1_blocks[buffer_index] = uint4(alpha_p0, alpha_p1, color_p0, color_p1);

#ifdef ENABLE_DIAGNOSTICS
  elapsed_time[buffer_index] =
      uint4(startTime.x, startTime.y, startTime.x, startTime.y);
#endif
}
