#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (C) 2013 SIL International
import graphite2
import itertools
import math
import os
import random
import resource
import sys
import time
from multiprocessing import Pool, current_process
from argparse import ArgumentParser


class Estimator:
    def __init__(self, max):
        self.__base = time.time()
        self.__max = float(max)
        self.__m = 0
        self.__t = 1
        self.__st = self.__base
        self.__ct = 0
        self.modulus = 10

    def sample(self, current):
        ct = time.time()
        if (ct - self.__st <= opts.status):
            return None
        else:
            self.modulus = int(round(math.pow(
                                10,
                                math.floor(math.log10(current - self.__ct)))))
        self.__ct = current
        x = current / (ct-self.__base)
        k = (ct-self.__base)/opts.status+1
        m = self.__m + (x - self.__m)/k
        s = (k-1) and (self.__t + (x - self.__m)*(x - m)) / (k-1)
        m = self.bayes(x, s, self.__m, self.__t)
        self.__t = s
        self.__m = m
        self.__st = ct
        return (x, time.ctime(ct + (self.__max - current)/m) if m else 'never')

    @staticmethod
    def bayes(x, s, m, t):
        st = s + t
        return m*s/st + x*t/st


class MultiProc(object):
    chunksize = 10

    def __init__(self, doclass, *args):
        self.pool = Pool(initializer=self.initialize,
                         initargs=[doclass, args],
                         processes=opts.cpus)

    def initialize(self, doclass, args):
        proc = current_process()
        proc.inst = doclass(*args)

    def execute(self, top):
        if top == 0:
            myiter = itertools.repeat(1)
        else:
            myiter = range(top)
        e = Estimator(top)
        try:
            results = self.pool.imap_unordered(multiproctest, myiter,
                                               self.chunksize)
            for count, res in enumerate(results):
                if count % e.modulus == 0:
                    sam = e.sample(count)
                    if sam:
                        sys.stdout.write("{0} - {1:8.2f}: {2}\r".format(
                                    count,
                                    sam[0],
                                    (sam[1] if top else "")))
                        sys.stdout.flush()
                yield (count, res)
        except KeyboardInterrupt:
            sys.stdout.write("\n")
            sys.exit(0)


def multiproctest(val):
    proc = current_process()
    return proc.inst.test()


class Test(object):
    chars = " %()-.3=[]A{}\u0627\u064b\u0663\u08f1" \
            "\u200b\u202a\u202b\u202c\u202d\u202e\u2066\u2067\u2068\u2069"
    seed = None

    def __init__(self, grface, grfont):
        self.random = random.Random()
        self.random.seed(self.seed)
        Test.seed = self.random.random()
        self.grface = grface
        self.font = grfont
        self.rtl = 1
        self.feats = {}

    def test(self):
        length = int(self.random.betavariate(2, 50) * 1000)
        lenchars = len(self.chars) - 1

        def g(x): return self.chars[self.random.randint(0, lenchars)]
        text = "".join(map(g, range(length)))
        pid = os.fork()
        if pid < 0:
            print("Failed to fork")
            return (-1, text)
        elif pid > 0:
            (respid, status) = os.waitpid(pid, 0)
            if (status & 0xFF):
                return (status & 0x7F, text)  # don't care abut the core
            else:
                return (0, text)
        else:
            self.subtest(text)

    def subtest(self, text):
        if opts.timeout:
            resource.setrlimit(resource.RLIMIT_CPU,
                               (opts.timeout, opts.timeout))
        if opts.memory:
            mem = opts.memory * 1024 * 1024
            resource.setrlimit(resource.RLIMIT_AS, (mem, mem))
        graphite2.Segment(self.font, self.grface, "", text, self.rtl,
                          self.feats)
        sys.exit(0)


p = ArgumentParser()
p.add_argument('fontname', help='font file')
p.add_argument('-m', '--max', type=int, default=0,
               help="Number of tests to run [infinite]")
p.add_argument('--timeout', type=int,
               help="limit subprocess time in seconds")
p.add_argument('--memory', type=int,
               help="memory limit for subprocesses in MB")
p.add_argument('-s', '--status', type=int, default=10,
               help="Update status every n seconds")
p.add_argument('-j', '--cpus', type=int,
               help="Number of threads to run in parallel [num cpus]")
opts = p.parse_args()

try:
    grface = graphite2.Face(opts.fontname)
except Exception as err:
    print("Failed to load font {}: {}".format(opts.fontname, err),
          file=sys.stderr, flush=True)
    sys.exit(1)
grfont = graphite2.Font(grface, 16)

m = MultiProc(Test, grface, grfont)

for (count, res) in m.execute(opts.max):
    if res[0]: print("{0},{1}".format(res[0], repr(res[1])))

print("\n")
