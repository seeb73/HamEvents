#ifndef XML_PARSER_H
#define XML_PARSER_H

#include <string>
#include <map>

class XmlParser {
public:
    std::map<std::string, std::string> parse(const std::string& data);
    std::map<std::string, std::string> parseRecord(const std::string& data); // <--- Zgodność z nowym procesorem
};

#endif // XML_PARSER_H