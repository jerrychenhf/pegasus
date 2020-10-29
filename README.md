# Pegasus - Distributed & Accelerated Data Service
Pegasus is fully distributed data service for providing a scalable and speed data access layer for modern distribued computing framework such as Spark or Flink. Pegasus targets to implement a distributed data/cache service which seats with the computing layer providing the centralized store management for fast data/cache media with APIs satisfy the needs for different scenarios such as Table/Dataset/DataFrame, RDD or Shuffle. 

## Background
We saw the need of a separate (from compute) data/cache layer but collocated with computing Layer to accelerate data access of computing. The data/cache service can manage all the cache resources in a centralized way so that they can shared among systems and be full utilized. The need of such service is driven by the following factors:
* The trend of seperation of storage and compute.
* Foundamental different speed of storage medias (such as DRAM, DCPMM, Optane SSD, NVMe SSD, SSD, HDD) are available in the system.
* Varous data scenarios (for example, Input Data (OAP), RDD, Shuffle¡­ Input Data (OAP), RDD, Shuffle) have very different requirements of accessing speed.

## Goal of Pegasus
Pegaus is to address the above issues by implementing a distributed data/cache service which seats with computing layer providing the centralized store management for data/cache media with APIs satisfy the needs for different scenarios.
Below are a few key concepts and goals:
* Distributed data/cache service seats with computing layer<br/>
  It also means the system and the cached data can be shared among different systems such as Spark, Flink, ¡­
* Centralized store management for fast medias in the system<br/>
    Which means the data media at the compute nodes can be centralized managed and shared among different use cases and system components, making more efficient and full utilization for fast media at the compute node
* Hardware Acceleration<br/>
    Because it is a seperate data service layer, it can improve independently to support various hardware accelerations, AVX(compression, ¡­), PMem, RDMA,¡­

## Build and Develop
Please refer to [Developer Guide](docs/developer-guide.md) 
