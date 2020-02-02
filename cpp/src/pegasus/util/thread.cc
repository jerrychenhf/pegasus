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

#include "util/thread.h"

#include <set>
#include <map>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "gutil/strings/substitute.h"
#include "common/names.h"

#ifndef NDEBUG
DECLARE_bool(thread_creation_fault_injection);
#endif

namespace this_thread = boost::this_thread;

namespace pegasus {

class ThreadMgr;

// Singleton instance of ThreadMgr. Only visible in this file, used only by Thread.
// The Thread class adds a reference to thread_manager while it is supervising a thread so
// that a race between the end of the process's main thread (and therefore the destruction
// of thread_manager) and the end of a thread that tries to remove itself from the
// manager after the destruction can be avoided.
shared_ptr<ThreadMgr> thread_manager;


// A singleton class that tracks all live threads, and groups them together for easy
// auditing. Used only by Thread.
class ThreadMgr {
 public:
  ThreadMgr() { }

  // Registers a thread to the supplied category. The key is a boost::thread::id, used
  // instead of the system TID since boost::thread::id is always available, unlike
  // gettid() which might fail.
  void AddThread(const thread::id& thread, const string& name, const string& category,
      int64_t tid);

  // Removes a thread from the supplied category. If the thread has
  // already been removed, this is a no-op.
  void RemoveThread(const thread::id& boost_id, const string& category);

 private:
  // Container class for any details we want to capture about a thread
  // TODO: Add start-time.
  // TODO: Track fragment ID.
  class ThreadDescriptor {
   public:
    ThreadDescriptor() { }
    ThreadDescriptor(const string& category, const string& name, int64_t thread_id)
        : name_(name), category_(category), thread_id_(thread_id) {
    }

    const string& name() const { return name_; }
    const string& category() const { return category_; }
    int64_t thread_id() const { return thread_id_; }

   private:
    string name_;
    string category_;
    int64_t thread_id_;
  };

  // A ThreadCategory is a set of threads that are logically related.
  // TODO: unordered_map is incompatible with boost::thread::id, but would be more
  // efficient here.
  struct ThreadCategory {
    int64_t num_threads_created;
    map<const thread::id, ThreadDescriptor> threads_by_id;
  };

  // All thread categorys, keyed on the category name.
  typedef map<string, ThreadCategory> ThreadCategoryMap;

  // Protects thread_categories_
  mutex lock_;

  // All thread categorys that ever contained a thread, even if empty
  ThreadCategoryMap thread_categories_;
};

void ThreadMgr::AddThread(const thread::id& thread, const string& name,
    const string& category, int64_t tid) {
  lock_guard<mutex> l(lock_);
  ThreadCategory& thread_category = thread_categories_[category];
  thread_category.threads_by_id[thread] = ThreadDescriptor(category, name, tid);
  ++thread_category.num_threads_created;
}

void ThreadMgr::RemoveThread(const thread::id& boost_id, const string& category) {
  lock_guard<mutex> l(lock_);
  ThreadCategoryMap::iterator category_it = thread_categories_.find(category);
  DCHECK(category_it != thread_categories_.end());
  category_it->second.threads_by_id.erase(boost_id);
}

Status Thread::StartThread(const std::string& category, const std::string& name,
    const ThreadFunctor& functor, unique_ptr<Thread>* thread,
    bool fault_injection_eligible) {
  DCHECK(thread_manager.get() != nullptr)
      << "Thread created before InitThreading called";
  DCHECK(thread->get() == nullptr);

#ifndef NDEBUG
  if (fault_injection_eligible && FLAGS_thread_creation_fault_injection) {
    // Fail roughly 1% of the time on eligible codepaths.
    if ((rand() % 100) == 1) {
      return Status(Substitute("Fake thread creation failure (category: $0, name: $1)",
          category, name));
    }
  }
#endif

  unique_ptr<Thread> t(new Thread(category, name));
  Promise<int64_t> thread_started;
  try {
    t->thread_.reset(
        new boost::thread(&Thread::SuperviseThread, t->name_, t->category_, functor,
            &thread_started));
  } catch (boost::thread_resource_error& e) {
    return Status::ThreadCreationFailed(name, category, e.what());
  }
  // TODO: This slows down thread creation although not enormously. To make this faster,
  // consider delaying thread_started.Get() until the first call to tid(), but bear in
  // mind that some coordination is required between SuperviseThread() and this to make
  // sure that the thread is still available to have its tid set.
  t->tid_ = thread_started.Get();

  VLOG(2) << "Started thread " << t->tid() << " - " << category << ":" << name;
  *thread = move(t);
  return Status::OK();
}

void Thread::SuperviseThread(const string& name, const string& category,
    Thread::ThreadFunctor functor, Promise<int64_t>* thread_started) {
  int64_t system_tid = syscall(SYS_gettid);
  if (system_tid == -1) {
    string error_msg = "unknown";
    LOG_EVERY_N(INFO, 100) << "Could not determine thread ID: " << error_msg;
  }
  // Make a copy, since we want to refer to these variables after the unsafe point below.
  string category_copy = category.empty() ? "no-category" : category;;
  shared_ptr<ThreadMgr> thread_mgr_ref = thread_manager;
  string name_copy = name.empty() ? Substitute("thread-$0", system_tid) : name;

  // Use boost's get_id rather than the system thread ID as the unique key for this thread
  // since the latter is more prone to being recycled.
  thread_mgr_ref->AddThread(this_thread::get_id(), name_copy, category_copy, system_tid);

  thread_started->Set(system_tid);

  // Any reference to any parameter not copied in by value may no longer be valid after
  // this point, since the caller that is waiting on *tid != 0 may wake, take the lock and
  // destroy the enclosing Thread object.

  functor();
  thread_mgr_ref->RemoveThread(this_thread::get_id(), category_copy);
}

void ThreadGroup::AddThread(unique_ptr<Thread>&& thread) {
  threads_.emplace_back(move(thread));
}

void ThreadGroup::JoinAll() {
  for (auto& thread : threads_) thread->Join();
}

int ThreadGroup::Size() const {
  return threads_.size();
}

void InitThreading() {
  DCHECK(thread_manager.get() == nullptr);
  thread_manager.reset(new ThreadMgr());
}


}
