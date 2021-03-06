#-------------------------------------------------------------------------------
# Name:        Driver/Simulator for the Memory Manager
# Purpose:
#
# Author:      Michael
#
# Created:     08/05/2013
# Copyright:   (c) Michael 2013
# Licence:     <your licence>
#-------------------------------------------------------------------------------
#!/usr/bin/env python

from __future__ import division

import ctypes
from ctypes import *

import sys
import random
import math

FIRST_FIT = 0
NEXT_FIT = 1
BEST_FIT = 2
WORST_FIT = 3

METADATA_SIZE = 4

dll = ctypes.CDLL("Release/memory.dll")


class MemoryManager(object):
    def __init__(self, mem_size):
        self.obj = dll.memory_manager_new(mem_size)

    def request(self, size, strategy):
        return dll.memory_manager_request(self.obj, size, strategy)

    def release(self, location):
        dll.memory_manager_release(self.obj, location)

    def to_string(self):
        return c_char_p(dll.memory_manager_to_string(self.obj))

    def get_utilization(self):
        return dll.memory_manager_get_utilization(self.obj)

    def get_utilization_fraction(self):
        return self.get_utilization()/self.get_size()

    def get_last_request_cost(self):
        return dll.memory_manager_get_last_request_cost(self.obj)

    def get_size(self):
        return dll.memory_manager_get_size(self.obj)

class Results(object):
    def __init__(self, utilization_fractions, cost, requests):
        self.utilization_fractions = utilization_fractions
        self.cost = cost
        self.requests = requests

class Simulator(object):
    def __init__(self, size, a, d, strategy, sim_step):
        self.manager = MemoryManager(size)
        self.a = a
        self.d = d
        self.strategy = strategy
        self.sim_step = sim_step
        self.blocks = []

    def next_request_size(self):
        max_size = self.manager.get_size() - METADATA_SIZE
        candidate = int(math.floor(random.gauss(self.a, self.d)))
        while (candidate >= max_size * 4) or (candidate <= 0):
            candidate = int(math.floor(random.gauss(self.a, self.d)))
        return candidate

    def get_random_block(self):
        return self.blocks.pop(random.randint(0, len(self.blocks) - 1))

    def drive(self):
        utilization_fractions = []
        cost = 0
        requests = 0

        for i in range(0, self.sim_step):
            print(i)
            size = self.next_request_size()
            print("Request: " + repr(size))
            location = self.manager.request(size, self.strategy)

            cost = cost + self.manager.get_last_request_cost()
            requests = requests + 1

            while location:
                self.blocks.append(location)

                size = self.next_request_size()
                print("Request: " + repr(size))
                location = self.manager.request(size, self.strategy)

                cost = cost + self.manager.get_last_request_cost()
                requests = requests + 1

            utilization_fractions.append(self.manager.get_utilization_fraction())
            block = self.get_random_block()
            self.manager.release(block)

        results = Results(utilization_fractions, cost, requests)
        return results

def test():
    manager = MemoryManager(100) #initialize 100 word memory
    pointers = [manager.request(16*4, FIRST_FIT) for x in range(5)]
    manager.release(pointers[1])
    manager.release(pointers[0])
    manager.release(pointers[3])
    manager.request(16*4, FIRST_FIT)
    manager.request(16*4, FIRST_FIT)
    manager.request(16*4, FIRST_FIT)
    print("Hole addresses (in order):")
    print(repr(pointers))
    print("Raw memory:")
    print(manager.to_string().value)
    print("Size:")
    print(manager.get_size())
    print("Last Request Cost:")
    print(manager.get_last_request_cost())
    print("Utilization:")
    print(manager.get_utilization())
    print("Utilization fraction:")
    print(manager.get_utilization_fraction())
    pass

def main():
    #test()

    sim = None
    output = None
    arg_count = len(sys.argv)
    # Expects:
    #   mem size (in words),
    #   average request size (in bytes),
    #   standard deviatoin (bytes),
    #   strategy
    #   sim_step
    if (arg_count >= 6):
        sim = Simulator(int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]))
    else:
        memory_size = raw_input("Specify memory size (in words):")
        average_request_size = raw_input("Specify average request size (in bytes):")
        standard_deviation = raw_input("Specify standard deviation (in bytes):")
        strategy = raw_input("Specify strategy (int):")
        sim_step = raw_input("Specify sim_step:")

        sim = Simulator(memory_size, average_request_size, standard_deviation, strategy, sim_step)
    results = sim.drive()
    print("Results")
    print("-------")
    print("Total Cost (how many holes searched): " + repr(results.cost))
    print("Total Number of Requests: " + repr(results.requests))
    average_utilization = sum(results.utilization_fractions) / len(results.utilization_fractions)
    print("Average Memory Utilization Fraction: " + repr(average_utilization))
    average_search_time = results.cost/results.requests
    print("Average Search Time (Cost divided by Requests): " + repr(average_search_time))
    pass

if __name__ == '__main__':
    main()
