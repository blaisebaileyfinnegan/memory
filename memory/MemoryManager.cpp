/*
 * Author: Michael Vu  
 * Date: 5/7/13
 * Ucinetid: 10025186
 *
 * I have a feeling this is way more work than the class actually demands out of this project. It
 * may also be considered unnecessarily overcomplicated, since it can easily be done using
 * a simple array and performing array index addition/subtraction rather than having to deal
 * with pointers and all the de-referencing crap. 
 *
 *
 */

#include "stdafx.h"
#include <stdlib.h>
#include "MemoryManager.h"
#include "Hole.h"

MemoryManager::MemoryManager( int memSize )
{
	if (memSize < 4) {
		abort();
	}

	// memSize is number of words we want. malloc() takes bytes as input
	this->memory_ = (int*) malloc(memSize * 4);
	this->size_ = memSize;

	// Allocate a free hole memSize - 4
	writeHole(this->memory_, this->memory_, memSize - METADATA_SIZE, HOLE_FREE, this->memory_);
	this->head_ = this->memory_;
	this->last_ = this->head_;

	// The following is for testing:
}


MemoryManager::~MemoryManager(void)
{

}

int *MemoryManager::request( unsigned int size, Strategy strategy /*= FIRST_FIT*/ )
{
	int words = (size + (size % 4))/4; // Round to the nearest factor of 4 and then convert to words

	int *location = (int*)0xDEADBEEF;
	int *current = this->head_;
	switch (strategy) 
	{
	case NEXT_FIT:
		// Next-fit algorithm starts off at the last place we looked
		current = this->last_;
	case FIRST_FIT:
		while (hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
			current = hole_get_successor(current);
		}
		if (hole_get_tag(current) == HOLE_ALLOCATED) {
			// No space available!
			return NULL;
		} else {
			location = current;
		}
		break;
	case BEST_FIT:
		// Best-fit allocates the smallest possible free hole
		// We need to do a linear search across all holes and keep track of the smallest one
		int *min_hole_location = NULL;
		int min_hole_size = INT_MAX;
		while (hole_get_tag(current) == HOLE_FREE && hole_get_size(current) >= words) {
			int currentSize = hole_get_size(current);
			if (currentSize < min_hole_size) {
				min_hole_location = current;
				min_hole_size = currentSize;
			}

			current = hole_get_successor(current);
		}
		if (!min_hole_location || hole_get_tag(min_hole_location) == HOLE_ALLOCATED) {
			// No space available!
			return NULL;
		} else {
			location = min_hole_location;
		}

		break;
	case WORST_FIT:
		break;
	}


	// We've chosen the location, now split that hole up into a free and an allocated hole
	// First step: save the predecessor and successor
	int *predecessor = hole_get_predecessor(location);
	int *successor = hole_get_successor(location);
	int original_size = hole_get_size(location);

	// Now destroy it
	this->destroyHole(location);

	// Write the two new holes: 1 allocated, 1 free
	int *allocated_hole = location;
	int *free_hole = location + METADATA_SIZE + words;
	this->writeHole(allocated_hole, predecessor, words, HOLE_ALLOCATED, free_hole);

	// If the original hole's successor is before this free hole, then it was referring to itself.
	if (successor < free_hole) {
		successor = free_hole;
	}
	this->writeHole(free_hole, allocated_hole, original_size - words - METADATA_SIZE, HOLE_FREE, successor);

	// For use with next-fit algorithm
	this->last_ = free_hole;

	return allocated_hole;
}

void MemoryManager::release( int *hole )
{
	hole_set_tag(hole, HOLE_FREE);
}

void MemoryManager::writeHole( int *location, int *predecessor, int size, int tag, int *successor )
{
	// Our predecessor and successor data don't point to the actual addresses unless we do this cast
	// This converts the address to a value, rather than having hole point to the actual value that predecessor
	// also points to
	*location = (int) predecessor; 
	*(location + 1) = size;

	for (int i = 0; i < size; i++)
	{
		// Initialize our data
		*(location + 2 + i) = 0xDEADBEEF;
	}

	*(location + size + 2) = tag;

	int *last = location + size + 3;
	*last = (int) successor;
}

void MemoryManager::destroyHole(int *location)
{
	int size = hole_get_size(location);
	for (int i = 0; i < size + METADATA_SIZE; i++)
	{
		*(location + i) = 0xDEADBEEF;
	}
}