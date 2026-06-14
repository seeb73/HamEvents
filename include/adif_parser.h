#ifndef ADIF_PARSER_H
#define ADIF_PARSER_H

#include <string>
#include <map>

class AdifParser {
public:
    static std::map<std::string, std::string> parseRecord(const std::string& adifRecord);
};

#endif // ADIF_PARSER_H
