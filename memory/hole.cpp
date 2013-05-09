#pragma once

#include "Hole.h"
#include "MemoryManager.h"

int *hole_get_predecessor(int *start)
{
	return start;
}

int hole_get_size(int *start)
{
	return *(start + 1);
}

int hole_get_tag(int *start)
{
	return *(start + hole_get_size(start) + 1);
}

int *hole_get_successor(int *start)
{
	return (start + hole_get_size(start) + 2);
}