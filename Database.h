#ifndef DATABASE_H
#define DATABASE_H

#include <string>
#include <leveldb/db.h>

// LevelDB 数据库封装类
class Database {
public:
    Database();
    ~Database();
    
    // 打开数据库
    bool Open(const std::string& path);
    
    // 关闭数据库
    void Close();
    
    // 存储数据
    bool Put(const std::string& key, const std::string& value);
    
    // 读取数据
    bool Get(const std::string& key, std::string& value);
    
    // 删除数据
    bool Delete(const std::string& key);
    
    // 检查 key 是否存在
    bool Exists(const std::string& key);
    
    // 获取数据库是否打开
    bool IsOpen() const { return db_ != nullptr; }

private:
    leveldb::DB* db_;
    leveldb::Options options_;
};

#endif // DATABASE_H
