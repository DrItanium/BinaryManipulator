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
    std::cout << "Simple test 1: Single Nested Description" << std::endl;
    using TestDescription = BinaryManipulation::Description<uint32_t, BinaryManipulation::Ordinal32AsLittleEndianBytes,
                                                                      BinaryManipulation::Ordinal32AsLittleEndianHalves>;
    auto result = BinaryManipulation::unpack<uint32_t, TestDescription>(0xFDEDABCD);
    auto [quarters, halves] = result;
    auto [l0, l1, h0, h1] = quarters;
    auto [lower, upper] = halves;

    outputToCout(l0);
    outputToCout(l1);
    outputToCout(h0);
    outputToCout(h1);
    outputToCout(lower);
    outputToCout(upper);

    auto combination = BinaryManipulation::pack<uint32_t, TestDescription>(std::move(result));
    outputToCout(combination);
}
void test2() {
    std::cout << "Simple test 2: i960 Opcode Translation" << std::endl;
    using Ordinal = uint32_t;
    using HalfOrdinal = uint16_t;
    // we must construct a 16-bit opcode from the standard and extended pieces
    using StandardOpcodePattern = BinaryManipulation::Pattern<Ordinal, HalfOrdinal, 0xFF00'0000, 24>;
    using ExtendedOpcodePattern = BinaryManipulation::Pattern<Ordinal, HalfOrdinal, 0b111'1000'0000, 7>;
    using ShiftStandardOpcodeIntoOpcode16 = BinaryManipulation::NoCastPattern<HalfOrdinal, 0x00'FF, 4>;
    using ShiftExtendedOpcodeIntoOpcode16 = BinaryManipulation::NoCastPattern<HalfOrdinal, 0x00'0F>;

    using OpcodeExtraction = BinaryManipulation::Description<Ordinal, StandardOpcodePattern, ExtendedOpcodePattern>;
    using Opcode16Builder  = BinaryManipulation::Description<HalfOrdinal, ShiftStandardOpcodeIntoOpcode16, ShiftExtendedOpcodeIntoOpcode16>;

    auto fn = [](Ordinal bits) noexcept {
        auto standardManual = StandardOpcodePattern::decode(bits);
        auto extendedManual = ExtendedOpcodePattern::decode(bits);
        auto [sAuto, eAuto] = OpcodeExtraction::decode(bits);
        if ((sAuto != standardManual) || (eAuto != extendedManual)) {
            if ((sAuto != standardManual)) {
                std::cout << "sAuto( " << std::hex << sAuto << ") != sManual(" << std::hex << standardManual << ")" << std::endl;
            }
            if ((eAuto != extendedManual)) {
                std::cout << "eAuto( " << std::hex << eAuto << ") != eManual(" << std::hex << extendedManual << ")" << std::endl;
            }
            return false;
        }
        auto opcode16MajorManual = ShiftStandardOpcodeIntoOpcode16::encode(standardManual);
        auto opcode16ExtendedManual = ShiftExtendedOpcodeIntoOpcode16::encode(extendedManual);
        auto o16ManualResult = opcode16MajorManual | opcode16ExtendedManual;
        auto o16AutoResult = Opcode16Builder::encode(std::move(sAuto), std::move(eAuto));
        if (o16ManualResult != o16AutoResult) {
            std::cout << "manual result (0x" << std::hex  << o16ManualResult << ") != auto result(0x" << std::hex << o16AutoResult << ")" << std::endl;
            return false;
        }
        /// @todo more checks here for decomposing back into component pieces and going backwards
        return true;
    };
    for (int i = 0; i < 0x100; ++i) {
        for (int j = 0; j < 16; ++j) {
            Ordinal value = StandardOpcodePattern::encode(i) | ExtendedOpcodePattern::encode(j);
            if (!fn(value)) {
                std::cout << "Bad instruction " << std::hex << value << std::endl;
                std::cout << "Failure! terminating early!" << std::endl;
                return;
            }
        }
    }
    std::cout << "Passed!" << std::endl;
}
int main() {
    test0();
    test1();
    test2();
    return 0;
}
