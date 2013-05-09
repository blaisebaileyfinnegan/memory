// memory.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "MemoryManager.h"


int _tmain(int argc, _TCHAR* argv[])
{
	MemoryManager *manager = new MemoryManager(10);

	delete manager;
	return 0;
}

