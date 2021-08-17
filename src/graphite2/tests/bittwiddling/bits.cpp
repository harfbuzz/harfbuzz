/*-----------------------------------------------------------------------------
Copyright (C) 2011 SIL International
Responsibility: Tim Eves

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should also have received a copy of the GNU Lesser General Public
    License along with this library in the file named "LICENSE".
    If not, write to the Free Software Foundation, 51 Franklin Street,
    Suite 500, Boston, MA 02110-1335, USA or visit their web page on the
    internet at http://www.fsf.org/licenses/lgpl.html.

Alternatively, the contents of this file may be used under the terms of the
Mozilla Public License (http://mozilla.org/MPL) or the GNU General Public
License, as published by the Free Software Foundation, either version 2
of the License or (at your option) any later version.

Description:
The test harness for the Sparse class. This validates the
sparse classe is working correctly.
-----------------------------------------------------------------------------*/

#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include "inc/Main.h"
#include "inc/bits.h"

using namespace graphite2;

#if (defined(__x86_64__) && !defined(__ILP32__)) || defined(_WIN64)
	#define HAS_64BIT
#endif

#define maskoff(n) (size_t(-1L) >> (8*sizeof(size_t) - n))

#define pat(b)   0x01##b, 0x03##b, 0x07##b, 0x0f##b
#define pat8(b)  pat(b), pat(f##b)
#define pat16(b) pat8(b), pat8(ff##b)
#define pat32(b) pat16(b), pat16(ffff##b)
#define pat64(b) pat32(b), pat32(ffffffff##b)

#define patterns(bw) \
    uint##bw##_t const  u##bw##_pat[] = {0, pat##bw(UL) }; \
    int##bw##_t const * s##bw##_pat   = reinterpret_cast<int##bw##_t const *>(u##bw##_pat)

//#define BENCHMARK 40000000
#if defined BENCHMARK
#define benchmark() for (int n = BENCHMARK; n; --n)
#else
#define benchmark()
#endif


typedef     unsigned long long   uint64;
typedef     signed long long     int64;
namespace
{
    patterns(8);
    patterns(16);
    patterns(32);
#if defined(HAS_64BIT)
    patterns(64);
#endif

    int ret = 0;


    template<typename T>
    struct type_name {};

    // template<typename T>
    // inline
    // std::ostream & operator << (std::ostream & o, type_name<T>)
    // {
    //     if (!std::numeric_limits<T>::is_signed) o.put('u');
    //     o << "int" << std::dec << sizeof(T)*8;
    // }

    template<typename T>
    inline
    void test_bit_set_count(const T pat[])
    {
        for (unsigned int p = 0; p <= sizeof(T)*8; ++p)
        {
#if !defined BENCHMARK
            std::cout << "bit_set_count("
                        << (!std::numeric_limits<T>::is_signed ? "uint" : "int")
                        << std::dec << sizeof(T)*8 << "(0x"
                            << std::hex
                            << std::setw(sizeof(T)*2)
                            << std::setfill('0')
                            << (pat[p] & maskoff(8*sizeof(T)))
                      << ")) -> "
                        << std::dec
                        << bit_set_count(pat[p]) << std::endl;
#endif
            if (bit_set_count(pat[p]) != p)
            {
                std::cerr << " != " << std::dec << p << std::endl;
                ret = sizeof(T);
            }
        }
    }

}

int main(int argc , char *argv[])
{
    assert(sizeof(uint64) == 8);

    benchmark()
    {
        // Test bit_set_count
        test_bit_set_count(u8_pat);
        test_bit_set_count(s8_pat);
        test_bit_set_count(u16_pat);
        test_bit_set_count(s16_pat);
        test_bit_set_count(u32_pat);
        test_bit_set_count(s32_pat);
#if defined(HAS_64BIT)
        test_bit_set_count(u64_pat);
        test_bit_set_count(s64_pat);
#endif
    }

    return ret;
}
