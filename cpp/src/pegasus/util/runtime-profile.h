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


#ifndef PEGASUS_UTIL_RUNTIME_PROFILE_H
#define PEGASUS_UTIL_RUNTIME_PROFILE_H

#include <boost/function.hpp>
#include <boost/thread/lock_guard.hpp>
#include <iostream>

#include "common/atomic.h"

namespace pegasus {


/// Runtime profile is a group of profiling counters.  It supports adding named counters
/// and being able to serialize and deserialize them.
/// The profiles support a tree structure to form a hierarchy of counters.
/// Runtime profiles supports measuring wall clock rate based counters.  There is a
/// single thread per process that will convert an amount (i.e. bytes) counter to a
/// corresponding rate based counter.  This thread wakes up at fixed intervals and updates
/// all of the rate counters.
///
/// All methods are thread-safe unless otherwise mentioned.
class RuntimeProfile { // NOLINT: This struct is not packed, but there are not so many
                       // of them that it makes a performance difference
 public:
  class Counter {
   public:
    Counter(int64_t value = 0) :
      value_(value) {
    }
    virtual ~Counter(){}

    virtual void Add(int64_t delta) {
      value_.Add(delta);
    }

    /// Use this to update if the counter is a bitmap
    void BitOr(int64_t delta) {
      int64_t old;
      do {
        old = value_.Load();
        if (LIKELY((old | delta) == old)) return; // Bits already set, avoid atomic.
      } while (UNLIKELY(!value_.CompareAndSwap(old, old | delta)));
    }

    virtual void Set(int64_t value) { value_.Store(value); }

    virtual void Set(int value) { value_.Store(value); }

    virtual void Set(double value) {
      DCHECK_EQ(sizeof(value), sizeof(int64_t));
      value_.Store(*reinterpret_cast<int64_t*>(&value));
    }

    virtual int64_t value() const { return value_.Load(); }

    virtual double double_value() const {
      int64_t v = value_.Load();
      return *reinterpret_cast<const double*>(&v);
    }

    ///  Return the name of the counter type
    virtual string CounterType() const {
      return "Counter";
    }

   protected:
    friend class RuntimeProfile;

    AtomicInt64 value_;
  };

};

}

#endif
