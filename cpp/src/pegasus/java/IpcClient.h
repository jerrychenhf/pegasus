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

#ifndef PEGASUS_IPC_CLIENT_H
#define PEGASUS_IPC_CLIENT_H

#include <inttypes.h>
#include <stddef.h>
#include <mutex>
#include <unistd.h>
#include <unordered_map>
#include <sys/mman.h>
#include <memory>
#include <vector>

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

class IpcClient {
public:
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
public:  
  IpcClient();
  ~IpcClient();

  bool Connect(const char* s_name);
  bool Disconnect();
  
  bool MapFileDescriptors(int* mmap_fds, int count);
  
  uint8_t* LookupOrMmap(int fd, int64_t map_size);
  uint8_t* LookupMmappedFile(int fd);

  void CleanupMmap();
private:
  bool RequestServerFds(const std::vector<int>& server_fds);
  bool SendFdRequest(const std::vector<int>& server_fds);
  bool ReceiveFdResponse(std::vector<int>& local_fds);
    
private:
  // the ipc socket
  int socket_;
  
  //table to track server fd to process local fd
  std::unordered_map<int, int> fd_table_;
    
  //table to track map of file descriptors
  std::unordered_map<int, std::unique_ptr<MmapTableEntry>> mmap_table_;
};

}  // namespace pegasus

#endif  // PEGASUS_IPC_CLIENT_H
