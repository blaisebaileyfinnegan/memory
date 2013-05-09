#pragma once

#include "MemoryManager.h"

int *hole_get_predecessor(int *start);
int hole_get_size(int *start);
int hole_get_tag(int *start);
int *hole_get_successor(int *start);