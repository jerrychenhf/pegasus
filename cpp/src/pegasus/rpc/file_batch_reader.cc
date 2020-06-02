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

/// \brief Data structure providing an opaque identifier or credential to use
/// when requesting a data stream with the DoGet RPC

#include "rpc/file_batch_reader.h"
#include "arrow/result.h"
#include "arrow/ipc/dictionary.h"
#include "dataset/dataset_cache_block_manager.h"

namespace pegasus {

namespace rpc {
  
  CachedFileBatchReader::CachedFileBatchReader() {

  }

  CachedFileBatchReader::CachedFileBatchReader(
    std::vector<std::shared_ptr<CachedColumn>> columns): columns_(columns){
     if (columns_.size() < 0) {
      // return arrow::Status::Invalid("The cached columns size is 0 !!!");
    } 

    std::shared_ptr<CachedColumn> column = columns_[0]; 
    CacheRegion* cache_region = column->GetCacheRegion();
    rowgroup_nums_ = cache_region->object_entrys().size();
    absolute_rowgroup_position_ = 0;
  }
  
  std::shared_ptr<arrow::Schema> CachedFileBatchReader::schema() const {
    //TO DO
    return nullptr;
  }
  
  arrow::Status CachedFileBatchReader::ReadNext(std::shared_ptr<FileBatch>* out) {
    // column1 -> (rowgroup1-> buffer, rowgroup2 -> buffer)
    // columns2 -> (rowgroup1 -> buffer, rowgroup2 -> buffer)
    // FileBatch1
    //          column1 -> (rowgroup1-> buffer)
    //          column2 -> (rowgroup1 -> buffer)
    // FileBatch2
    //          column1 -> (rowgroup2-> buffer)
    //          column2 -> (rowgroup2 -> buffer) 
    if (columns_.size() < 0) {
      *out = nullptr;
      return arrow::Status::OK();
    }
    
    std::vector<std::shared_ptr<ObjectEntry>> batch_data(columns_.size());

    // Traverse the columns and create the FileBatch for each rowgroup
    for (int i = 0; i < columns_.size(); ++i) {
      std::shared_ptr<CachedColumn> column = columns_[i];
      CacheRegion* cache_region = column->GetCacheRegion();
      unordered_map<int, std::shared_ptr<ObjectEntry>> object_entrys =
       cache_region->object_entrys();

      auto iter  = object_entrys.find(absolute_rowgroup_position_);
      std::shared_ptr<ObjectEntry> object_entry = iter->second;
      batch_data[i] = object_entry;
    }
  
    *out = std::make_shared<FileBatch>(absolute_rowgroup_position_, batch_data);
    absolute_rowgroup_position_ += 1;
    return arrow::Status::OK();
  }
  
  // ----------------------------------------------------------------------
// RecordBatchStreamReader implementation

class FileBatchStreamReaderImpl : public FileBatchStreamReader {
 public:
  arrow::Status Open(std::unique_ptr<arrow::ipc::MessageReader> message_reader,
              const arrow::ipc::IpcOptions& options) {
    message_reader_ = std::move(message_reader);
    options_ = options;

    // Read schema
    std::unique_ptr<arrow::ipc::Message> message;
    RETURN_NOT_OK(message_reader_->ReadNextMessage(&message));
    if (!message) {
      return arrow::Status::Invalid("Tried reading schema message, was null or length 0");
    }

    //TO DO
    //read schema
    /*
    return UnpackSchemaMessage(*message, options, &dictionary_memo_, &schema_,
                               &out_schema_, &field_inclusion_mask_);
    */
    return arrow::Status::OK();
  }

  arrow::Status ReadNext(std::shared_ptr<FileBatch>* batch) override {
    if (empty_stream_) {
      // ARROW-6006: Degenerate case where stream contains no data, we do not
      // bother trying to read a RecordBatch message from the stream
      *batch = nullptr;
      return arrow::Status::OK();
    }

    std::unique_ptr<arrow::ipc::Message> message;
    RETURN_NOT_OK(message_reader_->ReadNextMessage(&message));
    if (message == nullptr) {
      // End of stream
      *batch = nullptr;
      return arrow::Status::OK();
    }

    //TO DO
    // read the buffer into FileBatch
    /*
      CHECK_HAS_BODY(*message);
      ARROW_ASSIGN_OR_RAISE(auto reader, Buffer::GetReader(message->body()));
      return ReadRecordBatchInternal(*message->metadata(), schema_, field_inclusion_mask_,
                                     &dictionary_memo_, options_, reader.get())
          .Value(batch);
    */
    return arrow::Status::OK();
  }

  std::shared_ptr<arrow::Schema> schema() const override { return out_schema_; }

 private:
  std::unique_ptr<arrow::ipc::MessageReader> message_reader_;
  arrow::ipc::IpcOptions options_;
  std::vector<bool> field_inclusion_mask_;

  bool empty_stream_ = false;

  arrow::ipc::DictionaryMemo dictionary_memo_;
  std::shared_ptr<arrow::Schema> schema_, out_schema_;
};
  
arrow::Status FileBatchStreamReader::Open(std::unique_ptr<arrow::ipc::MessageReader> message_reader,
                                     std::unique_ptr<FileBatchReader>* out) {
  auto result =
      std::unique_ptr<FileBatchStreamReaderImpl>(new FileBatchStreamReaderImpl());
  RETURN_NOT_OK(result->Open(std::move(message_reader), arrow::ipc::IpcOptions::Defaults()));
  *out = std::move(result);
  return arrow::Status::OK();
}

} // namespace rpc

} // namespace pegasus
