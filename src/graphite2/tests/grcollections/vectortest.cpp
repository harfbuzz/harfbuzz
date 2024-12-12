// SPDX-License-Identifier: MIT
// Copyright 2010, SIL International, All rights reserved.
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include "inc/Main.h"

#include <vector>
#include "inc/vector.hpp"

namespace gr2 = graphite2;

template<typename T>
void printVector(T & v)
{
    for (size_t i = 0; i < v.size(); i++)
    {
        printf("%d ", v[i]);
    }
    printf("\n");
}

int main(int /*argc*/, char ** /*argv*/)
{
    std::vector<int> stdVector(10u, 0);
    gr2::vector<int> grList(10u, 0);

    assert(stdVector.size() == grList.size());

    for (int i = 0; i < 10; i++)
    {
        stdVector[i] = i * 2;
        grList[i] = i * 2;
    }

    for (size_t i = 0; i < stdVector.size(); i++)
    {
        assert(grList[i] == stdVector[i]);
    }

    // test erase in middle
    stdVector.erase(std::next(stdVector.begin(),8));
    grList.erase(std::next(grList.begin(),8));

    stdVector.erase(std::next(stdVector.begin(),2), std::next(stdVector.begin(),4));
    grList.erase(std::next(grList.begin(),2), std::next(grList.begin(),4));

    assert(stdVector.size() == grList.size());
    for (size_t i = 0; i < stdVector.size(); i++)
    {
        assert(grList[i] == stdVector[i]);
    }

    // insert in middle
    stdVector.insert(std::next(stdVector.begin(),3), 20);
    grList.insert(std::next(grList.begin(),3), 20);

    // insert multiple in middle
    stdVector.insert(std::next(stdVector.begin(),1), 4u, 22);
    grList.insert(std::next(grList.begin(),1), 4u, 22);

    // insert at end
    stdVector.insert(stdVector.end(), 24);
    grList.insert(grList.end(), 24);

    stdVector.insert(stdVector.end(), 2, 25);
    grList.insert(grList.end(), 2u, 25);

    // insert at start
    stdVector.insert(stdVector.begin(), 26);
    grList.insert(grList.begin(), 26);

    stdVector.insert(stdVector.begin(), 3, 27);
    grList.insert(grList.begin(), 3u, 27);

    // test erase at start
    stdVector.erase(stdVector.begin(), stdVector.begin() + 1);
    grList.erase(grList.begin(), grList.begin() + 1);

    stdVector.erase(stdVector.begin());
    grList.erase(grList.begin());

    // test erase at end
    stdVector.erase(stdVector.begin() + stdVector.size() - 1);
    grList.erase(grList.begin() + (grList.size() - 1));


    printVector(stdVector);
    printVector(grList);
    assert(stdVector.size() == grList.size());
    for (size_t i = 0; i < stdVector.size(); i++)
    {
        assert(grList[i] == stdVector[i]);
    }

    // test erasing everything
    stdVector.erase(stdVector.begin(), stdVector.end());
    grList.erase(grList.begin(), grList.end());

    assert(stdVector.size() == grList.size());

    // test inserting just 1 element and erasing
    stdVector.insert(stdVector.begin(), 30);
    grList.insert(grList.begin(), 30);

    assert(stdVector.size() == grList.size());
    assert(stdVector[0] == grList[0]);

    stdVector.erase(stdVector.begin());
    grList.erase(grList.begin());

    stdVector.insert(stdVector.begin(), 1u, 31);
    grList.insert(grList.begin(), 1u, 31);

    assert(stdVector.size() == grList.size());
    assert(stdVector[0] == grList[0]);

    stdVector.erase(stdVector.begin(), stdVector.end());
    grList.erase(grList.begin(), grList.end());

    assert(stdVector.size() == grList.size());

    // check that push_back still works after a complete erase
    for (int i = 0; i < 4; i++)
    {
        stdVector.push_back(i*3);
        grList.push_back(i*3);
    }

    assert(stdVector.size() == grList.size());
    for (size_t i = 0; i < stdVector.size(); i++)
    {
        assert(grList[i] == stdVector[i]);
    }

    std::vector<int> stdVector2;
    gr2::vector<int> grList2;

    stdVector2.reserve(8);
    grList2.reserve(8);

    stdVector2.insert(stdVector2.begin(), stdVector.begin(), stdVector.end());
    grList2.insert(grList2.begin(), grList.begin(), grList.end());

    printVector(grList);
    printVector(stdVector2);
    printVector(grList2);

    for (int i = 0; i < 4; i++)
    {
        stdVector[i] = (i*4);
        grList[i] = (i*4);
    }
    stdVector2.insert(stdVector2.begin(), stdVector.begin(), stdVector.end());
    grList2.insert(grList2.begin(), grList.begin(), grList.end());

    printVector(grList);
    printVector(stdVector2);
    printVector(grList2);

    for (int i = 0; i < 4; i++)
    {
        stdVector[i] = (i*5);
        grList[i] = (i*5);
    }
    stdVector2.insert(stdVector2.end(), stdVector.begin(), stdVector.end());
    grList2.insert(grList2.end(), grList.begin(), grList.end());

    printVector(grList);
    printVector(stdVector2);
    printVector(grList2);
    assert(stdVector2.size() == grList2.size());
    for (size_t i = 0; i < stdVector2.size(); i++)
    {
        assert(grList2[i] == stdVector2[i]);
    }

    std::vector <int> stdVector3;
    gr2::vector <int> grList3;

//    stdVector3.assign(10, 123);
//    grList3.assign(10, 123);
    assert(stdVector3.size() == grList3.size());
    for (size_t i = 0; i < stdVector3.size(); i++)
    {
        assert(grList3[i] == stdVector3[i]);
    }

    return 0;
}
