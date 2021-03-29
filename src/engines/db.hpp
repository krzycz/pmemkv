#ifndef TAIR_CONTEST_INCLUDE_DB_H_
#define TAIR_CONTEST_INCLUDE_DB_H_

#include <cstdio>
#include <cstring>
#include <string>

enum Status : unsigned char {
    Ok,
    NotFound,
    IOError,
    OutOfMemory,
};

class Slice {
public:
    Slice() : _data(nullptr), _size(0) {}
    Slice(char* data) : _data(data) {
        _size = strlen(_data);
    }
    Slice(char* data, uint64_t size) : _data(data), _size(size) {}

    const char *&data() {
        return _data;
    }

    const char *data() const {
        return _data;
    }

    uint64_t &size() {
        return _size;
    }

    uint64_t size() const {
        return _size;
    }

    bool operator==(const Slice& b) {
        if (b.size() == this->_size && memcmp(this->_data, b.data(), b.size()) == 0) {
            return true;
        } else {
            return false;
        }
    }
    
    std::string to_string() {
        return std::string(_data, _size);
    }

    std::string to_string() const {
        return std::string(_data, _size);
    }

private:
    const char* _data;
    uint64_t _size;
};

class DB {
public:
    /*
     *  Create or recover db from pmem-file.
     *  It's not required to implement the recovery in round 1.
     *  You can assume that the file does not exist.
     *  You should write your log to the log_file. 
     *  Stdout, stderr would be redirect to /dev/null.
     */
    static Status CreateOrOpen(const std::string& name, DB** dbptr, FILE* log_file = nullptr);

    /*
     *  Get the value of key.
     *  If the key does not exist the NotFound is returned.
     */
    virtual Status Get(const Slice& key, std::string* value) = 0;

    /*
     *  Set key to hold the string value.
     *  If key already holds a value, it is overwritten. 
     */
    virtual Status Set(const Slice& key, const Slice& value) = 0;

    /*
     * Close the db on exit.
     */
    virtual ~DB() = 0;
};

#endif