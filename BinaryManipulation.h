/**
 * @file
 * Routines for generating, manipulating, and inspecting binary quantities
 * @copyright
 * BinaryManipulator
 * Copyright (c) 2020, Joshua Scoggins
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BinaryManipulation_h__
#define BinaryManipulation_h__
#include <tuple>
#include <type_traits>
#include <cstdint>
#include <climits>
namespace BinaryManipulation {

template<typename T>
constexpr auto IsBoolType = std::is_same_v<std::decay_t<T>, bool>;
template<typename T, typename R, T mask, T shift = static_cast<T>(0)>
constexpr R decode(T value) noexcept {
    using TK = std::decay_t<T>;
    if constexpr (IsBoolType<R> && std::is_integral_v<TK>) {
        // exploit explicit boolean conversion to get rid of the shift if we
        // can safely deduce that R is a boolean and T is an integral type
        return value & mask;
    } else {
        return static_cast<R>((value & mask) >> shift);
    }
}

/**
 * Tuple used in place of mask and shift
 */
template<typename T>
using ShiftMaskData = std::tuple<T, T>;


template<typename T, typename R, ShiftMaskData<T> description>
constexpr R decode(T value) noexcept {
    return decode<T, R, std::get<0>(description), std::get<1>(description)>(value);
}

template<typename T, typename R, T mask, T shift = static_cast<T>(0)>
constexpr T encode(T value, R input) noexcept {
    using K = std::decay_t<R>;
    if constexpr (auto result = value & (~mask); IsBoolType<R>) {
        // ignore the shift in the case of the bool since the mask will be the same as true
        // in the correct positions
        if (input) {
            result |= mask;
        }
        return result;
    } else {
        return result | ((static_cast<T>(input) << shift) & mask);
    }
}

template<typename T, typename R, ShiftMaskData<T> description>
constexpr T encode(T value, R input) noexcept {
    return encode<T, R, std::get<0>(description), std::get<1>(description)>(value, input);
}


template<typename T, typename R, T mask, T shift = static_cast<T>(0)>
class Pattern final {
public:
    using DataType = T;
    using SliceType = R;
    using InteractPair = ShiftMaskData<T>;
public:
    static constexpr auto Mask = mask;
    static constexpr auto Shift = shift;
public:
    constexpr Pattern() = default;
    ~Pattern() = default;
    constexpr auto getDescription() const noexcept { return _description; }
    constexpr auto getMask() const noexcept { return mask; }
    constexpr auto getShift() const noexcept { return shift; }
    static constexpr auto decode(DataType input) noexcept {
        return BinaryManipulation::decode<DataType, SliceType, _description>(input);
    }
    static constexpr auto encode(DataType value, SliceType input) noexcept {
        return BinaryManipulation::encode<DataType, SliceType, _description>(value, input);
    }
    static constexpr auto encode(SliceType input) noexcept {
        return encode(static_cast<DataType>(0), input);
    }
private:
    static constexpr InteractPair _description { mask, shift };
};

template<typename T, T mask, T shift = static_cast<T>(0)>
using NoCastPattern = Pattern<T, T, mask, shift>;

template<typename T, T mask, T shift = static_cast<T>(0)>
using BoolPattern = Pattern<T, bool, mask, shift>;

template<typename T, T position>
using FlagPattern = BoolPattern<T, static_cast<T>(1) << position, position>;

template<typename T, typename ... Patterns>
class Description final {
    public:
        using DataType = T;
        using SliceType = std::tuple<typename Patterns::SliceType ...>;
        static constexpr auto NumberOfPatterns = std::tuple_size_v<SliceType>;

        static_assert((std::is_same_v<typename Patterns::DataType, DataType> && ...), "All patterns must operate on the provided binary type!");
    public:
        static constexpr decltype(auto) decode(DataType input) noexcept {
            // Do not return a single element tuple as it makes everything more complicated, instead
            // return that element itself. In all other cases we must return the tuple as expected
            if constexpr (auto tup = std::make_tuple<typename Patterns::SliceType...>(Patterns::decode(input)...); NumberOfPatterns == 1) {
                return std::get<0>(tup);
            } else {
                return tup;
            }
        }
        static constexpr DataType encode(typename Patterns::SliceType&& ... values) noexcept {
            return (Patterns::encode(std::move(values)) | ...);
        }
        static constexpr DataType encode(DataType value, typename Patterns::SliceType&& ... inputs) noexcept {
            if constexpr (NumberOfPatterns == 0) {
                return value;
            } else {
                return (Patterns::encode(value, inputs) | ... );
            }
        }
    private:
        template<std::size_t ... I>
        static constexpr DataType encode0(SliceType&& tuple, std::index_sequence<I...>) noexcept {
            return encode(std::move(std::get<I>(tuple))...);
        }
    public:
        static constexpr DataType encode(SliceType&& tuple) noexcept {
            // need to unpack the tuple
            return encode0(std::move(tuple), std::make_index_sequence<std::tuple_size_v<SliceType>> {});
        }
};
template<typename T, typename ... Patterns>
constexpr T pack(typename Patterns::SliceType&& ... inputs) noexcept {
    return Description<T, Patterns...>::encode(std::forward<typename Patterns::SliceType>(inputs)...);
}

template<typename T, typename ... Patterns>
constexpr decltype(auto) unpack(T input) noexcept {
    return Description<T, Patterns...>::decode(input);
}

template<typename T>
constexpr auto BitCount = sizeof(T) * CHAR_BIT;

template<typename T>
class HalfOf final {
    public:
        using FullType = T;
    public:
        HalfOf() = delete;
        ~HalfOf() = delete;
        HalfOf(const HalfOf&) = delete;
        HalfOf(HalfOf&&) = delete;
        HalfOf& operator=(const HalfOf&) = delete;
        HalfOf& operator=(HalfOf&&) = delete;
};

#define DefFact_HalfOf(x, r) \
template<> \
class HalfOf<x> final { \
    public: \
        using FullType = x; \
        using HalfType = r;  \
    public: \
        HalfOf() = delete; \
        ~HalfOf() = delete; \
        HalfOf(const HalfOf&) = delete; \
        HalfOf(HalfOf&&) = delete; \
        HalfOf& operator=(const HalfOf&) = delete; \
        HalfOf& operator=(HalfOf&&) = delete; \
}
DefFact_HalfOf(uint16_t, uint8_t);
DefFact_HalfOf(uint8_t, uint8_t);
DefFact_HalfOf(uint32_t, uint16_t);
DefFact_HalfOf(uint64_t, uint32_t);
DefFact_HalfOf(int16_t, int8_t);
DefFact_HalfOf(int8_t,  int8_t);
DefFact_HalfOf(int32_t, int16_t);
DefFact_HalfOf(int64_t, int32_t);
#undef DefFact_HalfOf

template<typename T>
using HalfType_t = typename HalfOf<T>::HalfType;
template<typename T>
using QuarterType_t = HalfType_t<HalfType_t<T>>;
template<typename T>
constexpr auto HalfShiftAmount = BitCount<T> / 2;
template<typename T>
constexpr auto QuarterShiftAmount = BitCount<T> / 4;
template<typename T>
constexpr T LowerHalfMask = static_cast<T>(std::numeric_limits<HalfType_t<T>>::max());
template<> constexpr uint8_t LowerHalfMask<uint8_t> = 0x0F;
template<> constexpr int8_t LowerHalfMask<int8_t> = 0x0F;
template<typename T>
constexpr T UpperHalfMask = LowerHalfMask<T> << HalfShiftAmount<T>;

static_assert(UpperHalfMask<uint8_t> == 0xF0);

template<typename T>
constexpr T LowestQuarterMask = static_cast<T>(std::numeric_limits<QuarterType_t<T>>::max());
template<> constexpr uint8_t LowestQuarterMask<uint8_t> = 0b11;
template<> constexpr int8_t LowestQuarterMask<int8_t> = 0b11;
template<typename T>
constexpr T LowerQuarterMask = LowestQuarterMask<T> << QuarterShiftAmount<T>;
template<typename T>
constexpr T HigherQuarterMask = LowerQuarterMask<T> << QuarterShiftAmount<T>;
template<typename T>
constexpr T HighestQuarterMask = HigherQuarterMask<T> << QuarterShiftAmount<T>;
static_assert(HalfShiftAmount<uint16_t> == 8);
static_assert(HalfShiftAmount<uint8_t> == 4);
static_assert(HalfShiftAmount<int8_t> == 4);
static_assert(QuarterShiftAmount<int8_t> == 2);

static_assert(HighestQuarterMask<uint32_t> == 0xFF00'0000);
static_assert(HigherQuarterMask<uint32_t>  == 0x00FF'0000);
static_assert(LowerQuarterMask<uint32_t>   == 0x0000'FF00);
static_assert(LowestQuarterMask<uint32_t>  == 0x0000'00FF);
static_assert(HighestQuarterMask<uint8_t>  == 0b1100'0000);
static_assert(HigherQuarterMask<uint8_t>   == 0b0011'0000);
static_assert(LowerQuarterMask<uint8_t>    == 0b0000'1100);
static_assert(LowestQuarterMask<uint8_t>   == 0b0000'0011);
template<typename T>
using UpperHalfPattern = Pattern<T, HalfType_t<T>, UpperHalfMask<T>, HalfShiftAmount<T>>;
template<typename T>
using LowerHalfPattern = Pattern<T, HalfType_t<T>, LowerHalfMask<T>, 0>;

template<typename T>
using HighestQuarterPattern = Pattern<T, QuarterType_t<T>, HighestQuarterMask<T>, QuarterShiftAmount<T> * 3>;
template<typename T>
using HigherQuarterPattern = Pattern<T, QuarterType_t<T>, HigherQuarterMask<T>, QuarterShiftAmount<T> * 2>;
template<typename T>
using LowerQuarterPattern  = Pattern<T, QuarterType_t<T>, LowerQuarterMask<T>, QuarterShiftAmount<T>>;
template<typename T>
using LowestQuarterPattern = Pattern<T, QuarterType_t<T>, LowestQuarterMask<T>>;

using UpperHalfOfOrdinal16 = UpperHalfPattern<uint16_t>;
using LowerHalfOfOrdinal16 = LowerHalfPattern<uint16_t>;
using Byte3OfOrdinal32 = Pattern<uint32_t, uint8_t, 0xFF00'0000, 24>;
using Byte2OfOrdinal32 = Pattern<uint32_t, uint8_t, 0x00FF'0000, 16>;
using Byte1OfOrdinal32 = Pattern<uint32_t, uint8_t, 0x0000'FF00, 8>;
using Byte0OfOrdinal32 = Pattern<uint32_t, uint8_t, 0x0000'00FF>;
using UpperHalfOfOrdinal32 = UpperHalfPattern<uint32_t>;
using LowerHalfOfOrdinal32 = LowerHalfPattern<uint32_t>;
using UpperHalfOfOrdinal64 = UpperHalfPattern<uint64_t>;
using LowerHalfOfOrdinal64 = LowerHalfPattern<uint64_t>;

template<typename T>
using LittleEndianHalves = Description<T, LowerHalfPattern<T>, UpperHalfPattern<T>>;

template<typename T>
using BigEndianHalves = Description<T, UpperHalfPattern<T>, LowerHalfPattern<T>>;

using Ordinal32AsLittleEndianBytes = Description<uint32_t, 
      Byte0OfOrdinal32,
      Byte1OfOrdinal32,
      Byte2OfOrdinal32,
      Byte3OfOrdinal32>;

} // end namespace BinaryManipulation
#endif // BinaryManipulation_h__
