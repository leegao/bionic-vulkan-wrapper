// MIT License
// Copyright (c) 2026 Lee Gao

// Design Doc:
// https://docs.google.com/document/d/1lyu7vRDGmXe3G0DedLf9aiBVK9pElZ9lzdGqAUd0B8o/edit?usp=sharing

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

struct Diagnostics {
  uint2 start_time;
  float mse;
  uint unused;
#ifndef DISABLE_RECONSTRUCTION
  Reconstruction reconstructed;
#else
  uint reconstructed[16];
#endif
};

[[vk::binding(2, 0)]]
RWStructuredBuffer<Diagnostics> diagnostics;
#endif

struct PushConstants {
  int2 u_TextureDimensions;
  uint u_Flags;
};

[[vk::push_constant]]
PushConstants pushConstants;

struct PackedArray<let M : uint, let N : uint> {
  static const uint kElementsPerStorage = 32 / M;
  static const uint kStorageCount =
      (N + kElementsPerStorage - 1u) / kElementsPerStorage;
  static const uint kMask = (1u << M) - 1u;

  uint data[kStorageCount];

  __init(uint initialValues[N]) {
    this.data = {};
    for (int n = 0; n < N; n++) {
      this[n] = initialValues[n];
    }
  }

  inline __subscript(uint n)->uint {
    get {
      var arrayIndex = n / kElementsPerStorage;
      var bitOffset = (n % kElementsPerStorage) * M;

      return (data[arrayIndex] >> bitOffset) & kMask;
    }
    set {
      var arrayIndex = n / kElementsPerStorage;
      var bitOffset = (n % kElementsPerStorage) * M;
      var clearMask = ~(kMask << bitOffset);

      data[arrayIndex] =
          (data[arrayIndex] & clearMask) | ((newValue & kMask) << bitOffset);
    }
  }
};

static const var alpha_modifier_table = PackedArray<4, 4>( { 2, 5, 8, 14 });

static const var etc1_mod_table = PackedArray<8, 16>( {
  2,
  8,
  5,
  17,
  9,
  29,
  13,
  42,
  18,
  60,
  24,
  80,
  33,
  106,
  47,
  183,
});

static const var etc2_distance_table =
    PackedArray<8, 8>( { 3, 6, 11, 16, 23, 32, 41, 64 });

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

#ifdef USE_FLAG_MODE_ARRAY

// 0   1  |  2   3
// 4   5  |  6   7
// -------+-------
// 8   9  | 10  11
// 12  13 | 14  15
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

// 0   1  |  2   3
// 4   5  |  6   7
// -------+-------
// 8   9  | 10  11
// 12  13 | 14  15
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

uint luminance(Pixel pixel) {
  // Prioritize color correctness - use gray-luminance instead of YUV
  return (pixel.x + pixel.y + pixel.z) / 3;
}

struct Pixels {
  uint data[16];

  __subscript(uint n)->Pixel {
    get {
      return Pixel(data[n] & 0xFF, (data[n] >> 8) & 0xFF,
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

typedef PackedArray<8, 16> Luminances;
typedef PackedArray<2, 16> Selectors;
typedef PackedArray<8, 16> Alphas;
typedef uint3 Pixel;

#ifdef ENABLE_DIAGNOSTICS
#ifndef DISABLE_RECONSTRUCTION
typedef uint8_t4[16] Reconstruction;
#else
// Dummy type for diagnostics to minimize footprint
typedef bool Reconstruction;
#endif
#else
// Dummy type for diagnostics to minimize footprint
typedef bool Reconstruction;
#endif

uint pack_selectors(const Selectors selectors) {
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
  return px;
}

uint pack_etc1_RGB_payload(Pixel c0, Pixel c1, uint t0, uint t1,
                           bool horizontal) {
  var c0_5i = int3(c0.xyz) >> 3;
  var c1_5i = int3(c1.xyz) >> 3;
  var c0_5 = uint3(c0_5i) & 0x1Fu;
  var c0_4 = uint3(c0.xyz >> 4) & 0x0Fu;
  var c1_4 = uint3(c1.xyz >> 4) & 0x0Fu;
  var diff_i = c1_5i - c0_5i;
  var diff_3 = uint3(diff_i) & 0x07u;

  var valid_delta = (diff_i >= -4) && (diff_i <= 3);
  var valid_target = (c0_5i + diff_i >= 0) && (c0_5i + diff_i <= 31);
  var diffmode = all(valid_delta) && all(valid_target);

  uint py = 0u;
  py |= uint(horizontal) & 1u;      // horizontal flag (1 bit)
  py |= (uint(diffmode) & 1u) << 1; // diffmode flag (1 bit)
  py |= (t0 << 5) | (t1 << 2);      // table indices (2x3 bits)
  if (diffmode) {
    // 63     59 58   56 55    51 50   48 47    43 42   40 39  37 36  34    32
    // +--------+-------+--------+-------+--------+-------+------+------+-+-+
    // |   R1   |  dR2  |   G1   |  dG2  |   B1   |  dB2  |  t1  |  t2  |D|F|
    // +--------+-------+--------+-------+--------+-------+------+------+-+-+
    //    5 bit   3-bit    5 bit   3-bit    5 bit   3-bit  3 bit  3 bit |   |
    //                                                                  |   |
    //    diffbit (Bit 33) = 1 -----------------------------------------+   |
    //    flipbit (Bit 32) [0 = Vertical (2x4), 1 = Horizontal (4x2)] ------+
    py |= (diff_3.x << 24) | (diff_3.y << 16) | (diff_3.z << 8);
    py |= (c0_5.x << 27) | (c0_5.y << 19) | (c0_5.z << 11);
  } else {
    // 63     60 59    56 55    52 51    48 47    44 43    40 39  37 36  34   32
    // +--------+--------+--------+--------+--------+--------+------+------+-+-+
    // |   R1   |   R2   |   G1   |   G2   |   B1   |   B2   |  t1  |  t1  |D|F|
    // +--------+--------+--------+--------+--------+--------+------+------+-+-+
    //    4 bit    4 bit    4 bit    4 bit    4 bit    4 bit  3 bit  3 bit |   |
    //                                                                     |   |
    //     diffbit (Bit 33) = 0 -------------------------------------------+   |
    //     flipbit (Bit 32) [0 = Vertical (2x4), 1 = Horizontal (4x2)] --------+
    py |= (c1_4.x << 24) | (c1_4.y << 16) | (c1_4.z << 8);
    py |= (c0_4.x << 28) | (c0_4.y << 20) | (c0_4.z << 12);
  }

  return py;
}

uint pack_etc2_T_RGB_payload(Pixel c0, Pixel c1, uint distance_idx) {
  var c0_4 = uint3(c0 >> 4) & 0xFu;
  var c1_4 = uint3(c1 >> 4) & 0xFu;
  var r0 = c0_4.x & 0xFu;
  var r0_low = r0 & 3u;
  var r0_high = (r0 >> 2u) & 3u;

  // Force the decoder into an over/underflow state.
  // 1. Set bit 26 (the sign bit for 'rd') and
  // 2. Set bits 29-31 (the top of 'r') depending on r0's values to guarantee
  // that (r + rd) overflows.
  uint legacy_rd_26;
  uint legacy_r0_29_31;
  if (r0_high + r0_low < 4u) {
    // underflow
    legacy_rd_26 = 1u;
    legacy_r0_29_31 = 0u;
  } else {
    // overflow
    legacy_rd_26 = 0u;
    legacy_r0_29_31 = 7u;
  }

  uint py = 0u;
  py |= (distance_idx & 1u) << 0;                       // dist low (1 bit)
  py |= 1u << 1;                                        // diffbit=1 (1 bit)
  py |= ((distance_idx >> 1) & 3u) << 2;                // dist high (2 bits)
  py |= (c1_4.z << 4) | (c1_4.y << 8) | (c1_4.x << 12); // bgr1 (3x4 bits)
  py |= (c0_4.z << 16) | (c0_4.y << 20);                // bgr0 (2x4 bits)
  py |= r0_low << 24;                                   // r0 low (2 bits)
  py |= legacy_rd_26 << 26;                             // force over/underflow
  py |= r0_high << 27;                                  // r0 high (2 bits)
  py |= legacy_r0_29_31 << 29;                          // force over/underflow

  return py;
}

Pixel quantize_pixel<let bits : uint>(Pixel color) {
  static const uint mask = (1u << bits) - 1u;
  static const uint shift = 8u - bits;
  var packed = (color >> shift) & mask;
  return packed << shift;
}

uint sse(Pixel target, Pixel base, int modifier) {
  var reconstructed = clamp(int3(base) + modifier, 0, 255);
  var diff = int3(target) - reconstructed;
  return uint(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
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

uint get_table_index(const Luminances Y, const FlagMode sb) {
  var high_Y_sum = 0.0;
  var low_Y_sum = 0.0;
  var high_count = 0;
  var low_count = 0;

  var avg_Y = 0;
  for (var i = 0; i < 8; ++i) {
    var idx = sb[i];
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

int3 clamp3(int3 v, int min, int max) {
  return int3(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max));
}

int process_subblock_etc1<let nb : int>(const Pixels pixels, const Luminances Y,
                                        const FlagMode sb,
                                        inout Pixel best_color,
                                        out uint best_table,
                                        out Selectors best_selectors,
                                        inout Reconstruction reconstructed) {
  best_color = 0u;
  for (var i = 0; i < 8; ++i) {
    best_color += pixels[sb[i]];
  }
  best_color /= 8;
  best_color = quantize_pixel<5>(best_color);

  var Y_base = luminance(best_color);
  int table_idx = get_table_index(Y, sb);
  var initial_table_idx = clamp(table_idx, nb, 7 - nb);

  int min_block_error = 2147483647;

  for (var t = initial_table_idx - nb; t <= initial_table_idx + nb; t++) {
    int current_table_error = 0;
    Selectors current_selectors;

    for (var p = 0; p < 8; p++) {
      var idx = sb[p];
      var modifier = opt_selector(t, Y[idx], Y_base, current_selectors[idx]);
      current_table_error += sse(pixels[idx], best_color, modifier);
    }

    if (nb == 0 || current_table_error < min_block_error) {
      min_block_error = current_table_error;
      best_table = t;
      best_selectors = current_selectors;
    }
  }

#ifdef ENABLE_DIAGNOSTICS
#ifndef DISABLE_RECONSTRUCTION
  for (int i = 0; i < 8; i++) {
    var idx = sb[i];
    var modifier =
        opt_selector(best_table, Y[idx], Y_base, best_selectors[idx]);
    reconstructed[idx] =
        uint8_t4(uint8_t3(clamp3(int3(best_color) + modifier, 0, 255)), 255);
  }
#endif
#endif

  return min_block_error;
}

void find_best_T_colors(const Pixels pixels, const Luminances Y, out Pixel c1,
                        out Pixel c2) {
  var sum_rgb = int3(0);
  for (int i = 0; i < 16; ++i) {
    sum_rgb += int3(pixels[i]);
  }
  var color_mean = sum_rgb / 16;

  // Pick the furthest outlier from the mean that is orthogonal to
  // the mean + grayshift (1,1,1) line
  int max_dist = -1;
  int outlier_idx = 0;
  for (int i = 0; i < 16; ++i) {
    var diff = int3(pixels[i]) - color_mean;
    int dist_sq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
    int sum = diff.x + diff.y + diff.z;
    // sum*sum/3 is the proj of the diff onto the (1,1,1) line
    int ortho_dist_sq = dist_sq - (sum * sum) / 3;

    if (ortho_dist_sq > max_dist) {
      max_dist = ortho_dist_sq;
      outlier_idx = i;
    }
  }

  var seed1 = color_mean;
  var seed2 = int3(pixels[outlier_idx]);

  // 2-means clustering update
  if ((pushConstants.u_Flags & 2u) == 0) {
    int3 c1_sum = 0, c2_sum = 0;
    int c1_count = 0, c2_count = 0;

    for (int i = 0; i < 16; ++i) {
      var p = int3(pixels[i]);

      int3 d1 = p - seed1, d2 = p - seed2;
      int dist1 = d1.x * d1.x + d1.y * d1.y + d1.z * d1.z;
      int dist2 = d2.x * d2.x + d2.y * d2.y + d2.z * d2.z;

      if (dist1 <= dist2) {
        c1_sum += p;
        c1_count++;
      } else {
        c2_sum += p;
        c2_count++;
      }
    }

    seed1 = (c2_count > 0) ? (c2_sum / c2_count) : seed1;
    seed2 = (c1_count > 0) ? (c1_sum / c1_count) : seed2;
  }

  c1 = quantize_pixel<4>(seed1);
  c2 = quantize_pixel<4>(seed2);
}

int process_block_etc2(const Pixels pixels, const Luminances Y,
                       inout Pixel best_colors[2], out uint best_distance,
                       out Selectors best_selectors,
                       inout Reconstruction reconstructed) {
  Pixel c1, c2;
  find_best_T_colors(pixels, Y, c1, c2);
  best_colors[0] = c1;
  best_colors[1] = c2;
  int min_block_error = 2147483647;

  for (uint t = 0u; t < 8u; ++t) {
    int d = etc2_distance_table[t];
    int current_block_error = 0;
    Selectors current_selectors;

    for (int i = 0; i < 16; ++i) {
      int err0 = sse(pixels[i], c1, 0), err1 = sse(pixels[i], c2, d),
          err2 = sse(pixels[i], c2, 0), err3 = sse(pixels[i], c2, -d);

      int sel1 = (err1 < err0) ? 1 : 0;
      int e1 = (err1 < err0) ? err1 : err0;
      int sel2 = (err2 < e1) ? 2 : sel1;
      int e2 = (err2 < e1) ? err2 : e1;
      int sel3 = (err3 < e2) ? 3 : sel2;
      int e3 = (err3 < e2) ? err3 : e2;

      current_selectors[i] = sel3;
      current_block_error += e3;
    }

    if (current_block_error < min_block_error) {
      min_block_error = current_block_error;
      best_distance = t;
      best_selectors = current_selectors;
    }
  }

#ifdef ENABLE_DIAGNOSTICS
#ifndef DISABLE_RECONSTRUCTION
  for (int i = 0; i < 16; i++) {
    int d = etc2_distance_table[best_distance];
    int s = best_selectors[i];
    if (s == 0) {
      reconstructed[i] = uint8_t4(uint8_t3(clamp3(int3(c1), 0, 255)), 255);
    } else if (s == 1) {
      reconstructed[i] = uint8_t4(uint8_t3(clamp3(int3(c2 + d), 0, 255)), 255);
    } else if (s == 2) {
      reconstructed[i] = uint8_t4(uint8_t3(clamp3(int3(c2), 0, 255)), 255);
    } else {
      reconstructed[i] = uint8_t4(uint8_t3(clamp3(int3(c2 - d), 0, 255)), 255);
    }
  }
#endif
#endif

  return min_block_error;
}

uint2 encode_etc2_block(Pixels pixels) {
  Luminances Y = pixels.Y();

  int Y_top = Y[0] + Y[1] + Y[2] + Y[3] + Y[4] + Y[5] + Y[6] + Y[7];
  int Y_total =
      Y_top + Y[8] + Y[9] + Y[10] + Y[11] + Y[12] + Y[13] + Y[14] + Y[15];
  int Y_left = Y[0] + Y[1] + Y[4] + Y[5] + Y[8] + Y[9] + Y[12] + Y[13];
  var H_contrast = abs(Y_top - (Y_total - Y_top));
  var V_contrast = abs(Y_left - (Y_total - Y_left));

  const bool horizontal = (H_contrast > V_contrast);
  const FlagMode sb0 =
      horizontal ? FlagMode.HORIZONTAL_TOP : FlagMode.VERTICAL_LEFT;
  const FlagMode sb1 =
      horizontal ? FlagMode.HORIZONTAL_BOTTOM : FlagMode.VERTICAL_RIGHT;

  static const int nb = 1;
  // Start with ETC1 encoding
  Pixel etc1_colors[2];
  uint etc1_tables[2];
  Selectors etc1_selectors;
  Reconstruction etc1_reconstructed;
  int etc1_sse = 0;
  for (int block = 0; block < 2; block++) {
    let sb = block == 0 ? sb0 : sb1;
    var sse = process_subblock_etc1<nb>(pixels, Y, sb, etc1_colors[block],
                                        etc1_tables[block], etc1_selectors,
                                        etc1_reconstructed);
    etc1_sse += sse;
  }

  let try_T = (pushConstants.u_Flags & 1u) == 0;
  // Try ETC2 T-mode encoding
  Pixel etc2_colors[2];
  uint etc2_distance;
  Selectors etc2_selectors;
  Reconstruction etc2_reconstructed;
  var etc2_sse = 0;
  if (try_T)
    etc2_sse = process_block_etc2(pixels, Y, etc2_colors, etc2_distance,
                                  etc2_selectors, etc2_reconstructed);

  let is_T_mode = try_T && etc2_sse < etc1_sse;

#ifdef ENABLE_DIAGNOSTICS
  if (is_T_mode) {
    diagnostics[buffer_index].mse = float(etc2_sse) / 65025 / 24;
#ifndef DISABLE_RECONSTRUCTION
    diagnostics[buffer_index].reconstructed = etc2_reconstructed;
#endif
  } else {
    diagnostics[buffer_index].mse = float(etc1_sse) / 65025 / 24;
#ifndef DISABLE_RECONSTRUCTION
    diagnostics[buffer_index].reconstructed = etc1_reconstructed;
#endif
  }
#endif

  if (!is_T_mode) {
    return uint2(pack_selectors(etc1_selectors),
                 pack_etc1_RGB_payload(etc1_colors[0], etc1_colors[1],
                                       etc1_tables[0], etc1_tables[1],
                                       horizontal));
  } else {
    return uint2(
        pack_selectors(etc2_selectors),
        pack_etc2_T_RGB_payload(etc2_colors[0], etc2_colors[1], etc2_distance));
  }
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

static uint buffer_index;

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

  buffer_index =
      block_pos.y * (pushConstants.u_TextureDimensions.x / 4) + block_pos.x;

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
      pixels[y * 4 + x] = Pixel(r, g, b);
      alphas[y * 4 + x] = a;
    }
  }

  uint alpha_p0 = 0;
  uint alpha_p1 = 0;
  encode_etc2_alpha(alphas, alpha_p0, alpha_p1);

  var packed = encode_etc2_block(pixels);
  var color_p0 = flip_endian_encode(packed.y);
  var color_p1 = flip_endian_encode(packed.x);
  etc1_blocks[buffer_index] = uint4(alpha_p0, alpha_p1, color_p0, color_p1);

#ifdef ENABLE_DIAGNOSTICS
  diagnostics[buffer_index].start_time = startTime;
#endif
}
