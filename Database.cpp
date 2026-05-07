#include "Database.h"
#include <leveldb/write_batch.h>

Database::Database() : db_(nullptr) {
    options_.create_if_missing = true;
}

Database::~Database() {
    Close();
}

bool Database::Open(const std::string& path) {
    if (db_) {
        return true;  // 已经打开
    }
    
    leveldb::Status status = leveldb::DB::Open(options_, path, &db_);
    if (!status.ok()) {
        db_ = nullptr;
        return false;
    }
    
    return true;
}

void Database::Close() {
    if (db_) {
        delete db_;
        db_ = nullptr;
    }
}

bool Database::Put(const std::string& key, const std::string& value) {
    if (!db_) return false;
    
    leveldb::Status status = db_->Put(leveldb::WriteOptions(), key, value);
    return status.ok();
}

bool Database::Get(const std::string& key, std::string& value) {
    if (!db_) return false;
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    return status.ok();
}

bool Database::Delete(const std::string& key) {
    if (!db_) return false;
    
    leveldb::Status status = db_->Delete(leveldb::WriteOptions(), key);
    return status.ok();
}

bool Database::Exists(const std::string& key) {
    if (!db_) return false;
    
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    return status.ok();
}
