//
// Created by Jason Li on 2024/3/13.
//

#include <vector>

#ifndef ECE141_ARCHIVE_IDATAPROCESSOR_H
#define ECE141_ARCHIVE_IDATAPROCESSOR_H

#endif //ECE141_ARCHIVE_IDATAPROCESSOR_H
class IDataProcessor {
public:
    virtual std::vector<uint8_t> process(const std::vector<uint8_t>& input) = 0;
    virtual std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) = 0;
    virtual ~IDataProcessor(){};
};