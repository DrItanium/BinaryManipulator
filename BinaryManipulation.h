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
class BitPattern final {
public:
    using DataType = T;
    using SliceType = R;
    using InteractPair = ShiftMaskData<T>;
public:
    static constexpr auto Mask = mask;
    static constexpr auto Shift = shift;
public:
    constexpr BitPattern() = default;
    ~BitPattern() = default;
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
using NoCastPattern = BitPattern<T, T, mask, shift>;

template<typename T, T mask, T shift = static_cast<T>(0)>
using BoolPattern = BitPattern<T, bool, mask, shift>;

template<typename T, T position>
using FlagPattern = BoolPattern<T, static_cast<T>(1) << position, position>;

template<typename T, typename ... Patterns>
class EncoderDecoder final {
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
    return EncoderDecoder<T, Patterns...>::encode(std::forward<typename Patterns::SliceType>(inputs)...);
}

template<typename T, typename ... Patterns>
constexpr decltype(auto) unpack(T input) noexcept {
    return EncoderDecoder<T, Patterns...>::decode(input);
}

using Byte3OfOrdinal32 = BitPattern<uint32_t, uint8_t, 0xFF00'0000, 24>;
using Byte2OfOrdinal32 = BitPattern<uint32_t, uint8_t, 0x00FF'0000, 16>;
using Byte1OfOrdinal32 = BitPattern<uint32_t, uint8_t, 0x0000'FF00, 8>;
using Byte0OfOrdinal32 = BitPattern<uint32_t, uint8_t, 0x0000'00FF>;
using UpperHalfOfOrdinal32 = BitPattern<uint32_t, uint16_t, 0xFFFF'0000, 16>;
using LowerHalfOfOrdinal32 = BitPattern<uint32_t, uint16_t, 0x0000'FFFF>;

using Ordinal32AsLittleEndianBytes = EncoderDecoder<uint32_t, 
      Byte0OfOrdinal32,
      Byte1OfOrdinal32,
      Byte2OfOrdinal32,
      Byte3OfOrdinal32>;
using Ordinal32AsLittleEndianHalves = EncoderDecoder<uint32_t,
      LowerHalfOfOrdinal32,
      UpperHalfOfOrdinal32>;


} // end namespace BinaryManipulation
#endif // BinaryManipulation_h__
