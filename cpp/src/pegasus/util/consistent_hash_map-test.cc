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

#include <iostream>
#include <string>
#include <stdint.h>

#include <boost/functional/hash.hpp>
#include <boost/format.hpp>
#include <boost/crc.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/thread.hpp> // required, otherwise link error
//#include <glog/logging.h>
#include "test/gtest-util.h"

#include "consistent_hash_map.hpp"

namespace pegasus {

struct crc32_hasher {
    uint32_t operator()(const std::string& node) {
        boost::crc_32_type ret;
        ret.process_bytes(node.c_str(),node.size());
        return ret.checksum();
    }
    typedef uint32_t result_type;
};

TEST(ConsistentHashMapTest, Unit) {
#if 1
    typedef consistent_hash_map<std::string,crc32_hasher> consistent_hash_t;
    consistent_hash_t consistent_hash_;
    boost::format node_fmt("192.168.1.%1%");

    std::cout<<"setup conhash:"<<std::endl;
    for(std::size_t i=0;i<3;++i) {
        std::string node = boost::str(node_fmt % i);
        consistent_hash_.insert(node);
        std::cout<<boost::format("add node: %1%") % node << std::endl;
    }

{
    std::cout<<"========================================================="<<std::endl;
    std::cout<<"iterate the conhash:"<<std::endl;
    for(consistent_hash_t::iterator it = consistent_hash_.begin();it != consistent_hash_.end(); ++it) {
        std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    }
}

{
    crc32_hasher h;
    std::cout << "h(\"teststring\"): " << h("teststring") << std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(h("teststring"));
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
}

{
    std::cout<<"look for 290235110:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(290235110);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
}

{
    std::cout<<"look for 2286285664:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(2286285664);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
}

{
    std::cout<<"look for 4282565578:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(4282565578);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
}  

{
    std::cout<<"look for 1234567890:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(1234567890);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
}  

  std::cout<<"========================================================="<<std::endl;
{
    std::string node = boost::str(node_fmt % 1);
    std::cout<<"removing node: " << node << std::endl;
    consistent_hash_.erase(node);
    std::cout<<"remaining nodes: " << std::endl;
    for(consistent_hash_t::iterator it = consistent_hash_.begin();it != consistent_hash_.end(); ++it) {
        std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    }
}

{
    crc32_hasher h;
    std::cout << "h(\"teststring\"): " << h("teststring") << std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(h("teststring"));
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
}

  std::cout<<"========================================================="<<std::endl;
{
    std::cout<<"look for 4282565578:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(4282565578);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    std::cout<<"-------------------------------------------"<<std::endl;
    std::cout<<"removing the node for 4282565578:"<<std::endl;
    consistent_hash_.erase(it);
    std::cout<<"remaining nodes: " << std::endl;
    for(consistent_hash_t::iterator it = consistent_hash_.begin();it != consistent_hash_.end(); ++it) {
        std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    }
}  

  std::cout<<"========================================================="<<std::endl;
{
    std::cout<<"-------------------------------------------"<<std::endl;
    std::cout<<"look for 4282565578:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(4282565578);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    std::cout<<"-------------------------------------------"<<std::endl;

    std::cout<<"look for 4282565576:"<<std::endl;
    it = consistent_hash_.find(4282565576);
    std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    std::cout<<"-------------------------------------------"<<std::endl;

    std::cout<<"removing the node for 4282565576:"<<std::endl;
    consistent_hash_.erase(it);

    std::cout<<"remaining nodes: " << std::endl;
    for(consistent_hash_t::iterator it = consistent_hash_.begin();it != consistent_hash_.end(); ++it) {
        std::cout<<boost::format("node:%1%,%2%") % it->second % it->first << std::endl;
    }
    std::cout<<"-------------------------------------------"<<std::endl;
}


  std::cout<<"========================================================="<<std::endl;
{
    std::cout<<"-------------------------------------------"<<std::endl;
    std::cout<<"look for 4282565578:"<<std::endl;
    consistent_hash_t::iterator it;
    it = consistent_hash_.find(4282565578);
    if(it == consistent_hash_.end()) {
        std::cout<<"not found, consistent_hash is empty"<<std::endl;
    }
}
#endif
}


}

PEGASUS_TEST_MAIN();
