/**
 * Software Rasterizer Playground.
 *
 * Portable archive and serialization support.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>

#include "containers/memory.h"
#include "containers/string.h"
#include "containers/vector.h"
#include "../utils.h"
#include "except.h"

namespace serial
{

/*
 * Assert assumptions use in the code.
 */

static_assert(
  std::endian::native == std::endian::little || std::endian::native == std::endian::big,
  "Only little endian or big endian CPU architectures are supported.");

static_assert(
  sizeof(bool) == 1, "Only 1-byte bools are supported.");

static_assert(
  sizeof(char) == 1, "Only 1-byte chars are supported.");

static_assert(
  sizeof(float) == 4, "Only 4-byte floats are supported.");

static_assert(
  sizeof(double) == 8, "Only 8-byte doubles are supported.");    // NOLINT(readability-magic-numbers)

static_assert(
  sizeof(std::size_t) == 8, "Only 8-byte std::size_t's are supported.");    // NOLINT(readability-magic-numbers)

/** A primitive scalar type. */
template<class T>
concept serializable_scalar =
  std::is_same_v<T, std::remove_cv_t<T>>
  && std::is_same_v<T, std::remove_reference_t<T>>
  && (std::is_arithmetic_v<T> || std::is_same_v<T, std::byte>);

/** An abstract archive for byte-order independent serialization. */
class Archive
{
    /** The target byte order for persistent archives. */
    std::endian target_byte_order;

    /** Whether this is a read archive. */
    bool read;

    /** Whether this is a write archive. */
    bool write;

    /** Whether this is a persistent archive. */
    bool persistent;

protected:
    /**
     * Serialize raw bytes.
     *
     * @param bytes Span containing the bytes.
     */
    virtual void serialize_bytes(
      std::span<std::byte> bytes) = 0;

public:
    /** Defaulted and deleted constructors. */
    Archive() = delete;
    Archive(const Archive&) = default;
    Archive(Archive&&) noexcept = default;

    /** Default destructor. */
    virtual ~Archive() noexcept = default;

    /**
     * Set up an archive.
     *
     * @param read Whether this is a read archive.
     * @param write Whether this is a write archive.
     * @param persistent Whether this is a persistent archive.
     * @param target_byte_order The target byte order for persistent archives.
     */
    Archive(
      bool read,
      bool write,
      bool persistent,
      std::endian target_byte_order = std::endian::native)
    : target_byte_order{
        persistent
          ? target_byte_order
          : std::endian::native}
    , read{read}
    , write{write}
    , persistent{persistent}
    {
        if(read && write)
        {
            throw std::runtime_error{
              "An archive cannot be both readable and writable."};
        }
    }

    /** Default assignments. */
    Archive& operator=(const Archive&) = default;
    Archive& operator=(Archive&&) noexcept = default;

    /** Get the position in the archive. */
    virtual std::size_t tell() = 0;

    /**
     * Seek to a position in the archive.
     *
     * @param pos The position to seek to.
     * @returns Returns the new position.
     */
    virtual std::size_t seek(std::size_t pos) = 0;

    /** Get the size of the archive. */
    virtual std::size_t size() = 0;

    /** Return the target byte order for this archive. Only used/relevant for persistent archives. */
    [[nodiscard]]
    std::endian get_target_byte_order() const
    {
        return target_byte_order;
    }

    /** Returns whether this is a read archive. */
    [[nodiscard]]
    bool is_reading() const
    {
        return read;
    }

    /** Returns whether this is a write archive. */
    [[nodiscard]]
    bool is_writing() const
    {
        return write;
    }

    /** Return whether this is a persistent archive. */
    [[nodiscard]]
    bool is_persistent() const
    {
        return persistent;
    }

    /**
     * Serialize byte span to little endian. That is, if `bytes.size()>1`,
     * the buffer is reversed on big endian architectures.
     *
     * @param bytes The bytes to serialize.
     */
    virtual void serialize(
      std::span<std::byte> bytes)
    {
        if(!is_persistent())
        {
            // in-memory archive.
            serialize_bytes(bytes);
        }
        else
        {
            // persistent archive.
            if(target_byte_order == std::endian::native)
            {
                serialize_bytes(bytes);
            }
            else
            {
                for(std::size_t i = bytes.size(); i > 0; --i)
                {
                    serialize_bytes({&bytes[i - 1], 1});    // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
        }
    }

    /**
     * Serialize a scalar type.
     *
     * @tparam T The scalar type.
     * @param s The scalar.
     */
    template<serializable_scalar T>
    Archive& operator&(T& s)
    {
        serialize(std::as_writable_bytes(std::span<T>(&s, 1)));
        return *this;
    }
};

/** A variable length integer. */
struct VLEInt
{
    /** The integer. */
    std::int64_t i{0};

    /** Defaulted constructors. */
    VLEInt() = default;
    VLEInt(const VLEInt&) = default;
    VLEInt(VLEInt&&) = default;

    /** Defaulted destructor. */
    ~VLEInt() = default;

    /** Defaulted assignments. */
    VLEInt& operator=(const VLEInt&) = default;
    VLEInt& operator=(VLEInt&&) = default;

    /**
     * Construct a variable length integer instance.
     * Just initializes the stored integer.
     *
     * @param i The integer.
     */
    explicit VLEInt(std::int64_t i)
    : i{i}
    {
    }
};

namespace detail
{

/**
 * ZigZag-encode a signed 64-bit integer into an unsigned value.
 *
 * Maps small negative and positive values to small unsigned integers so
 * they can be efficiently encoded using variable-length encoding.
 *
 * @param x The signed integer to encode.
 * @returns The ZigZag-encoded unsigned integer.
 */
constexpr std::uint64_t zigzag_encode(std::int64_t x)
{
    return (static_cast<std::uint64_t>(x) << 1) ^ static_cast<std::uint64_t>(x >> 63);
}

/**
 * Decode a ZigZag-encoded unsigned integer.
 *
 * @param x The ZigZag-encoded unsigned integer.
 * @returns The decoded signed integer.
 */
constexpr std::int64_t zigzag_decode(std::uint64_t x)
{
    return static_cast<std::int64_t>((x >> 1) ^ (~(x & 1) + 1));
}

/**
 * Write an unsigned variable-length integer.
 *
 * Encodes the value using a base-128 variable-length representation.
 *
 * @param ar The archive to use.
 * @param value The unsigned integer to encode.
 * @returns The input archive.
 */
inline Archive& write_vle_uint64(
  Archive& ar,
  std::uint64_t value)
{
    while(value >= 0x80)
    {
        std::uint8_t byte = (value & 0x7f) | 0x80;
        ar & byte;
        value >>= 7;
    }

    std::uint8_t byte = static_cast<std::uint8_t>(value);
    ar & byte;

    return ar;
}

/**
 * Read an unsigned variable-length integer.
 *
 * Decodes a base-128 variable-length integer into an unsigned value.
 *
 * @param ar The archive to use.
 * @param value Receives the decoded integer.
 * @returns The input archive.
 * @throws `SerializationError` If the encoded integer is malformed.
 */
inline Archive& read_vle_uint64(
  Archive& ar,
  std::uint64_t& value)
{
    value = 0;

    std::uint32_t shift = 0;

    for(std::size_t i = 0; i < 10; ++i)
    {
        std::uint8_t byte;
        ar & byte;

        value |= static_cast<std::uint64_t>(byte & 0x7f) << shift;

        if((byte & 0x80) == 0)
        {
            return ar;
        }

        shift += 7;
    }

    throw SerializationError{
      "Invalid variable-length integer (too many continuation bytes)."};
}

}    // namespace detail

/**
 * Serialize a variable length integer.
 *
 * @param ar The archive to use.
 * @param i The integer.
 * @returns The input archive.
 */
inline Archive& operator&(
  Archive& ar,
  VLEInt& value)
{
    if(ar.is_writing())
    {
        auto u = detail::zigzag_encode(value.i);
        return detail::write_vle_uint64(ar, u);
    }

    std::uint64_t u;
    detail::read_vle_uint64(ar, u);
    value.i = detail::zigzag_decode(u);

    return ar;
}

/** Serialization helper type for writing constants. */
template<typename T>
struct ConstantSerializer
{
    /** The constant to serialize. */
    const T& c;

    /** Delete default constructors. */
    ConstantSerializer() = delete;
    ConstantSerializer(const ConstantSerializer&) = delete;
    ConstantSerializer(ConstantSerializer&&) = delete;

    /** Delete assignments. */
    ConstantSerializer& operator=(const ConstantSerializer&) = delete;
    ConstantSerializer& operator=(ConstantSerializer&&) = delete;

    /** Default destructor. */
    ~ConstantSerializer() = default;

    /**
     * Construct a constant serializer.
     *
     * @param c The constant to serialize.
     */
    explicit ConstantSerializer(const T& c)
    : c{c}
    {
    }
};

/**
 * Serializer for constants.
 *
 * @note The constant is copied during serialization.
 *
 * @param ar The archive to use for serialization.
 * @param c Wrapper around the constant to serialize.
 * @returns The input archive.
 */
template<typename T>
Archive& operator&(
  Archive& ar,
  const ConstantSerializer<T>& c)
{
    std::remove_cv_t<std::remove_reference_t<T>> copy = c.c;
    ar & copy;
    return ar;
}

}    // namespace serial
