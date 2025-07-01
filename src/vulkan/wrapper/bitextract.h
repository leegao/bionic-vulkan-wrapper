#ifndef BITEXTRACT_H_
#define BITEXTRACT_H_

int extract_bits(uvec4 payload, int offset, int bits)
{
	int last_offset = offset + bits - 1;
	int result;

	if (bits <= 0)
		result = 0;
	else if ((last_offset >> 5) == (offset >> 5))
		result = int(bitfieldExtract(payload[offset >> 5], offset & 31, bits));
	else
	{
		int first_bits = 32 - (offset & 31);
		int result_first = int(bitfieldExtract(payload[offset >> 5], offset & 31, first_bits));
		int result_second = int(bitfieldExtract(payload[(offset >> 5) + 1], 0, bits - first_bits));
		result = result_first | (result_second << first_bits);
	}
	return result;
}

int extract_bits_sign(uvec4 payload, int offset, int bits)
{
	int last_offset = offset + bits - 1;
	int result;

	if (bits <= 0)
		result = 0;
	else if ((last_offset >> 5) == (offset >> 5))
		result = bitfieldExtract(int(payload[offset >> 5]), offset & 31, bits);
	else
	{
		int first_bits = 32 - (offset & 31);
		int result_first = int(bitfieldExtract(payload[offset >> 5], offset & 31, first_bits));
		int result_second = bitfieldExtract(int(payload[(offset >> 5) + 1]), 0, bits - first_bits);
		result = result_first | (result_second << first_bits);
	}
	return result;
}

int extract_bits_reverse(uvec4 payload, int offset, int bits)
{
	int last_offset = offset + bits - 1;
	int result;

	if (bits <= 0)
		result = 0;
	else if ((last_offset >> 5) == (offset >> 5))
		result = int(bitfieldReverse(bitfieldExtract(payload[offset >> 5], offset & 31, bits)) >> (32 - bits));
	else
	{
		int first_bits = 32 - (offset & 31);
		uint result_first = bitfieldExtract(payload[offset >> 5], offset & 31, first_bits);
		uint result_second = bitfieldExtract(payload[(offset >> 5) + 1], 0, bits - first_bits);
		result = int(bitfieldReverse(result_first | (result_second << first_bits)) >> (32 - bits));
	}
	return result;
}

uint vec4_to_rgba8(vec4 color) {
    // 1. Clamp the color to ensure it's within the [0.0, 1.0] range.
    color = clamp(color, 0.0, 1.0);

    // 2. Scale the floating-point components [0.0, 1.0] to the integer range [0, 255].
    // We multiply by 255.0 and then convert to unsigned integer type.
    uint r = uint(color.r * 255.0);
    uint g = uint(color.g * 255.0);
    uint b = uint(color.b * 255.0);
    uint a = uint(color.a * 255.0);

    // 3. Bit-shift each component to its correct position and combine using bitwise OR.
    // The layout is AABBGGRR
    return (a << 24) | (b << 16) | (g << 8) | r;
}

#endif
