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

dll = ctypes.CDLL("release/memory.dll")


class MemoryManager(object):
    def __init__(self, mem_size):
        self.obj = dll.memory_manager_new(mem_size)

    def request(self, size, strategy):
        return dll.memory_manager_request(self.obj, size, strategy)

    def release(self, location):
        dll.memory_manager_release(self.obj, location)

    def toString(self):
        return c_char_p(dll.memory_manager_to_string(self.obj))

def main():
    manager = MemoryManager(15)
    manager.request(11*4, FIRST_FIT)
    print repr(manager.toString())
    pass

if __name__ == '__main__':
    main()
