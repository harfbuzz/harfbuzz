// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include "inc/Main.h"

#include <list>
#include "inc/list.hpp"

namespace gr2 = graphite2;

template<typename L>
void print_container(L const & c)
{
    for (auto &&v: c)
    {
        printf("%d ", v);
    }
    printf("\n");
}

template<typename L, typename R, class P>
void for_pair(L &l, R &r, P p)
{
    auto li = l.cbegin(), le = l.cend();
    auto ri = r.cbegin(), re = r.cend();
    while (li != le && ri != re) p(*li++, *ri++);
}


int main(int /*argc*/, char ** /*argv*/)
{
    std::list<int> stdList;
    gr2::list<int> grList;

    assert(stdList.size() == grList.size());

    for (int i = 0; i < 10; i++)
    {
        stdList.push_back(i * 2);
        grList.push_back(i * 2);
    }

    for_pair(stdList, grList, [](int s, int g) 
    {
        assert(s == g);
    });

    // test erase in middle
    stdList.erase(std::next(stdList.begin(),8));
    grList.erase(std::next(grList.begin(),8));

    stdList.erase(std::next(stdList.begin(),2), std::next(stdList.begin(),4));
    grList.erase(std::next(grList.begin(),2), std::next(grList.begin(),4));

    assert(stdList.size() == grList.size());
    for_pair(stdList, grList, [](int s, int g) 
    {
        assert(s == g);
    });

    // insert in middle
    stdList.insert(std::next(stdList.begin(), 3), 20);
    grList.insert(std::next(grList.begin(), 3), 20);

    // insert multiple in middle
    // stdList.insert(std::next(stdList.begin()), 4, 22);
    // grList.insert(std::next(grList.begin()), 4, 22);

    // insert at end
    stdList.insert(stdList.end(), 24);
    grList.insert(grList.end(), 24);

    // insert multiple at end
    // stdList.insert(stdList.end(), 2, 25);
    // grList.insert(grList.end(), 2, 25);

    // insert at start
    stdList.insert(stdList.begin(), 26);
    grList.insert(grList.begin(), 26);

    // insert multiple at start
    // stdList.insert(stdList.begin(), 3, 27);
    // grList.insert(grList.begin(), 3, 27);

    // test erase at start
    stdList.erase(stdList.begin(), std::next(stdList.begin()));
    grList.erase(grList.begin(), std::next(grList.begin()));

    stdList.erase(stdList.begin());
    grList.erase(grList.begin());

    // test erase at end
    stdList.erase(--stdList.end());
    grList.erase(--grList.end());


    print_container(stdList);
    print_container(grList);
    assert(stdList.size() == grList.size());
    for_pair(stdList, grList, [](int s, int g) 
    {
        assert(s == g);
    });

    // test erasing everything
    stdList.erase(stdList.begin(), stdList.end());
    grList.erase(grList.begin(), grList.end());

    assert(stdList.size() == grList.size());

    // test inserting just 1 element and erasing
    stdList.insert(stdList.begin(), 30);
    grList.insert(grList.begin(), 30);

    assert(stdList.size() == grList.size());
    assert(stdList.front() == grList.front());

    stdList.erase(stdList.begin());
    grList.erase(grList.begin());

    // stdList.insert(stdList.begin(), 1, 31);
    // grList.insert(grList.begin(), 1, 31);

    // assert(stdList.size() == grList.size());
    // assert(stdList.front() == grList.front());

    // stdList.erase(stdList.begin(), stdList.end());
    // grList.erase(grList.begin(), grList.end());

    assert(stdList.size() == grList.size());

    // check that push_back still works after a complete erase
    for (int i = 0; i < 4; i++)
    {
        stdList.push_back(i*3);
        grList.push_back(i*3);
    }

    assert(stdList.size() == grList.size());
    for_pair(stdList, grList, [](int s, int g) 
    {
        assert(s == g);
    });

    std::list<int> stdList2;
    gr2::list<int> grList2;

    stdList2.insert(stdList2.begin(), stdList.begin(), stdList.end());
    grList2.insert(grList2.begin(), grList.begin(), grList.end());

    print_container(stdList2);
    print_container(grList2);

    stdList.clear();
    grList.clear();
    for (int i = 0; i < 4; i++)
    {
        stdList.push_back(i*4);
        grList.push_back(i*4);
    }
    print_container(stdList);
    print_container(grList);
    print_container(stdList2);
    print_container(grList2);

    stdList2.insert(stdList2.begin(), stdList.begin(), stdList.end());
    grList2.insert(grList2.begin(), grList.begin(), grList.end());

    print_container(stdList);
    print_container(grList);
    print_container(stdList2);
    print_container(grList2);

    stdList.clear();
    grList.clear();
    for (int i = 0; i < 4; i++)
    {
        stdList.push_back(i*5);
        grList.push_back(i*5);
    }
    stdList2.insert(stdList2.end(), stdList.begin(), stdList.end());
    grList2.insert(grList2.end(), grList.begin(), grList.end());

    print_container(grList);
    print_container(stdList2);
    print_container(grList2);
    assert(stdList2.size() == grList2.size());
    for_pair(stdList2, grList2, [](int s, int g) 
    {
        assert(s == g);
    });


    std::list <int> stdList3;
    gr2::list <int> grList3;

//    stdList3.assign(10, 123);
//    grList3.assign(10, 123);
    assert(stdList3.size() == grList3.size());
    for_pair(stdList3, grList3, [](int s, int g) 
    {
        assert(s == g);
    });

    return 0;
}
