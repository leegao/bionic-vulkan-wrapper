#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "wrapper_log.h"
#include "wrapper_debug.h"
#include "wrapper_private.h"

#include "bcdec.h"

void BCnDecompression(VkFormat format,
      void* mappedSrcBase,
      void* mappedDst,
      const VkBufferImageCopy* regions) {

    uint8_t* startCompressedData = (uint8_t*)mappedSrcBase + (regions ? regions->bufferOffset : 0);

    // Determine the block size and decoding function based on the format
    uint32_t format_id = format - VK_FORMAT_BC1_RGB_UNORM_BLOCK;
    uint32_t block_size = 16;

    switch (format_id) {
    case 0:
    case 1:
    case 2:
    case 3:
        block_size = 8;
        break;
    case 8:
    case 9:
        block_size = 8;
        break;
    default:
        break;
    }

    int texel_size;
    if (format_id == 12 || format_id == 13) { // BC6H_UFLOAT or BC6H_SFLOAT
        texel_size = 8; // R16G16B16A16_SFLOAT = 8 bytes per pixel
    } else {
        texel_size = 4; // R8G8B8A8_UNORM/SNORM = 4 bytes per pixel
    }

    int height = regions->imageExtent.height;
    int width = regions->imageExtent.width;
    int dst_stride = texel_size * width;
    int src_stride = block_size * (regions->bufferRowLength ? regions->bufferRowLength : width);
    if (regions->bufferRowLength != 0 && (width + 3) / 4 * 4 != regions->bufferRowLength) {
        WLOGE("Texture (fmt=%d) is not tightly packed, block_size = %d, width = %d, bufferRowLength = %d", format, block_size, width, regions->bufferRowLength);
    }

    uint16_t temp_rgb_block[4 * 4 * 4]; // 4x4, 3 channels (rgb) for bc6h, 4 for everyone else

    uint32_t unsupportedBitsBc = get_unsupported_bcn_masks();
    uint32_t watermarkedBitsBc = get_watermarked_bcn_masks();
    uint32_t watermark_size = get_watermark_size();

    bool is_masked = (unsupportedBitsBc & (1 << format_id)) != 0;
    bool is_watermarked = (watermarkedBitsBc & (1 << format_id)) != 0;

    // Loop over the image in 4x4 blocks
    void *srcDataBlock = startCompressedData;
    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            void *dstPixelBlock = (uint8_t *) mappedDst + (y * dst_stride) + (x * texel_size);
            if (is_masked || (is_watermarked && x < watermark_size && y < watermark_size)) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    #define WATERMARK(r,g,b) { \
                        uint8_t* dstRow = (uint8_t*)dstPixelBlock + (i * dst_stride); \
                        dstRow[j * 4 + 0] = r; \
                        dstRow[j * 4 + 1] = g; \
                        dstRow[j * 4 + 2] = b; \
                        dstRow[j * 4 + 3] = 0xff; \
                        break; \
                    }

                    switch (format_id) {
                        case 0: // BC1_RGB_UNORM_BLOCK
                        WATERMARK(0x80, 0, 0); // Maroon
                        case 1: // BC1_RGB_SRGB_BLOCK
                        WATERMARK(0x9a, 0x63, 0x24); // Brown
                        case 2: // BC1_RGBA_UNORM_BLOCK
                        WATERMARK(0x80, 0x80, 0); // Olive
                        case 3: // BC1_RGBA_SRGB_BLOCK
                        WATERMARK(0x46, 0x99, 0x90); // Dark Teal
                        case 4: // BC2_UNORM_BLOCK
                        WATERMARK(0x00, 0x00, 0x75); // Navy
                        case 5: // BC2_SRGB_BLOCK
                        WATERMARK(0xff, 0xfa, 0xc8); // Beige
                        case 6: // BC3_UNORM_BLOCK
                        WATERMARK(0xf5, 0x82, 0x31); // Orange
                        case 7: // BC3_SRGB_BLOCK
                        WATERMARK(0xff, 0xe1, 0x19); // Yellow
                        case 8: // BC4_UNORM_BLOCK
                        WATERMARK(0xbf, 0xef, 0x45); // Lime green
                        case 9: // BC4_SNORM_BLOCK
                        WATERMARK(0x3c, 0xb4, 0x4b); // Green
                        case 10: // BC5_UNORM_BLOCK
                        WATERMARK(0x42, 0xd4, 0xf4); // Cyan
                        case 11: // BC5_SNORM_BLOCK
                        WATERMARK(0x91, 0x1e, 0xb4); // Purple
                        case 12: // BC6H_UFLOAT
                        {
                            // Magenta
                            uint16_t* dstRow = (uint16_t*)((uint8_t*)dstPixelBlock + (i * dst_stride));
                            dstRow[j * 4 + 0] = 0x3c00;
                            dstRow[j * 4 + 1] = 0x3c00;
                            dstRow[j * 4 + 2] = 0;
                            dstRow[j * 4 + 3] = 0x3C00;
                            break;
                        }
                        case 13: // BC6H_SFLOAT
                        {
                            // Red
                            uint16_t* dstRow = (uint16_t*)((uint8_t*)dstPixelBlock + (i * dst_stride));
                            dstRow[j * 4 + 0] = 0x3c00;
                            dstRow[j * 4 + 1] = 0;
                            dstRow[j * 4 + 2] = 0;
                            dstRow[j * 4 + 3] = 0x3C00;
                            break;
                        }
                        case 14:
                        WATERMARK(0xaa, 0xff, 0xc3); // Mint
                        case 15:
                        WATERMARK(0xfa, 0xbe, 0xd4); // Pink
                        default:
                        // Unknown/unsupported format, do nothing.
                        WLOGE("ERROR: Unsupported BCn format %d", format);
                        break;
                    }
                }
            }

            srcDataBlock += block_size;
            continue;
            }

            switch (format_id) {
            case 0: // BC1_RGB_UNORM_BLOCK
            case 1: // BC1_RGB_SRGB_BLOCK
            case 2: // BC1_RGBA_UNORM_BLOCK
            case 3: // BC1_RGBA_SRGB_BLOCK
                    bcdec_bc1(srcDataBlock, dstPixelBlock, dst_stride);
                    break;

            case 4: // BC2_UNORM_BLOCK
            case 5: // BC2_SRGB_BLOCK
                    bcdec_bc2(srcDataBlock, dstPixelBlock, dst_stride);
                    break;

            case 6: // BC3_UNORM_BLOCK
            case 7: // BC3_SRGB_BLOCK
                    bcdec_bc3(srcDataBlock, dstPixelBlock, dst_stride);
                    break;

            case 8: // BC4_UNORM_BLOCK
            case 9: // BC4_SNORM_BLOCK
                    bcdec_bc4(srcDataBlock, dstPixelBlock, dst_stride, format_id == 9);
                    break;

            case 10: // BC5_UNORM_BLOCK
            case 11: // BC5_SNORM_BLOCK
                    bcdec_bc5(srcDataBlock, dstPixelBlock, dst_stride, format_id == 11);
                    break;
            case 12: // BC6H_UFLOAT
            case 13: // BC6H_SFLOAT
                    {
                        bcdec_bc6h_half(srcDataBlock, temp_rgb_block, 4 * 3, format_id==13);
                        for (int i = 0; i < 4; ++i) {
                        // Start of the current destination row for the 4x4 block
                        uint16_t* dstRow = (uint16_t*)((uint8_t*)dstPixelBlock + (i * dst_stride));
                        for (int j = 0; j < 4; ++j) {
                            // Calculate index into the temporary RGB half-float array
                            int src_pixel_offset = (i * 4 + j) * 3; // 3 channels per texel
                            dstRow[j * 4 + 0] = temp_rgb_block[src_pixel_offset + 0]; // R
                            dstRow[j * 4 + 1] = temp_rgb_block[src_pixel_offset + 1]; // G
                            dstRow[j * 4 + 2] = temp_rgb_block[src_pixel_offset + 2]; // B
                            dstRow[j * 4 + 3] = 0x3C00; // A
                        }
                        }
                        break;
                    }
            case 14:
            case 15:
                    bcdec_bc7(srcDataBlock, dstPixelBlock, dst_stride);
                    break;
            default:
                    // Unknown/unsupported format, do nothing.
                    WLOGE("ERROR: Unsupported BCn format %d", format);
                    break;
            }
            srcDataBlock += block_size;
        }
    }
}
