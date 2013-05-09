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
	if (memSize <= 4) {
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
#ifdef _DEBUG
	this->request(7*4);
#endif
}


MemoryManager::~MemoryManager(void)
{
	free(this->memory_);
}

int *MemoryManager::request( unsigned int size, Strategy strategy /*= FIRST_FIT*/ )
{
	int words = (size + (size % 4))/4; // Ceil to the nearest factor of 4 and then convert to words

	int *location = (int*)0xDEADBEEF;
	int *current = this->head_;
	switch (strategy) 
	{
	case NEXT_FIT:
		// Next-fit algorithm starts off at the last place we looked
		// We need to cycle through until we reach our original, if we didn't find a suitable hole
		current = this->last_;
		while (hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
			int *successor = hole_get_successor(current);
			if (successor == current) {
				// We reached the end! Start at beginning
				current = this->head_;
				while (current != this->last_ && hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
					current = hole_get_successor(current);
				}
			} else {
				current = successor;
			}
		}
		if (hole_get_tag(current) == HOLE_ALLOCATED || current == this->last_) {
			// No space available!
			return NULL;
		} else {
			location = current;
		}
		break;
	case FIRST_FIT:
		while (hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
			int *successor = hole_get_successor(current);
			if (successor == current) {
				// We reached the end!
				return NULL;
			}

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
		{
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

	// Write the two new holes: 1 allocated, 1 free (if possible)
	int *allocated_hole = location;

	// Make sure we actually have space for a free hole;
	int *free_hole = location + METADATA_SIZE + words;
	if (free_hole > this->memory_ + this->size_ - METADATA_SIZE) {
		free_hole = allocated_hole;
	}

	this->writeHole(allocated_hole, predecessor, words, HOLE_ALLOCATED, free_hole);

	if (free_hole != allocated_hole) {
		// If the original hole's successor is before this free hole, then it was referring to itself.
		if (successor < free_hole) {
			successor = free_hole;
		}
		this->writeHole(free_hole, allocated_hole, original_size - words - METADATA_SIZE, HOLE_FREE, successor);
	}

	// For use with next-fit algorithm
	this->last_ = allocated_hole;

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

char *MemoryManager::toString()
{
	char *buffer = (char *) calloc(sizeof(int)*3, this->size_);
	for (int i = 0; i < this->size_; i++) {
		char number[256];
		sprintf_s(number, 256, "%d", *(this->memory_ + i));
		sprintf_s(buffer + strlen(buffer), strlen(number) + 4, "%s, ", number);
	}

	*(buffer + strlen(buffer) - 2) = '\0';

	return buffer;
}

extern "C" {
	__declspec(dllexport) MemoryManager *memory_manager_new(int memSize) 
	{ 
		return new MemoryManager(memSize); 
	}

	__declspec(dllexport) int *memory_manager_request(MemoryManager *manager, unsigned int size, MemoryManager::Strategy strategy = MemoryManager::Strategy::FIRST_FIT) 
	{ 
		return manager->request(size, strategy); 
	}

	__declspec(dllexport) void memory_manager_release(MemoryManager *manager, int *hole)
	{
		manager->release(hole);
	}

	__declspec(dllexport) char *memory_manager_to_string(MemoryManager *manager)
	{
		return manager->toString();
	}
};