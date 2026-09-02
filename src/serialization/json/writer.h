/**
 * Software Rasterizer Playground.
 *
 * JSON writer.
 *
 * \author Felix Lubbe
 * \copyright Copyright (c) 2026
 * \license Distributed under the MIT software license (see accompanying LICENSE.txt).
 */

#pragma once

#include <cassert>
#include <charconv>
#include <cstdio>
#include <string_view>
#include <type_traits>
#include <vector>

#include "containers/string.h"

namespace serial::json
{

/** A JSON writer. */
class JsonWriter
{
    /** Indentation size in whitespaces. */
    std::size_t indentation_size;

    /** Whether to use a compacted format: no indentation, no newlines. */
    bool use_compacted_format;

    enum class ContextType
    {
        Object,
        Array,
    };

    struct Context
    {
        ContextType type;
        bool first = true;
        bool expecting_value = false;
    };

    std::vector<Context> stack;

    /** JSON string. */
    swr::string json;

private:
    /*
     * Helpers.
     */

    void append(std::string_view sv)
    {
        json.append(sv.data(), sv.size());
    }

    void write_newline()
    {
        if(!use_compacted_format)
        {
            append("\n");
        }
    }

    void write_indent()
    {
        if(!use_compacted_format)
        {
            json.append(stack.size() * indentation_size, ' ');
        }
    }

    void prepare_value()
    {
        if(!stack.empty())
        {
            auto& current = stack.back();
            if(current.type == ContextType::Object)
            {
                assert(current.expecting_value
                       && "Values in an object must be preceded by a key (e.g. write_key).");
                current.expecting_value = false;
            }
            else if(current.type == ContextType::Array)
            {
                begin_value();
            }
        }
    }

    void begin_value()
    {
        if(stack.empty())
        {
            return;
        }

        auto& current = stack.back();

        if(!current.first)
        {
            append(",");
            if(current.type == ContextType::Array)
            {
                write_newline();
            }
        }
        else
        {
            current.first = false;
            if(current.type == ContextType::Array)
            {
                write_newline();
            }
        }

        if(current.type == ContextType::Array)
        {
            write_indent();
        }
    }

    void write_string(std::string_view str)
    {
        append("\"");
        for(char c: str)
        {
            switch(c)
            {
            case '"': append("\\\""); break;
            case '\\': append("\\\\"); break;
            case '\b': append("\\b"); break;
            case '\f': append("\\f"); break;
            case '\n': append("\\n"); break;
            case '\r': append("\\r"); break;
            case '\t': append("\\t"); break;
            default:
                if(static_cast<unsigned char>(c) < 0x20)
                {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    append(buf);
                }
                else
                {
                    json.push_back(c);
                }
                break;
            }
        }
        append("\"");
    }

public:
    /**
     * Construct a JSON writer.
     *
     * @param indentation_size Indentation size. Defaults to 4.
     * @param use_compacted_format Whether to use a compacted format: no indentation, no newlines.
     *     Defaults to `false`.
     */
    JsonWriter(
      std::size_t indentation_size = 4,
      bool use_compacted_format = false)
    : indentation_size{indentation_size}
    , use_compacted_format{use_compacted_format}
    {
    }

    /** Returns the formatted JSON string. */
    [[nodiscard]]
    const swr::string& get() const noexcept
    {
        return json;
    }

    void begin_object()
    {
        prepare_value();
        append("{");
        stack.push_back({.type = ContextType::Object, .first = true});
    }

    void end_object()
    {
        assert(!stack.empty()
               && "Called end_object() when stack was empty.");
        assert(stack.back().type == ContextType::Object
               && "Mismatched container: expected end_object() but current stack context is Array.");
        assert(!stack.back().expecting_value
               && "Dangling key in object: write_key was called without an accompanying value.");

        bool had_items = !stack.back().first;
        stack.pop_back();

        if(had_items)
        {
            write_newline();
            write_indent();
        }

        append("}");
    }

    void begin_array()
    {
        prepare_value();
        append("[");
        stack.push_back({.type = ContextType::Array, .first = true});
    }

    void end_array()
    {
        assert(!stack.empty()
               && "Called end_array() when stack was empty.");
        assert(stack.back().type == ContextType::Array
               && "Mismatched container: expected end_array() but current stack context is Object.");

        bool had_items = !stack.back().first;
        stack.pop_back();

        if(had_items)
        {
            write_newline();
            write_indent();
        }

        append("]");
    }

    void write_key(std::string_view key)
    {
        assert(!stack.empty()
               && "Cannot write key at root level.");
        assert(stack.back().type == ContextType::Object
               && "Keys can only be written inside an Object context.");
        assert(!stack.back().expecting_value
               && "Cannot write two keys sequentially without a value in between.");

        begin_value();
        write_newline();
        write_indent();
        write_string(key);
        append(use_compacted_format ? ":" : ": ");

        stack.back().expecting_value = true;
    }

    /*
     * Value Writers.
     */

    /**
     * Write integral and floating-point numeric types using std::to_chars.
     */
    template<typename T>
        requires(std::is_arithmetic_v<T>
                 && !std::is_same_v<T, bool>
                 && !std::is_same_v<T, char>)
    void write_val(T value)
    {
        prepare_value();

        char buffer[64] = {0};

        auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if(ec == std::errc{})
        {
            append(std::string_view(buffer, ptr - buffer));
        }
    }

    void write_val(std::string_view value)
    {
        prepare_value();
        write_string(value);
    }

    void write_val(const char* value)
    {
        write_val(std::string_view(value));
    }

    void write_val(bool value)
    {
        prepare_value();
        append(value ? "true" : "false");
    }

    void write_null()
    {
        prepare_value();
        append("null");
    }

    /*
     * Convenience Key-Value Pair Writers.
     */

    template<typename T>
    void write_key_value(std::string_view key, T&& value)
    {
        write_key(key);
        write_val(std::forward<T>(value));
    }

    void write_key_null(std::string_view key)
    {
        write_key(key);
        write_null();
    }
};

}    // namespace serial::json
