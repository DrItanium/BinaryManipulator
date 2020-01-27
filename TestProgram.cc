#include "BinaryManipulation.h"
#include <iostream>

int main() {
    auto result = BinaryManipulation::unpack<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes>(0xFDEDABCD);
    // we get a tuple of tuples back
    auto [l0, l1, h0, h1] = std::get<0>(result);
    auto fn = [](auto value) noexcept {
        std::cout << "0x" << std::hex << static_cast<int>(value) << std::endl;
    };
    fn(l0);
    fn(l1);
    fn(h0);
    fn(h1);
    return 0;
}