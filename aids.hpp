// Copyright 2020 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================
//
// aids — 0.6.0 — std replacement for C++. Designed to aid developers
// to a better programming experience.
//
// https://github.com/rexim/aids
//
// ============================================================
//
// ChangeLog (https://semver.org/ is implied)
//
//   0.6.0  swap
//   0.5.0  Equality operations for Maybe<T>
//   0.4.0  mod
//   0.3.0  Stretchy_Buffer
//   0.2.0  unwrap_into
//          print1 for long int
//   0.1.0  print1 for long unsigned int
//          print1 for int
//          Pad
//   0.0.3  bugfix for print1 of Maybe<T>
//   0.0.2  fix sign-unsigned integer comparison in aids::read_file_as_string_view
//   0.0.1  min, max, clamp,
//          defer,
//          Maybe<T>,
//          String_View,
//          print, println
//
// ============================================================
//
// Contributors:
//   Alexey Kutepov (github:rexim)
//   Aodhnait Étaín (github:aodhneine)

#ifndef AIDS_HPP_
#define AIDS_HPP_

#include <cassert>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace aids
{
    ////////////////////////////////////////////////////////////
    // ALGORITHM
    ////////////////////////////////////////////////////////////

    template <typename T>
    T min(T a, T b)
    {
        return a < b ? a : b;
    }

    template <typename T>
    T max(T a, T b)
    {
        return a > b ? a : b;
    }

    template <typename T>
    T clamp(T x, T low, T high)
    {
        return min(max(low, x), high);
    }

    template <typename T>
    T mod(T a, T b)
    {
        return (a % b + b) % b;
    }

    template <typename T>
    void swap(T *a, T *b)
    {
        T t = *a;
        *a = *b;
        *b = t;
    }

    ////////////////////////////////////////////////////////////
    // DEFER
    ////////////////////////////////////////////////////////////

    // https://www.reddit.com/r/ProgrammerTIL/comments/58c6dx/til_how_to_defer_in_c/
    template <typename F>
    struct saucy_defer {
        F f;
        saucy_defer(F f) : f(f) {}
        ~saucy_defer() { f(); }
    };

    template <typename F>
    saucy_defer<F> defer_func(F f)
    {
        return saucy_defer<F>(f);
    }

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x)    DEFER_2(x, __COUNTER__)
#define defer(code)   auto DEFER_3(_defer_) = ::aids::defer_func([&](){code;})

    ////////////////////////////////////////////////////////////
    // MAYBE
    ////////////////////////////////////////////////////////////

    template <typename T>
    struct Maybe
    {
        bool has_value;
        T unwrap;

        bool operator!=(const Maybe<T> &that) const
        {
            return !(*this == that);
        }

        bool operator==(const Maybe<T> &that) const
        {
            if (this->has_value && that.has_value) {
                return this->unwrap == that.unwrap;
            }

            return !this->has_value && !that.has_value;
        }
    };

#define unwrap_into(lvalue, maybe)              \
    do {                                        \
        auto maybe_var = (maybe);               \
        if (!maybe_var.has_value) return {};    \
        (lvalue) = maybe_var.unwrap;            \
    } while (0)

    ////////////////////////////////////////////////////////////
    // STRING_VIEW
    ////////////////////////////////////////////////////////////

    struct String_View
    {
        size_t count;
        const char *data;

        [[nodiscard]]
        String_View trim_begin(void) const
        {
            String_View view = *this;

            while (view.count != 0 && isspace(*view.data)) {
                view.data  += 1;
                view.count -= 1;
            }
            return view;
        }

        [[nodiscard]]
        String_View trim_end(void) const
        {
            String_View view = *this;

            while (view.count != 0 && isspace(*(view.data + view.count - 1))) {
                view.count -= 1;
            }
            return view;
        }

        [[nodiscard]]
        String_View trim(void) const
        {
            return trim_begin().trim_end();
        }

        void chop_back(size_t n)
        {
            count -= n < count ? n : count;
        }

        void chop(size_t n)
        {
            if (n > count) {
                data += count;
                count = 0;
            } else {
                data  += n;
                count -= n;
            }
        }

        void grow(size_t n)
        {
            count += n;
        }

        String_View chop_by_delim(char delim)
        {
            assert(data);

            size_t i = 0;
            while (i < count && data[i] != delim) i++;
            String_View result = {i, data};
            chop(i + 1);

            return result;
        }

        String_View chop_word(void)
        {
            *this = trim_begin();

            size_t i = 0;
            while (i < count && !isspace(data[i])) i++;

            String_View result = { i, data };

            count -= i;
            data  += i;

            return result;
        }

        template <typename Integer>
        Maybe<Integer> from_hex() const
        {
            Integer result = {};

            for (size_t i = 0; i < count; ++i) {
                Integer x = data[i];
                if ('0' <= x && x <= '9') {
                    x = (Integer) (x - '0');
                } else if ('a' <= x && x <= 'f') {
                    x = (Integer) (x - 'a' + 10);
                } else if ('A' <= x && x <= 'F') {
                    x = (Integer) (x - 'A' + 10);
                } else {
                    return {};
                }
                result = result * (Integer) 0x10 + x;
            }

            return {true, result};
        }

        template <typename Integer>
        Maybe<Integer> as_integer() const
        {
            Integer sign = 1;
            Integer number = {};
            String_View view = *this;

            if (view.count == 0) {
                return {};
            }

            if (*view.data == '-') {
                sign = -1;
                view.chop(1);
            }

            while (view.count) {
                if (!isdigit(*view.data)) {
                    return {};
                }
                number = number * 10 + (*view.data - '0');
                view.chop(1);
            }

            return { true, number * sign };
        }

        Maybe<float> as_float() const
        {
            char buffer[300] = {};
            memcpy(buffer, data, min(sizeof(buffer) - 1, count));
            char *endptr = NULL;
            float result = strtof(buffer, &endptr);

            if (buffer > endptr || (size_t) (endptr - buffer) != count) {
                return {};
            }

            return {true, result};
        }


        String_View subview(size_t start, size_t count) const
        {
            if (start + count <= this->count) {
                return {count, data + start};
            }

            return {};
        }

        bool operator==(String_View view) const
        {
            if (this->count != view.count) return false;
            return memcmp(this->data, view.data, this->count) == 0;
        }

        bool operator!=(String_View view) const
        {
            return !(*this == view);
        }

        bool has_prefix(String_View prefix) const
        {
            return prefix.count <= this->count
                && this->subview(0, prefix.count) == prefix;
        }
    };

    String_View operator ""_sv(const char *data, size_t count)
    {
        return {count, data};
    }

    String_View cstr_as_string_view(const char *cstr)
    {
        return {strlen(cstr), cstr};
    }

    void print1(FILE *stream, String_View view)
    {
        fwrite(view.data, 1, view.count, stream);
    }

    Maybe<String_View> read_file_as_string_view(const char *filename)
    {
        FILE *f = fopen(filename, "rb");
        if (!f) return {};
        defer(fclose(f));

        int err = fseek(f, 0, SEEK_END);
        if (err < 0) return {};

        long size = ftell(f);
        if (size < 0) return {};

        err = fseek(f, 0, SEEK_SET);
        if (err < 0) return {};

        auto data = malloc(size);
        if (!data) return {};

        size_t read_size = fread(data, 1, size, f);
        if (read_size != (size_t) size && ferror(f)) return {};

        return {true, {static_cast<size_t>(size), static_cast<const char*>(data)}};
    }

    ////////////////////////////////////////////////////////////
    // STRETCHY BUFFER
    ////////////////////////////////////////////////////////////

    struct Stretchy_Buffer
    {
        size_t capacity;
        size_t size;
        char *data;

        void push(const char *that_data, size_t that_size)
        {
            if (size + that_size > capacity) {
                capacity = 2 * capacity + that_size;
                data = (char*)realloc((void*)data, capacity);
            }

            memcpy(data + size, that_data, that_size);
            size += that_size;
        }

        template <typename T>
        void push(T x)
        {
            push((char*) &x, sizeof(x));
        }
    };

    void print1(FILE *stream, Stretchy_Buffer buffer)
    {
        fwrite(buffer.data, 1, buffer.size, stream);
    }

    ////////////////////////////////////////////////////////////
    // PRINT
    ////////////////////////////////////////////////////////////

    void print1(FILE *stream, const char *s)
    {
        fwrite(s, 1, strlen(s), stream);
    }

    void print1(FILE *stream, char *s)
    {
        fwrite(s, 1, strlen(s), stream);
    }

    void print1(FILE *stream, char c)
    {
        fputc(c, stream);
    }

    void print1(FILE *stream, float f)
    {
        fprintf(stream, "%f", f);
    }

    void print1(FILE *stream, unsigned long long x)
    {
        fprintf(stream, "%lld", x);
    }

    void print1(FILE *stream, long unsigned int x)
    {
        fprintf(stream, "%lu", x);
    }

    void print1(FILE *stream, int x)
    {
        fprintf(stream, "%d", x);
    }

    void print1(FILE *stream, long int x)
    {
        fprintf(stream, "%ld", x);
    }

    template <typename ... Types>
    void print(FILE *stream, Types... args)
    {
        (print1(stream, args), ...);
    }

    template <typename T>
    void print1(FILE *stream, Maybe<T> maybe)
    {
        if (!maybe.has_value) {
            print(stream, "None");
        } else {
            print(stream, "Some(", maybe.unwrap, ")");
        }
    }

    template <typename ... Types>
    void println(FILE *stream, Types... args)
    {
        (print1(stream, args), ...);
        print1(stream, '\n');
    }

    struct Pad
    {
        size_t n;
        char c;
    };

    void print1(FILE *stream, Pad pad)
    {
        for (size_t i = 0; i < pad.n; ++i) {
            fputc(pad.c, stream);
        }
    }
}

#endif  // AIDS_HPP_
