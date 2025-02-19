#include "memsim.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iterator>
#include <list>
#include <set>
#include <unordered_map>

struct Partion {
  long tag;
  long size;
  int64_t addr;
  Partion(long tag, long size, int64_t addr)
  {
    this->tag = tag;
    this->size = size;
    this->addr = addr;
  }
};

typedef std::list<Partion>::iterator PartitionRef;

struct scmp {
  bool operator()(const PartitionRef & c1, const PartitionRef & c2) const
  {
    if (c1->size == c2->size) return c1->addr < c2->addr;
    else
      return c1->size < c2->size;
  }
};

struct Simulator {
  Simulator(int64_t page_size) { page_size_ = page_size; }
  void allocate(long tag, long size)
  {
    std::list<Partion> dummy { Partion(-1, size, 0) };
    auto sbesti = free_blocks.lower_bound(dummy.begin());
    auto smallest = part_ll.end();
    if (sbesti != free_blocks.end()) smallest = *sbesti;

    // If the best fit was found
    if (smallest != part_ll.end()) {
      // Make sure to erase the best fit it from free blocks as it will be either no longer used or
      // modified
      free_blocks.erase(smallest);

      // If we found a perfect fit just change the tag and add it to tagged_blocks
      if (smallest->size == size) {
        smallest->tag = tag;
        tagged_blocks[tag].push_back(smallest);
      }
      // Otherwise split smallest into 2
      else {
        Partion p(tag, size, smallest->addr);
        smallest->size = smallest->size - size;
        // smallest->tag = -1;
        smallest->addr += p.size;

        PartitionRef pRef = part_ll.insert(smallest, p);
        free_blocks.insert(smallest);
        tagged_blocks[tag].push_back(pRef);
      }

    }
    // If there wasn't space for the alloc req
    else {
      // Set a new it to the last block
      PartitionRef it = part_ll.end();
      --it;

      // Check to see if the last block is free we can take advantage of its mem
      if (it->tag == -1) {
        free_blocks.erase(it);
        int64_t page_request;

        long diff = size - it->size;
        page_request = diff / page_size_;
        if (diff % page_size_ > 0) page_request++;
        pagesRequested += page_request;

        Partion p(tag, size, it->addr);
        PartitionRef pRef = part_ll.insert(it, p);
        tagged_blocks[tag].push_back(pRef);

        // If I wasn't able to perfect use the free block I adjust its size
        if ((page_request * page_size_) - diff > 0) {
          it->size = (page_request * page_size_) - diff;
          it->addr = pRef->addr + pRef->size;
          free_blocks.insert(it);
        } else
          part_ll.erase(it);
      }
      // Otherwise if the last block wasn't occupied then i want to create more memory
      else {
        Partion p(tag, size, it->addr + it->size);

        int64_t page_request;
        page_request = size / page_size_;
        if (size % page_size_ > 0) page_request++;
        pagesRequested += page_request;

        PartitionRef pRef = part_ll.insert(part_ll.end(), p);
        tagged_blocks[tag].push_back(pRef);

        // If I wasn't able to create a perfect block i gotta store the extra free mem
        if ((page_request * page_size_) - size > 0) {
          Partion notOccupied(-1, ((page_request * page_size_) - size), pRef->addr + pRef->size);
          PartitionRef emptyRef = part_ll.insert(part_ll.end(), notOccupied);
          free_blocks.insert(emptyRef);
        }
      }
    }
    return;
  }

  void deallocate(int tag)
  {
    // Loop through all the "tags", that we want to dealloc and check their adjacent neighbours and
    // see if they can be merged
    for (auto & p : tagged_blocks[tag]) {
      p->tag = -1;
      if (p != part_ll.begin() && std::prev(p)->tag == -1) {
        p->size += std::prev(p)->size;
        p->addr = std::prev(p)->addr;

        free_blocks.erase(std::prev(p));
        part_ll.erase(std::prev(p));
      }
      if (std::next(p) != part_ll.end() and std::next(p)->tag == -1) {
        p->size += std::next(p)->size;

        free_blocks.erase(std::next(p));
        part_ll.erase(std::next(p));
      }

      free_blocks.insert(p);
    }
    tagged_blocks[tag].clear();
  }

  void getStats(MemSimResult & result)
  {

    result.n_pages_requested = pagesRequested;
    for (auto free : free_blocks)
      if (free->size > result.max_free_partition_size) result.max_free_partition_size = free->size;
  }

  int64_t pagesRequested = 0;
  int64_t page_size_;
  std::list<Partion> part_ll;
  // Only stores blocks whose tags are -1
  std::set<PartitionRef, scmp> free_blocks;
  // Tagged_blocks ONLY stores blocks with value greater than or equal to 0
  std::unordered_map<long, std::vector<PartitionRef>> tagged_blocks;
};
void mem_sim(int64_t page_size, const std::vector<Request> & requests, MemSimResult & result)
{
  result.n_pages_requested = 0;
  result.max_free_partition_size = 0;
  Simulator sim(page_size);
  // Start by creating an empty free block with size 0
  Partion empty(-1, 0, 0);
  sim.part_ll.push_back(empty);
  PartitionRef firstNode = sim.part_ll.end();
  --firstNode;
  sim.free_blocks.insert(firstNode);

  for (const auto & req : requests) {
    if (req.tag < 0) {
      sim.deallocate(-req.tag);
    } else {
      sim.allocate(req.tag, req.size);
    }
  }

  sim.getStats(result);
}