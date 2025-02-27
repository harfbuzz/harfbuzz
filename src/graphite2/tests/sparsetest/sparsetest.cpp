// SPDX-License-Identifier: MIT
// Copyright (C) 2012 SIL International
/*
Responsibility: Tim Eves
Description:
The test harness for the Sparse class. This validates the
sparse classe is working correctly.
*/

#include <iostream>
#include <string>
#include "inc/Sparse.h"

using namespace graphite2;

inline
sparse::value_type v(sparse::key_type k)
{
    return sparse::value_type(k, sparse::mapped_type(~k));
}

namespace
{
    const sparse::value_type data[] =
    {
            v(0),
            v(2),   v(3),   v(5),   v(7),   v(11),  v(13),  v(17),  v(19),  v(23),  v(29),
            v(31),  v(37),  v(41),  v(43),  v(47),  v(53),  v(59),  v(61),  v(67),  v(71),
            v(73),  v(79),  v(83),  v(89),  v(97),  v(101), v(103), v(107), v(109), v(113),
            v(127), v(131), v(137), v(139), v(149), v(151), v(157), v(163), v(167), v(173),
            v(179), v(181), v(191), v(193), v(197), v(199), v(211), v(223), v(227), v(229),
            v(233), v(239), v(241), v(251), v(257), v(263), v(269), v(271), v(277), v(281),
            v(283), v(293), v(307), v(311), v(313), v(317), v(331), v(337), v(347), v(349),
            v(353), v(359), v(367), v(373), v(379), v(383), v(389), v(397), v(401), v(409),
            v(419), v(421), v(431), v(433), v(439), v(443), v(449), v(457), v(461), v(463),
            v(467), v(479), v(487), v(491), v(499), v(503), v(509), v(521), v(523), v(541)
    };
    const sparse::value_type * const data_end = data+sizeof(data)/sizeof(sparse::value_type);

    const sparse::value_type bad_keys[] =
    {
            v(2),   v(3),   v(13),  v(7),   v(11),  v(17),  v(5),   v(19),  v(23),  v(29)
    };
    const sparse::value_type * const bad_keys_end = bad_keys+sizeof(bad_keys)/sizeof(sparse::value_type);

    const sparse::value_type dup_keys[] =
    {
            v(2),   v(3),   v(3),   v(7),   v(11),  v(17),  v(17),  v(19),  v(23),  v(29)
    };
    const sparse::value_type * const dup_keys_end = dup_keys+sizeof(dup_keys)/sizeof(sparse::value_type);

    const sparse::value_type no_zero[] =
    {
            sparse::value_type(2, 0),   sparse::value_type(3, 0),   sparse::value_type(7, 0),   sparse::value_type(11, 0),
            v(17),                      v(19),                      v(23),                      v(29)
    };
    const sparse::value_type * const no_zero_end = no_zero+sizeof(no_zero)/sizeof(sparse::value_type);
}

int main(int argc , char *argv[])
{
    sparse sp(data, data_end);

    // Check monotonicity is enforced. Array should be invalid
    if (sparse(bad_keys, bad_keys_end) == true)
        return 1;

    // Check duplicate key detection is enforced. Array should be invalid
    if (sparse(dup_keys, dup_keys_end) == true)
        return 2;

    // Check zero values are not stored
    if (sparse(no_zero, no_zero_end).capacity() != 4)
        return 3;

    // Check all values are stored
    if (sp.capacity() != sizeof(data)/sizeof(sparse::value_type))
        return 4;

    // Check the values we put in are coming out again
    for (int i = 0; i != sizeof(data)/sizeof(sparse::value_type); ++i)
    {
        if (sp[data[i].first] != data[i].second)
            return 5;
    }

    // Check the "missing" values return 0
    const sparse::value_type * d = data;
    for (int i = 0; i != data_end[-1].first+1; ++i)
    {
        if (i == (*d).first)
        {
            if (sp[i] != (*d++).second)
                return 6;
        }
        else
        {
            if (sp[i] != 0)
                return 7;
        }
    }

    std::cout << "key range:\t" << data[0].first << "-" << data_end[-1].first << std::endl
              << "key space size: " << data_end[-1].first - data[0].first << std::endl
              << "linear uint16_t array:" << std::endl
              << "\tcapacity:       " << data_end[-1].first+1 << std::endl
              << "\tresidency:      " << sizeof(data)/sizeof(sparse::value_type) << std::endl
              << "\tfill ratio:     " << 100.0f*(sizeof(data)/sizeof(sparse::value_type)/float(data_end[-1].first+1)) << "%" << std::endl
              << "\tsize:           " << (data_end[-1].first+1)*sizeof(uint16_t) << std::endl

              << "sparse uint16_t array:" << std::endl
              << "\tcapacity:       " << sp.capacity() << std::endl
              << "\tresidency:      " << sp.capacity() << std::endl
              << "\tfill ratio:     " << 100.0f << "%" << std::endl
              << "\tsize:           " << sp._sizeof() << std::endl;

    // Check indexing an default sparse array doesn't crash
    sparse def;
    for (int i = 0; i != 100; ++i)
    {
        if (def[i] != 0)
            return 8;
    }

    // Check indexing an empty sparse array doesn't crash
    sparse empty(data,data);
    for (int i = 0; i != 100; ++i)
    {
        if (empty[i] != 0)
            return 9;
    }

    return 0;
}

