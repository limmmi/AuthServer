#include "Database.h"
#include "Logger.h"
#include <iostream>
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
    
    LOG_INFO("Opening database at: {}", path);
    leveldb::Status status = leveldb::DB::Open(options_, path, &db_);
    if (!status.ok()) {
        LOG_ERROR("LevelDB Open failed: {}", status.ToString());
        db_ = nullptr;
        return false;
    }
    LOG_INFO("Database opened successfully");
    
    return true;
}

void Database::Close() {
    if (db_) {
        delete db_;
        db_ = nullptr;
    }
}

bool Database::Put(const std::string& key, const std::string& value) {
    if (!db_) {
        LOG_ERROR("Database Put failed: database not open");
        return false;
    }
    
    leveldb::Status status = db_->Put(leveldb::WriteOptions(), key, value);
    if (!status.ok()) {
        LOG_ERROR("Database Put failed for key '{}': {}", key, status.ToString());
        return false;
    }
    return true;
}

bool Database::Get(const std::string& key, std::string& value) {
    if (!db_) {
        LOG_ERROR("Database Get failed: database not open");
        return false;
    }
    
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    return status.ok();
}

bool Database::Delete(const std::string& key) {
    if (!db_) {
        LOG_ERROR("Database Delete failed: database not open");
        return false;
    }
    
    leveldb::Status status = db_->Delete(leveldb::WriteOptions(), key);
    if (!status.ok()) {
        LOG_ERROR("Database Delete failed for key '{}': {}", key, status.ToString());
        return false;
    }
    LOG_INFO("Database key deleted: {}", key);
    return true;
}

bool Database::Exists(const std::string& key) {
    if (!db_) return false;
    
    std::string value;
    leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &value);
    return status.ok();
}
