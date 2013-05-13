// memory.cpp : Defines the entry point for the console application.
//
// Build as debug to test through main
// Build as release to export functions for python

#include "stdafx.h"

#include "MemoryManager.h"


int _tmain(int argc, _TCHAR* argv[])
{
	MemoryManager *manager = new MemoryManager(100);

	delete manager;
	return 0;
}

