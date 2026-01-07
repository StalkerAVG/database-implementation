#pragma once

#include <iostream>
#include <string>
#include <bits/stdc++.h>

// Tbh this was just to overload operators (*^_^*)
struct Record {
    std::vector<uint8_t> data;
    std::vector<std::string> conf_vector;
    
    friend std::ostream& operator<<(std::ostream& os, const Record& record) {
        if (record.data.empty() || record.conf_vector.empty()) {
            os << "(empty record)";
            return os;
        }
        
        int number_of_columns = std::stoi(record.conf_vector[1]);
        int current_offset = 0;
        
        os << "{ ";
        
        for (int i = 0; i < number_of_columns; i++) {
            std::stringstream ss(record.conf_vector[i + 2]);
            std::string segment;
            std::vector<std::string> parts;
            
            while(std::getline(ss, segment, ',')) {
                parts.push_back(segment);
            }
            
            std::string col_name = parts[0];
            int col_type = std::stoi(parts[1]);
            int col_size = std::stoi(parts[2]);
            
            os << col_name << ": ";
            
            if (col_type == BIGTEXT || col_type == SMALLTEXT) {
                std::string text_val(reinterpret_cast<const char*>(&record.data[current_offset]), col_size);
                text_val.erase(std::find(text_val.begin(), text_val.end(), '\0'), text_val.end());
                os << "\"" << text_val << "\"";
            }
            else if (col_type == ID || col_type == BIGINT || col_type == SMALLINT) {
                if (col_size == 2) {
                    short val;
                    std::memcpy(&val, &record.data[current_offset], 2);
                    os << val;
                } else if (col_size == 4) {
                    int val;
                    std::memcpy(&val, &record.data[current_offset], 4);
                    os << val;
                } else {
                    long long val;
                    std::memcpy(&val, &record.data[current_offset], 8);
                    os << val;
                }
            }
            else if (col_type == FLOAT) {
                float val;
                std::memcpy(&val, &record.data[current_offset], 4);
                os << val;
            }
            
            current_offset += col_size;
            
            if (i < number_of_columns - 1) {
                os << ", ";
            }
        }
        
        os << " }";
        return os;
    }
};