#pragma once

#include <string>

#define HOLE_ALLOCATED		1
#define HOLE_FREE			0

#define METADATA_SIZE		4

class MemoryManager
{
public:
	enum Strategy 
	{
		FIRST_FIT = 0,
		NEXT_FIT,
		BEST_FIT,
		WORST_FIT
	};

	MemoryManager(int memSize);
	~MemoryManager(void);

	// Size is in bytes
	int *request(unsigned int size, Strategy strategy = FIRST_FIT);
	void release(int *hole);

	int getSize();

	int getUtilization();
	float getUtilizationFraction();

	inline int getLastRequestCost();

	char *toString();

private:
	
	// This does not error check! You may overwrite existing holes.
	void writeHole(int *location, int *predecessor, int size, int tag, int *successor);
	void destroyHole(int *location);

	int *head_;		// Refers to the first hole (first predecessor index, which should be 0)
	int *tail_;

	int *last_;		// For use with Next-Fit

	int *memory_;	// In all its glory.
	int size_;
	int cost_;
};