/**
 * Software Rasterizer Playground.
 *
 * Hashing.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cassert>
#include <stdexcept>
#include <span>

#include <xxhash.h>

/** A hashing error. */
struct HashError
: public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/** Hasher, using xxHash64. */
class Hasher
{
    /** xxHash64 state. */
    XXH64_state_t* state;

public:
    /**
     * Construct a hasher instance.
     *
     * @param seed An optional seed to alter the hash result predictably.
     * @throws Throws `HashError` is state creation failed.
     */
    explicit Hasher(
      std::uint64_t seed = 0ull)
    : state{XXH64_createState()}
    {
        if(state == nullptr)
        {
            throw HashError{
              "Unable to create hash state."};
        }

        XXH64_reset(state, seed);
    }

    /** Deleted copy constructor. */
    Hasher(const Hasher&) = delete;

    /** Move constructor. */
    Hasher(Hasher&& other) noexcept
    : state{std::exchange(other.state, nullptr)}
    {
    }

    /** Destructor. Frees the hash state. */
    ~Hasher()
    {
        XXH64_freeState(state);
    }

    /** Deleted copy operators. */
    Hasher& operator=(const Hasher&) = delete;

    /** Move operator. */
    Hasher& operator=(Hasher&& other) noexcept
    {
        if(this != &other)
        {
            XXH64_freeState(state);
            state = std::exchange(other.state, nullptr);
        }
        return *this;
    }

    /**
     * Update the hash.
     *
     * @tparam T Trivially copyable non-pointer type.
     * @param values Values to update the hash with.
     */
    template<typename T>
        requires(
          std::is_trivially_copyable_v<T>
          && !std::is_pointer_v<T>)
    void update(std::span<T> values)
    {
        [[maybe_unused]] const auto ret = XXH64_update(
          state,
          values.data(),
          values.size_bytes());
        assert(ret == XXH_OK);
    }

    /** Return the hash digest. */
    std::uint64_t digest() const
    {
        return XXH64_digest(state);
    }
};
