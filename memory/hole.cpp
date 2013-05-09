#include "stdafx.h"

#include "Hole.h"
#include "MemoryManager.h"

int *hole_get_predecessor(int *start)
{
	return (int *) *start;
}

int hole_get_size(int *start)
{
	return *(start + 1);
}

int hole_get_tag(int *start)
{
	return *(start + hole_get_size(start) + 2);
}

int *hole_get_successor(int *start)
{
	return (int *) *(start + hole_get_size(start) + 3);
}

void hole_set_tag(int *start, int tag)
{
	*(start + hole_get_size(start) + 2) = tag;
}