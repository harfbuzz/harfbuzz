// SPDX-License-Identifier: MIT
// Copyright 2014, SIL International, All rights reserved.
#include "inc/Intervals.h"
#include <utility>
#include <vector>
#include <cstdio>
#include <cstddef>
#include <cassert>

namespace gr2 = graphite2;

typedef std::pair<float, float> fpair;
typedef std::vector<fpair> fvector;
typedef fvector::iterator ifvector;

static int testCount = 0;

void printRanges(const char *pref, gr2::IntervalSet &is)
{
    printf ("%s: [ ", pref);
    for (gr2::IntervalSet::ivtpair s = is.begin(), end = is.end(); s != end; ++s)
        printf("(%.1f, %.1f) ", s->first, s->second);
    printf ("]\n");
}

int doTest(const char *pref, gr2::IntervalSet &is, fvector &fv)
{
    bool pass = true;
    gr2::IntervalSet::ivtpair s = is.begin(), e = is.end();
    ifvector bs = fv.begin(), be = fv.end();
    for ( ; s != e && bs != be; ++s, ++bs)
    {
        if (*s != *bs)
        {
            pass = false;
            break;
        }
    }
    if (bs != be || s != e)
        pass = false;
    printf("%d %s) ", ++testCount, pass ? "pass" : "FAIL");
    printRanges(pref, is);
    return pass ? 0 : 1;
}

void printFloat(const char *pref, float val)
{
    printf ("%s: [ %.1f ]\n", pref, val);
}

int doFloatTest(const char *pref, float test, float base)
{
    bool pass = (test == base);
    printf ("%d %s) %s: [ %.1f == %.1f ]\n", ++testCount, (pass ? "pass" : "FAIL"), pref, test, base);
    return (pass ? 0 : 1);
}

int main(int /*argc*/, char ** /*argv*/)
{
    int res = 0;
    fvector base;
    float fres;
    gr2::IntervalSet test, test2, testl;

    test.clear();
    base.clear();
    test.add(fpair(10., 100.));
    base.push_back(fpair(10., 100.));
    res += doTest("Init test", test, base);

    test.remove(fpair(30., 50.));
    base.clear();
    base.push_back(fpair(10., 30.));
    base.push_back(fpair(50., 100.));
    res += doTest("remove(30,50)", test, base);

    test.remove(fpair(45., 60.));
    base.back() = fpair(60., 100.);
    res += doTest("remove(45,60)", test, base);

    test.remove(fpair(80., 90.));
    base.clear();
    base.push_back(fpair(10., 30.));
    base.push_back(fpair(60., 80.));
    base.push_back(fpair(90., 100.));
    res += doTest("remove(80,90)", test, base);

    test2.clear();
    test2.add(fpair(10., 100.));
    test2.remove(fpair(20., 40.));
    test2.remove(fpair(45., 50.));
    test2.remove(fpair(55., 85.));
    base.clear();
    base.push_back(fpair(10., 20.));
    base.push_back(fpair(40., 45.));
    base.push_back(fpair(50., 55.));
    base.push_back(fpair(85., 100.));
    res += doTest("Init test2", test2, base);
    base.clear();
    base.push_back(fpair(20., 30.));
    base.push_back(fpair(60., 80));
    test.remove(test2);
    res += doTest("test - test2", test, base);

    test.clear();
    test.add(fpair(10., 85.));
    test.remove(fpair(30., 40.));
    base.clear();
    base.push_back(fpair(10., 30.));
    base.push_back(fpair(40., 85.));
    res += doTest("Init test3", test, base);
    test2.clear();
    test2.add(fpair(20., 90.));
    test2.remove(fpair(50., 60.));
    test2.remove(fpair(70., 80.));
    base.clear();
    base.push_back(fpair(20., 50.));
    base.push_back(fpair(60., 70.));
    base.push_back(fpair(80., 90.));
    res += doTest("Init test4", test2, base);
    test.remove(test2);
    base.clear();
    base.push_back(fpair(10., 20.));
    base.push_back(fpair(50., 60.));
    base.push_back(fpair(70., 80.));
    res += doTest("test3 - test4", test, base);
    testl = test.locate(fpair(30., 35.));
    base.clear();
    base.push_back(fpair(-20., -15.));
    base.push_back(fpair(20., 25.));
    base.push_back(fpair(40., 45.));
    res += doTest("locate(30,35)", testl, base);
    fres = testl.findClosestCoverage(0.);
    res += doFloatTest("Cover(0)", fres, -15.);
    fres = testl.findClosestCoverage(23.);
    res += doFloatTest("Cover(23)", fres, 23.);

    test.clear();
    test.remove(fpair(10., 20.));
    base.clear();
    res += doTest("Removing from empty", test, base);

    test.clear();
    test.add(fpair(10., 50.));
    test.add(fpair(20., 30.));
    test.add(fpair(40., 60.));
    test.add(fpair(60., 80.));
    test.add(fpair(90., 90.));
    test.add(fpair(80., 100.));
    base.clear();
    base.push_back(fpair(10., 100.));
    res += doTest("Slow add 10-100", test, base);

    test.clear();
    test.add(fpair(-447.149, 121.39));
    testl = test.locate(fpair(-487.753, -106.255));
    printRanges("          locate(-487,-186) in (-447,121)", testl);
    fres = testl.findClosestCoverage(0.);
    printFloat("          bestfit(0)", fres);

    test.clear();
    test.add(fpair(10., 90.));
    test2.clear();
    test2.add(fpair(50., 60.));
    test.remove(test2);
    test2.clear();
    test2.add(fpair(30., 40.));
    test.remove(test2);
    base.clear();
    base.push_back(fpair(10., 30.));
    base.push_back(fpair(40., 50.));
    base.push_back(fpair(60., 90.));
    res += doTest("(10,90)-[(50,60)]-[(30,40)]", test, base);

    test.clear();
    test.add(fpair(10., 90.));
    test2.clear();
    test2.add(fpair(20., 30.));
    test2.add(fpair(25., 40.));
    test.remove(test2);
    base.clear();
    base.push_back(fpair(10., 20.));
    base.push_back(fpair(40., 90.));
    res += doTest("(10,90)-[(20,30),(25,40)]", test, base);

    test.clear();
    test.add(fpair(10., 30.));
    test.add(fpair(40., 50.));
    test.add(fpair(60., 90.));
    test2.clear();
    test2.add(fpair(20., 80.));
    test.remove(test2);
    base.clear();
    base.push_back(fpair(10., 20.));
    base.push_back(fpair(80., 90.));
    res += doTest("Test 4 remove spans", test, base);
    test.clear();
    test.add(fpair(10., 30.));
    test.add(fpair(40., 50.));
    test.add(fpair(60., 90.));
    test.remove(fpair(20., 80.));
    res += doTest("Test 5 remove pair", test, base);

    test.clear();
    test.add(fpair(10., 30.));
    test.add(fpair(40., 50.));
    test.add(fpair(60., 90.));
    test2.clear();
    test2.add(15., 45.);
    test2.add(65., 70.);
    test2.add(75., 80.);
    test.intersect(test2);
    base.clear();
    base.push_back(fpair(15., 30.));
    base.push_back(fpair(40., 45.));
    base.push_back(fpair(65., 70.));
    base.push_back(fpair(75., 80.));
    res += doTest("Test 6 intersection", test, base);

    test.clear();
    test.add(1332, 1433);
    test.remove(1356, 1627);
    base.clear();
    base.push_back(fpair(1332, 1356));
    res += doTest("Test 7 right overlap removal", test, base);

    return res;
}
