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
	this->cost_ = 0;

	// Allocate a free hole memSize - 4
	writeHole(this->memory_, this->memory_, memSize - METADATA_SIZE, HOLE_FREE, this->memory_);
	this->head_ = this->memory_;
	this->last_ = this->head_;

	// The following is for testing:
#ifdef _DEBUG
	int *holes[5];
	holes[0] = this->request(80, NEXT_FIT);
	holes[1] = this->request(80, NEXT_FIT);
	holes[2] = this->request(160, NEXT_FIT);
	this->release(holes[0]);
	holes[0] = this->request(80, NEXT_FIT);
	this->release(holes[1]);
	this->release(holes[0]);
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
	int cost = 1; // Keep track of the number of holes examined.
	switch (strategy) 
	{
	case NEXT_FIT:
		{
			// Next-fit algorithm starts off at the last place we looked
			// We need to cycle through until we reach our original, if we didn't find a suitable hole
			current = this->last_;
			int *stop_here = NULL;
			while (hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
				cost++;
				if (stop_here == NULL) {
					stop_here = current;
				} else if (stop_here == current) {
					break;
				}

				int *successor = hole_get_successor(current);
				if (successor == current) {
					if (this->head_ == successor) {
						break;
					} else {
						current = this->head_;
					}
				} else {
					current = successor;
				}
			}

			if (hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
				return NULL;
			} else {
				location = current;
			}
		}
		break;
	case FIRST_FIT:
		while (hole_get_tag(current) == HOLE_ALLOCATED || hole_get_size(current) < words) {
			cost++;
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
			// We're looping around, so keep a pointer to the first hole
			int *min_hole_location = NULL;
			int min_hole_size = INT_MAX;

			int *last = NULL;
			while (current != last) {
				cost++;
				if (hole_get_tag(current) == HOLE_FREE && hole_get_size(current) >= words) {
					int currentSize = hole_get_size(current);
					if (currentSize < min_hole_size) {
						min_hole_location = current;
						min_hole_size = currentSize;
					}
				}

				last = current;
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
		// Worst-fit allocates the largest possible free hole
		// We need to do a linear search across all holes and keep track of the largest one
		// We're looping around, so keep a pointer to the first hole
		int *max_hole_location = NULL;
		int max_hole_size = INT_MIN;
		int *last = NULL;
		while (current != last) {
			cost++;
			if (hole_get_tag(current) == HOLE_FREE && hole_get_size(current) >= words) {
				int currentSize = hole_get_size(current);
				if (currentSize > max_hole_size) {
					max_hole_location = current;
					max_hole_size = currentSize;
				}
			}
			last = current;
			current = hole_get_successor(current);
		}
		if (!max_hole_location || hole_get_tag(max_hole_location) == HOLE_ALLOCATED) {
			// No space available!
			return NULL;
		} else {
			location = max_hole_location;
		}
		break;
	}


	// We've chosen the location
	// Now, if the there's enough room left over past the end after we allocate the hole, we must
	// write a free hole to that 
	int *predecessor = hole_get_predecessor(location);
	int *successor = hole_get_successor(location);
	int allocated_hole_size = words + METADATA_SIZE;

	// Calculate space after
	int space;
	if (successor != location) {
		// In the case there's an actual hole after us
		space = successor - (location + allocated_hole_size);
	} else {
		space = (this->memory_ + this->size_) - (location + allocated_hole_size);
	}

	this->destroyHole(location);
	if (space < 4) {
		this->writeHole(location, predecessor, words, HOLE_ALLOCATED, successor);
	} else {
		int *free_hole = location + METADATA_SIZE + words;
		this->writeHole(location, predecessor, words, HOLE_ALLOCATED, free_hole);

		// If the successor is before the free_hole, then that means there's nothing after us
		if (successor < free_hole) {
			successor = free_hole;
		}
		this->writeHole(free_hole, location, space - METADATA_SIZE, HOLE_FREE, successor);

		// We're adding a new hole, so the free hole's successor is still pointing to the allocated hole--fix that
		successor = hole_get_successor(free_hole);
		if (successor != free_hole) {
			hole_set_predecessor(successor, free_hole);
		}
	}

	// For use with next-fit algorithm
	this->last_ = hole_get_successor(location);

	this->cost_ = cost;

	return location;
}

void MemoryManager::release( int *hole )
{
	hole_set_tag(hole, HOLE_FREE);

	// Check if the holes next to us are free. If so, join those babies.
	int *predecessor = hole_get_predecessor(hole);
	int *successor = hole_get_successor(hole);

	if ((predecessor != hole) && hole_get_tag(predecessor) == HOLE_FREE) {
		// There's a possibility that we will become our own successor
		if (successor == hole) {
			successor = predecessor;
		}

		// Overwrite both holes with one new one:
		// We're joining two holes together, so we only need 1 metadata now instead of 2
		this->writeHole(predecessor, hole_get_predecessor(predecessor), hole_get_size(predecessor) + hole_get_size(hole) + METADATA_SIZE, HOLE_FREE, successor);
		if (this->last_ == hole) {
			this->last_ = predecessor;
		}
		hole = predecessor;

		// Since we merge with the predecessor, the successor's predecessor must also point to the new location
		successor = hole_get_successor(hole);
		if (successor != hole) {
			hole_set_predecessor(successor, hole);
		}

		// Since our offset moved, fix the successor tag of the hole before us
		predecessor = hole_get_predecessor(hole);
		if (predecessor != hole) {
			hole_set_successor(predecessor, hole);
		}
	}

	if ((successor != hole) && hole_get_tag(successor) == HOLE_FREE) {
		// Overwrite both holes with one new one:
		if (successor == hole_get_successor(successor)) {
			successor = hole;
		} else {
			successor = hole_get_successor(successor);
		}

		this->writeHole(hole, predecessor, hole_get_size(hole) + hole_get_size(hole_get_successor(hole)) + METADATA_SIZE, HOLE_FREE, successor);
		if (this->last_ == successor) {
			this->last_ = hole;
		}

		// Since there's one less hole, fix the predecessor tag of the hole after us
		if (successor != hole) {
			hole_set_predecessor(successor, hole);
		}
	}

	// There's a case where we may have a free memory less than 4 words long (it's not recognized as a hole since
	// there's not enough room for metadata. Find that and join it also. This should can be either be for:
	// 1) If the hole is at the very end of memory
	// 2) The hole can also be preceded by free memory less than 4 words long

	// See if there's any space in front of us
	predecessor = hole_get_predecessor(hole);
	if (predecessor != hole) {
		int space_before = hole - (predecessor + METADATA_SIZE + hole_get_size(predecessor));
		if (space_before > 0 && space_before < 4) {
			// Are we our own successor, if so: we want it to point to the new location
			successor = hole_get_successor(hole);
			if (successor == hole) {
				successor = hole - space_before;
			} else {
				// if not, then we should change the successor's predecessor tag to the new location
				hole_set_predecessor(successor, hole - space_before);
			}

			// Join it
			this->writeHole(hole - space_before, predecessor, hole_get_size(hole) + space_before, HOLE_FREE, successor);
			if (this->last_ == hole) {
				this->last_ = hole - space_before;
			}

			hole = hole - space_before;

			// Since our offset moved, we need to fix the successor tag of the hole before us
			hole_set_successor(predecessor, hole);

		}
	}

	// See if there's any space at end
	successor = hole_get_successor(hole);
	if (successor == hole) {
		int space_after = (this->memory_ + this->size_) - (hole + hole_get_size(hole) + 4); 
		if (space_after > 0 && space_after < 4) {
			// Join it
			this->writeHole(hole, hole_get_predecessor(hole), hole_get_size(hole) + space_after, HOLE_FREE, successor);
		}
	}
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
		int memory = *(this->memory_ + i);
		memory = (memory == 0xDEADBEEF) ? -1 : memory;
		sprintf_s(number, 256, "%d", memory);
		sprintf_s(buffer + strlen(buffer), strlen(number) + 4, "%s, ", number);
	}

	*(buffer + strlen(buffer) - 2) = '\0';

	return buffer;
}

int MemoryManager::getUtilization()
{
	// Do a linear search counting all block sizes
	int utilization = 0;
	int *previous = NULL;
	int *current = this->head_;
	do {
		if (hole_get_tag(current) == HOLE_ALLOCATED) {
			utilization += hole_get_size(current) + METADATA_SIZE;
		}
		previous = current;
		current = hole_get_successor(current);
	} while (current != previous);

	return utilization;
}

float MemoryManager::getUtilizationFraction()
{
	return static_cast<float> (getUtilization()) / this->getSize();
}

int MemoryManager::getLastRequestCost()
{
	return this->cost_;
}

int MemoryManager::getSize()
{
	return this->size_;
}

extern "C" 
{
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

	__declspec(dllexport) int memory_manager_get_utilization(MemoryManager *manager)
	{
		return manager->getUtilization();
	}

	/*
	__declspec(dllexport) float memory_manager_get_utilization_fraction(MemoryManager *manager)
	{
		return manager->getUtilizationFraction();
	}
	*/
	__declspec(dllexport) int memory_manager_get_last_request_cost(MemoryManager *manager)
	{
		return manager->getLastRequestCost();
	}

	__declspec(dllexport) int memory_manager_get_size(MemoryManager *manager)
	{
		return manager->getSize();
	}
};