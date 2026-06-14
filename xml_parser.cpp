/**
 * @file xml_parser.cpp
 * @brief Parsowanie ramek XML Ham Radio Deluxe.
 * @note Wzbogacona o ujednolicanie wielkości liter kluczy (tolower) dla kompatybilności.
 * @version 0.4.8
 */

#include "include/xml_pharser.h" // Oryginalna nazwa nagłówka z dysku
#include "include/logger.h"
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <cctype>
#include <libxml/parser.h>
#include <libxml/tree.h>

std::map<std::string, std::string> XmlParser::parse(const std::string& data) {
    std::map<std::string, std::string> xmlData;
    xmlDocPtr doc = xmlParseMemory(data.c_str(), data.length());

    if (doc == nullptr) {
        Logger::getInstance().log("Błąd parsowania XML: Dokument jest pusty lub nieprawidłowy.");
        return xmlData;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == nullptr) {
        xmlFreeDoc(doc);
        Logger::getInstance().log("Błąd parsowania XML: Brak elementu root.");
        return xmlData;
    }

    for (xmlNodePtr node = root->children; node; node = node->next) {
        if (node->type == XML_ELEMENT_NODE) {
            xmlChar* content = xmlNodeGetContent(node);
            if (content) {
                const xmlChar* name = node->name;
                std::string key = reinterpret_cast<const char*>(name);
                
                // Ujednolicamy klucz do małych liter, aby zachować spójność z bazą i logiką aplikacji
                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
                    return std::tolower(c);
                });

                std::string value = reinterpret_cast<const char*>(content);
                xmlData[key] = value;
                xmlFree(content);
            }
        }
    }
    xmlFreeDoc(doc);
    return xmlData;
}

