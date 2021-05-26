#pragma once

#include "decoder.h"

class BitDecoder {
public:
    BitDecoder(Decoder& dec) : dec(dec){};

    uint64_t readBits(size_t count) {
        if (count > 64) {
            throw std::logic_error("Only support reading 64 bits at a time");
        }

        size_t result = 0;
        while (count > 0) {
            if (remainingBits == 0) {
                dec.read_int_to(buffer);
                remainingBits = 8;
            }
            auto bitsToRead = std::min(count, size_t(remainingBits));

            result <<= bitsToRead;
            result |= getBitsFromBuffer(bitsToRead);
            count -= bitsToRead;
            remainingBits -= bitsToRead;
        };

        return result & ((1 << count) - 1);
    }

    bool readBit() {
        return bool(readBits(1) & 1);
    }

protected:
    static uint8_t getMask(size_t bitCount) {
        return (uint8_t(1) << bitCount) - 1;
    }

    uint8_t getBitsFromBuffer(size_t bitCount) {
        auto mask = getMask(bitCount);
        mask <<= (remainingBits - bitCount);
        auto value = buffer & mask;
        value >>= (remainingBits - bitCount);
        return value;
    }
    Decoder& dec;
    uint8_t buffer;
    uint8_t remainingBits = 0;
};