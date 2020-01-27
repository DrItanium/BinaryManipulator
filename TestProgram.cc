#include "BinaryManipulation.h"
#include <iostream>

template<typename T>
void outputToCout(T value) noexcept {
    using K = std::decay_t<T>;
    std::cout << "0x" << std::hex;
    // we want to cast the char to an int to make sure that garbage is not printed out
    if constexpr (std::is_same_v<K, signed char> || std::is_same_v<K, unsigned char>) {
        std::cout << static_cast<int>(value);
    } else {
        std::cout << value;
    }
    std::cout << std::endl;
}
void test0() {
    std::cout << "Simple test 0 of single decoder" << std::endl;
    auto contents = BinaryManipulation::unpack<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes>(0xFDEDABCD);
    // we get a tuple of tuples back
    auto [l0, l1, h0, h1] = contents;
    outputToCout(l0);
    outputToCout(l1);
    outputToCout(h0);
    outputToCout(h1);

    // put the tuple back in
    auto combination = BinaryManipulation::pack<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes>(std::move(contents));
    outputToCout(combination);
}
void test1() {
    std::cout << "Simple test 1: Single NestedEncoderDecoer" << std::endl;
    using TestEncoderDecoder = BinaryManipulation::EncoderDecoder<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes,
                                                                            BinaryManipulation::Ordinal32AsLittleEndianHalves>;
    auto result = BinaryManipulation::unpack<uint32_t, TestEncoderDecoder>(0xFDEDABCD);
    auto [quarters, halves] = result;
    auto [l0, l1, h0, h1] = quarters;
    auto [lower, upper] = halves;

    outputToCout(l0);
    outputToCout(l1);
    outputToCout(h0);
    outputToCout(h1);
    outputToCout(lower);
    outputToCout(upper);

    auto combination = BinaryManipulation::pack<uint32_t, TestEncoderDecoder>(std::move(result));
    outputToCout(combination);
}
int main() {
    test0();
    test1();
    return 0;
}
