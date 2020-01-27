#include "BinaryManipulation.h"
#include <iostream>

void test0() {
    std::cout << "Simple test 0 of single decoder" << std::endl;
    auto contents = BinaryManipulation::unpack<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes>(0xFDEDABCD);
    // we get a tuple of tuples back
    auto [l0, l1, h0, h1] = contents;
    auto fn = [](auto value) noexcept {
        std::cout << "0x" << std::hex << static_cast<int>(value) << std::endl;
    };
    fn(l0);
    fn(l1);
    fn(h0);
    fn(h1);

    // put the tuple back in
    auto combination = BinaryManipulation::pack<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes>(std::move(contents));
    fn(combination);
}
void test1() {
    std::cout << "Simple test 1: Single NestedEncoderDecoer" << std::endl;
    using TestEncoderDecoder = BinaryManipulation::EncoderDecoder<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes,
                                                                            BinaryManipulation::Ordinal32AsLittleEndianHalves>;
    auto result = BinaryManipulation::unpack<uint32_t, TestEncoderDecoder>(0xFDEDABCD);
    auto [quarters, halves] = result;
    auto [l0, l1, h0, h1] = quarters;
    auto [lower, upper] = halves;

    auto fn = [](auto value) noexcept {
        std::cout << "0x" << std::hex << static_cast<int>(value) << std::endl;
    };
    fn(l0);
    fn(l1);
    fn(h0);
    fn(h1);
    fn(lower);
    fn(upper);

    auto combination = BinaryManipulation::pack<uint32_t, TestEncoderDecoder>(std::move(result));
    fn(combination);
}
int main() {
    test0();
    test1();
    return 0;
}
