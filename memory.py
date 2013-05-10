#-------------------------------------------------------------------------------
# Name:        module1
# Purpose:
#
# Author:      Michael
#
# Created:     08/05/2013
# Copyright:   (c) Michael 2013
# Licence:     <your licence>
#-------------------------------------------------------------------------------
#!/usr/bin/env python

import ctypes
from ctypes import *

FIRST_FIT = 0
NEXT_FIT = 1
BEST_FIT = 2
WORST_FIT = 3

dll = ctypes.CDLL("Release/memory.dll")


class MemoryManager(object):
    def __init__(self, mem_size):
        self.obj = dll.memory_manager_new(mem_size)

    def request(self, size, strategy):
        return dll.memory_manager_request(self.obj, size, strategy)

    def release(self, location):
        dll.memory_manager_release(self.obj, location)

    def toString(self):
        return c_char_p(dll.memory_manager_to_string(self.obj))

class Simulator(object):
    def __init__(self, size, a, d, strategy, sim_step):
        self.manager = MemoryManager(size)
        self.a = a
        self.d = d
        self.strategy = strategy
        self.sim_step = sim_step

def main():
    manager = MemoryManager(100) #initialize 100 word memory
    pointers = [manager.request(16*4, FIRST_FIT) for x in range(5)]
    manager.release(pointers[1])
    manager.release(pointers[0])
    manager.release(pointers[3])
    manager.request(16*4, BEST_FIT)
    print("Hole addresses (in order):")
    print(repr(pointers))
    print("Raw memory:")
    print(manager.toString().value)
    pass

if __name__ == '__main__':
    main()
