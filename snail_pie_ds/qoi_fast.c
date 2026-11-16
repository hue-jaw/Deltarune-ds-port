#include <stdint.h>
#include <stdlib.h>

#define PACK_RGBA(r, g, b, a) (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(r))
#define UNPACK_R(c) ((c) & 0xFF)
#define UNPACK_G(c) (((c) >> 8) & 0xFF)
#define UNPACK_B(c) (((c) >> 16) & 0xFF)
#define UNPACK_A(c) (((c) >> 24) & 0xFF)

static inline int sign_ext(uint32_t val, int bits) {
    int m = 1 << (bits - 1);
    return (int)((val ^ m) - m);
}

int decode_gm_qoi(const uint8_t *data, size_t data_len, uint32_t *out_pixels) {
    if (data_len < 12 || data[0] != 'f' || data[1] != 'i' || data[2] != 'o' || data[3] != 'q')
        return -1;
    uint16_t w = data[4] | (data[5] << 8);
    uint16_t h = data[6] | (data[7] << 8);
    uint32_t len = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);

    const uint8_t *ptr = data + 12;
    const uint8_t *end = ptr + len;
    uint32_t total = (uint32_t)w * (uint32_t)h;
    uint32_t *raw = out_pixels;
    uint32_t *raw_end = out_pixels + total;

    uint32_t index[64] = {0};
    uint8_t r = 0, g = 0, b = 0, a = 255;
    uint32_t packed = PACK_RGBA(r, g, b, a);

    while (raw < raw_end && ptr < end) {
        uint8_t b1 = *ptr++;
        if ((b1 & 0xC0) == 0x00) {
            packed = index[b1 & 0x3F];
            r = UNPACK_R(packed); g = UNPACK_G(packed); b = UNPACK_B(packed); a = UNPACK_A(packed);
            *raw++ = packed;
        } else if ((b1 & 0xE0) == 0x40) {
            uint32_t run = (b1 & 0x1F) + 1;
            uint32_t rem = raw_end - raw;
            if (run > rem) run = rem;
            for (uint32_t i = 0; i < run; i++) *raw++ = packed;
        } else if ((b1 & 0xE0) == 0x60) {
            uint8_t b2 = *ptr++;
            uint32_t run = (((uint32_t)(b1 & 0x1F) << 8) | b2) + 33;
            uint32_t rem = raw_end - raw;
            if (run > rem) run = rem;
            for (uint32_t i = 0; i < run; i++) *raw++ = packed;
        } else if ((b1 & 0xC0) == 0x80) {
            r += sign_ext((b1 >> 4) & 3, 2);
            g += sign_ext((b1 >> 2) & 3, 2);
            b += sign_ext(b1 & 3, 2);
            packed = PACK_RGBA(r, g, b, a);
            index[(r ^ g ^ b ^ a) & 63] = packed;
            *raw++ = packed;
        } else if ((b1 & 0xE0) == 0xC0) {
            uint8_t b2 = *ptr++;
            uint32_t m = ((uint32_t)b1 << 8) | b2;
            r += sign_ext((m >> 8) & 0x1F, 5);
            g += sign_ext((m >> 4) & 0x0F, 4);
            b += sign_ext(m & 0x0F, 4);
            packed = PACK_RGBA(r, g, b, a);
            index[(r ^ g ^ b ^ a) & 63] = packed;
            *raw++ = packed;
        } else if ((b1 & 0xF0) == 0xE0) {
            uint8_t b2 = *ptr++;
            uint8_t b3 = *ptr++;
            uint32_t m = ((uint32_t)b1 << 16) | ((uint32_t)b2 << 8) | b3;
            r += sign_ext((m >> 15) & 0x1F, 5);
            g += sign_ext((m >> 10) & 0x1F, 5);
            b += sign_ext((m >> 5) & 0x1F, 5);
            a += sign_ext(m & 0x1F, 5);
            packed = PACK_RGBA(r, g, b, a);
            index[(r ^ g ^ b ^ a) & 63] = packed;
            *raw++ = packed;
        } else if ((b1 & 0xF0) == 0xF0) {
            if (b1 & 8) r = *ptr++;
            if (b1 & 4) g = *ptr++;
            if (b1 & 2) b = *ptr++;
            if (b1 & 1) a = *ptr++;
            packed = PACK_RGBA(r, g, b, a);
            index[(r ^ g ^ b ^ a) & 63] = packed;
            *raw++ = packed;
        }
    }
    return 0;
}
