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
	else {
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

vec3 srgb_to_linear(vec3 srgb)
{
    // A precise sRGB to linear conversion
    bvec3 cutoff = lessThan(srgb, vec3(0.04045));
    vec3 higher = pow((srgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3 lower = srgb / vec3(12.92);
    return mix(higher, lower, cutoff);
}


// #define extract_bits(payload, offset, bits) extract_bits_flat(payload.x, payload.y, payload.z, payload.w, offset, bits)

// #define extract_bits(payload, offset, bits) extract_bits_(payload, offset, bits)

//#define EXTRACT_BITS(payload, offset, bits) extract_bits_flat(payload.x, payload.y, payload.z, payload.w, offset, bits)

// "Flattened" version of extract_bits to avoid array indexing.
// This is often more compatible with mobile GPU compilers (e.g., qcom).
int extract_bits_flat(uint x, uint y, uint z, uint w, int offset, int bits) {
	if (bits <= 0) {
		return 0;
	}

	// Index of the 32-bit uint where the extraction starts (0=x, 1=y, 2=z, 3=w).
	int start_idx = offset >> 5; // Equivalent to floor(offset / 32)
	// Index of the 32-bit uint where the extraction ends.
	int end_idx = (offset + bits - 1) >> 5;

	// Case 1: The entire bitfield is contained within a single uint.
	if (start_idx == end_idx) {
		// Bit offset within the 32-bit word.
		int local_offset = offset & 31; // Equivalent to offset % 32
		
		if (start_idx == 0) return int(bitfieldExtract(x, local_offset, bits));
		if (start_idx == 1) return int(bitfieldExtract(y, local_offset, bits));
		if (start_idx == 2) return int(bitfieldExtract(z, local_offset, bits));
		if (start_idx == 3) return int(bitfieldExtract(w, local_offset, bits));
		
		// The starting offset is out of the 128-bit range.
		return 0;
	}
	// Case 2: The bitfield straddles two adjacent uints.
	// This also implies end_idx == start_idx + 1.
	else {
		int local_offset = offset & 31;
		int bits_in_first_word = 32 - local_offset;
		int bits_in_second_word = bits - bits_in_first_word;

		uint first_part;
		uint second_part;

		if (start_idx == 0) { // Straddles x and y
			first_part = bitfieldExtract(x, local_offset, bits_in_first_word);
			second_part = bitfieldExtract(y, 0, bits_in_second_word);
		} else if (start_idx == 1) { // Straddles y and z
			first_part = bitfieldExtract(y, local_offset, bits_in_first_word);
			second_part = bitfieldExtract(z, 0, bits_in_second_word);
		} else if (start_idx == 2) { // Straddles z and w
			first_part = bitfieldExtract(z, local_offset, bits_in_first_word);
			second_part = bitfieldExtract(w, 0, bits_in_second_word);
		} else {
			// Any other case is an invalid read (e.g., starts in 'w' and tries
			// to cross over, or starts out of bounds).
			return 0;
		}

		// Combine the two parts. The part from the second word becomes the
		// most significant bits of the result.
		return int(first_part | (second_part << bits_in_first_word));
	}
}


// int extract_bits3(uvec4 payload, uint start_bit, uint num_bits)
// {
// 	// Early exit for the trivial case. If num_bits can be non-constant,
// 	// this is a small, but still potentially divergent, branch.
// 	// However, for BC7, num_bits is usually small and constant per mode.
// 	if (num_bits == 0u)
// 	{
// 		return 0;
// 	}

// 	// Determine which 32-bit word the bit sequence starts in.
// 	// (start_bit / 32)
// 	uint word_index = start_bit >> 5;

// 	// Determine the bit offset within that word.
// 	// (start_bit % 32)
// 	uint bit_in_word = start_bit & 31u;

// 	// Create a "next word" payload that is shifted by one dword.
// 	// This avoids an out-of-bounds access (e.g., payload[4]) in a
// 	// branchless way. If word_index is 3, next_payload[3] will be 0.
// 	uvec4 next_payload = uvec4(payload.yzw, 0u);

// 	// Unconditionally combine the two potentially required words.
// 	// word0 is the uint where the bits start.
// 	// word1 is the uint that follows word0.
//     uint word0, word1;
//     if (word_index < 4) {
//         uint word0 = payload[word_index];
//         uint word1 = next_payload[word_index];
//     } else {
//         return 0;
//     }

//     uint mask = (1u << num_bits) - 1u;
// 	// Combine the two words into a logical 64-bit value using only 32-bit operations.
// 	// 1. Shift word0 right to align the starting bit to the LSB.
// 	// 2. Shift word1 left to "fill in" the high bits that were shifted out of word0.
// 	// This works even if no bits are needed from word1 (the shift amount will be >= 32).
// 	uint combined = (word0 >> bit_in_word) | (word1 << (32u - bit_in_word));
// 	// if (bit_in_word == 0u) {
// 	// 	return int((word0 >> bit_in_word) & mask);
// 	// } else {
// 	// 	uint combined = (word0 >> bit_in_word) | (word1 << (32u - bit_in_word));

// 	// 	// Create a mask for the desired number of bits.
// 	// 	// bitfieldMask() is a robust GLSL built-in that correctly handles num_bits = 32.
// 	// 	// It's equivalent to (num_bits == 32) ? 0xFFFFFFFFu : (1u << num_bits) - 1u;
// 	// 	// uint mask = uint(bitfieldMask(int(num_bits)));


// 	// 	// Apply the mask to get the final result.
// 	// 	return int(combined & mask);
// 	// }
// }

int extract_bits_(uvec4 payload, uint start_bit, uint num_bits)
{
    // --- Step 1: Calculate indices without branching ---
	uint word_index  = start_bit >> 5;
	uint bit_in_word = start_bit & 31u;

    // --- Step 2: Branchlessly select word0 and word1 ---
    // This is the core of the safe access. Instead of using a variable
    // to index `payload`, we use masks to select the correct component.
    // The `equal()` function returns a boolean vector, which when cast to
    // uint and negated, creates a mask of all 1s (0xFFFFFFFF) for the
    // matching element and all 0s for the others. This is a common
    // and highly efficient branchless technique.

    // Create a selector mask, e.g., (0, 0, 0xFFFFFFFF, 0) if word_index is 2.
	uvec4 selector = -uvec4(equal(uvec4(0, 1, 2, 3), uvec4(word_index)));

    // Select word0 by masking each component and ORing them together.
    // Only the component corresponding to word_index will be non-zero.
	uint word0 = (payload.x & selector.x) |
	             (payload.y & selector.y) |
	             (payload.z & selector.z) |
	             (payload.w & selector.w);

    // Select word1 (the next word) in the same way. The payload is
    // logically shifted, with 0 as the new last element.
	uint word1 = (payload.y & selector.x) |
	             (payload.z & selector.y) |
	             (payload.w & selector.z); // Last element is implicitly `| (0u & selector.w)`

    // The original bug: `word1 << (32 - bit_in_word)` is faulty when
    // bit_in_word is 0. We need to make this expression 0 in that case.
    // Create a mask that is 0 if `bit_in_word` is 0, and 0xFFFFFFFF otherwise.
	uint high_bits_mask = -uint(bit_in_word != 0u);

    // The shift `(32u - bit_in_word)` is still problematic when its result is 32.
    // However, by ANDing the result with our mask, the output becomes 0
    // if `bit_in_word` was 0, correctly nullifying the invalid shift.
	uint high_bits = (word1 << (32u - bit_in_word)) & high_bits_mask;
	uint combined  = (word0 >> bit_in_word) | high_bits;
	uint mask = (1u << num_bits) - 1u; // bitfieldMask(int(num_bits));
	return int(combined & mask);
}


#endif
