#include "types.h"
const Type DEFAULT_TYPES[DT_TOTAL] = {
    Type("void", 0), 
    Type("bool", 1),
    Type("int", 4),
    Type("char", 1)
};

const std::string& Type::name() const {
    return this->name_;
}

int Type::size() const {
    return this->size_;
}