// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "LocalMemoryMapping.h"

#include <mutex>
#include <unordered_map>
#include <sys/mman.h>
#include <unistd.h>
#include <memory>
#include <cassert>

// From Google gutil
#ifndef PEGASUS_DISALLOW_COPY_AND_ASSIGN
#define PEGASUS_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;            \
  void operator=(const TypeName&) = delete
#endif

namespace pegasus {

/// Gap between two consecutive mmap regions allocated by fake_mmap.
/// This ensures that the segments of memory returned by
/// fake_mmap are never contiguous and dlmalloc does not coalesce it
/// (in the client we cannot guarantee that these mmaps are contiguous).
constexpr int64_t MMAP_REGIONS_GAP = sizeof(size_t);

class MmapTableEntry {
 public:
  MmapTableEntry(int fd, int64_t map_size)
      : fd_(fd), pointer_(nullptr), length_(0) {
    // We subtract MMAP_REGIONS_GAP from the length that was added
    // in fake_mmap in malloc.h, to make map_size page-aligned again.
    length_ = map_size - MMAP_REGIONS_GAP;
    pointer_ = reinterpret_cast<uint8_t*>(
        mmap(NULL, length_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    // TODO(pcm): Don't fail here, instead return a Status.
    if (pointer_ == MAP_FAILED) {
      // TODO: log something herre
    }
    close(fd);  // Closing this fd has an effect on performance.
  }

  ~MmapTableEntry() {
    // We need to make sure that when the destructor is called, the mapped memory pointer
    // is no longer in use. 
    // We don't need to close the associated file, since it has
    // already been closed in the constructor.
    int r = munmap(pointer_, length_);
    if (r != 0) {
      // TODO: log some error here "munmap returned " << r << ", errno = " << errno;
    }
  }

  uint8_t* pointer() { return pointer_; }

  int fd() { return fd_; }

 private:
  /// The associated file descriptor on the client.
  int fd_;
  /// The result of mmap for this file descriptor.
  uint8_t* pointer_;
  /// The length of the memory-mapped file.
  size_t length_;

  PEGASUS_DISALLOW_COPY_AND_ASSIGN(MmapTableEntry);
};

//global table to track map of file descriptors
std::unordered_map<int, std::unique_ptr<MmapTableEntry>> mmap_table_;
  
// A mutex which protects this class.
 std::mutex mutex_;

// If the file descriptor fd has been mmapped in this process before,
// return the pointer that was returned by mmap, otherwise mmap it and store the
// pointer in a hash table.
uint8_t* LookupOrMmap(int fd, int64_t map_size) {
  std::lock_guard<std::mutex> guard(mutex_);
  auto entry = mmap_table_.find(fd);
  if (entry != mmap_table_.end()) {
    return entry->second->pointer();
  } else {
    mmap_table_[fd] =
        std::unique_ptr<MmapTableEntry>(new MmapTableEntry(fd, map_size));
    return mmap_table_[fd]->pointer();
  }
}

// Get a pointer to a file that we know has been memory mapped in this client
// process before.
uint8_t* LookupMmappedFile(int fd) {
  std::lock_guard<std::mutex> guard(mutex_);
  auto entry = mmap_table_.find(fd);
  assert(entry != mmap_table_.end());
  return entry->second->pointer();
}

//To cleanup all the mmap entries, make sure no shared memory pointers are in using
void CleanupMmap() {
   std::lock_guard<std::mutex> guard(mutex_);
   mmap_table_.clear();
}

}