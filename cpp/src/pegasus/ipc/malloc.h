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

#ifndef PEGASUS_MALLOC_H
#define PEGASUS_MALLOC_H

#include <inttypes.h>
#include <stddef.h>

#include <unordered_map>

namespace pegasus {

/// Gap between two consecutive mmap regions allocated by fake_mmap.
/// This ensures that the segments of memory returned by
/// fake_mmap are never contiguous and dlmalloc does not coalesce it
/// (in the client we cannot guarantee that these mmaps are contiguous).
constexpr int64_t kMmapRegionsGap = sizeof(size_t);

void GetMallocMapinfo(void* addr, int* fd, int64_t* map_length, ptrdiff_t* offset);

/// Get the mmap size corresponding to a specific file descriptor.
///
/// @param fd The file descriptor to look up.
/// @return The size of the corresponding memory-mapped file.
int64_t GetMmapSize(int fd);

/// Add the mmap record providing the pointer, file descrptor and mapped size.
/// @param pointer The mapped pointer
/// @param fd The file descriptor to look up.
/// @param The size of the corresponding memory-mapped file.
void AddMmapRecord(void* pointer, int fd, int64_t size);

/// Check the providing the pointer is mapped with the specific size and return the file descriptor
/// @param pointer The mapped pointer
/// @param The size of the corresponding memory-mapped file.
int GetMappedFd(void* pointer, int64_t size) ;

/// Remove the mmap record
/// @param pointer The mapped pointer
void RemoveMmapRecord(void* pointer );

struct MmapRecord {
  int fd;
  int64_t size;
};

/// Hashtable that contains one entry per segment that we got from the OS
/// via mmap. Associates the address of that segment with its file descriptor
/// and size.
extern std::unordered_map<void*, MmapRecord> mmap_records;

}  // namespace pegasus

#endif  // PEGASUS_MALLOC_H
