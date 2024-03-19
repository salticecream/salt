#pragma once
#include <string>
enum DataType {
    DT_VOID,
    DT_BOOL,
    DT_INT,
    DT_CHAR,


    DT_TOTAL
};

class Type {
private:
    std::string name_;
    int size_;
public:
    Type(const std::string& name, int size) : name_(name), size_(size) {}
    const std::string& name() const;
    int size() const;
};

extern const Type DEFAULT_TYPES[DT_TOTAL];