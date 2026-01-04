#pragma once

#include <iostream>
#include "page.hpp"
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

class SlottedPageHeader : public PageHeader {
    private:
        int _cell_size;
        uint8_t _usage_bitmap[128];
        bool _have_free_space = 1;
        int _row_count = 0;

    public:
        SlottedPageHeader(int id, int cell_size): PageHeader(id), _cell_size(cell_size), _row_count(0) 
        {
            std::memset(_usage_bitmap, 0, 128);
        }
        
        SlottedPageHeader(const std::vector<uint8_t>& buffer) : PageHeader(0) {
            if (buffer.size() != _size) {
                throw std::runtime_error("Invalid header buffer size");
            }

            std::memcpy(&_page_id, &buffer[0], 4);
            std::memcpy(&_cell_size, &buffer[4], 4);
            std::memcpy(&_row_count, &buffer[8], 4);
            
            std::memcpy(&_have_free_space, &buffer[12], 1);

            std::memcpy(_usage_bitmap, &buffer[16], 128);
        }

        SlottedPageHeader() {}

        int check_for_free_space() {
            int max_slots = (4096-_size) / _cell_size; 

            for(int byte_idx = 0; byte_idx < 128; byte_idx++){
                uint8_t byte = _usage_bitmap[byte_idx];

                if(byte != 0xFF){
                    for (int bit_idx = 0; bit_idx < 8; bit_idx++) {
                        if (!((byte >> bit_idx) & 1)) {

                            int candidate_slot = (byte_idx * 8) + bit_idx;

                            if (candidate_slot >= max_slots) {
                                return -1;
                            }

                            return candidate_slot;
                        }
                    }
                }
            }
            return -1;
        }

        int getHeaderSize() const { return _size; }
        int getCellSize() const { return _cell_size; }

        std::vector<uint8_t> getHeaderConfig() const{
            std::vector<uint8_t> buffer(_size, 0);

            // Byte 0-3: Page ID (4 bytes)
            std::memcpy(&buffer[0], &_page_id, 4);

            // Byte 4-7: Cell Size (4 bytes)
            std::memcpy(&buffer[4], &_cell_size, 4);

            // Byte 8-11: Row Count
            std::memcpy(&buffer[8], &_row_count, 4);

            // Byte 12: Bool
            std::memcpy(&buffer[12], &_have_free_space, 1);

            // Byte 13-15: Padding (Automatically 0)

            // Byte 16-143: Bitmap (128 bytes)
            std::memcpy(&buffer[16], _usage_bitmap, 128);

            return buffer;
        }

        void update_config(int row_count){
            _row_count = row_count;
        }

        void modify_bitmap(int slot_index, bool is_free) {
                int byte_idx = slot_index / 8;
                int bit_idx  = slot_index % 8;

                if (is_free) {
                    if (_usage_bitmap[byte_idx] & (1 << bit_idx)) {
                        _usage_bitmap[byte_idx] &= ~(1 << bit_idx); // Set to 0
                        _row_count--; 
                        _have_free_space = true;
                    }
                } 
                else {
                    if (!(_usage_bitmap[byte_idx] & (1 << bit_idx))) {
                        _usage_bitmap[byte_idx] |= (1 << bit_idx); // Set to 1
                        _row_count++;

                        int max_slots = (4096 - _size) / _cell_size;
                        if (_row_count >= max_slots) {
                            _have_free_space = false;
                        }
                    }
                }
        }
};


class SlottedPage : public Page {
    private:
        SlottedPageHeader _header;

        float _strict_stof(const std::string& str) {
            size_t pos = 0;
            float val;
            
            try {
                val = std::stof(str, &pos);
            } catch (...) {
                throw std::invalid_argument("Invalid float format: " + str);
            }

            if (pos != str.length()) {
                throw std::invalid_argument("Invalid characters found after number: " + str);
            }
            return val;
        }

    public:
        SlottedPage(std::string table_path, SlottedPageHeader& header): Page(table_path), _header(header) {}
        
        std::vector<uint8_t> serialize_row(
            std::vector<std::string>& values,
            std::vector<std::string>& conf_vector
        ){
            std::vector<uint8_t> buffer(_header.getCellSize(), 0);
            int current_offset = 0;
            int number_of_columns = std::stoi(conf_vector[1]);

            if (values.size() != number_of_columns) {
                throw std::invalid_argument("Argument count mismatch.");
            }

            for (int i = 0; i < number_of_columns; i++) {
                std::stringstream ss(conf_vector[i + 2]);
                std::string segment;
                std::vector<std::string> parts;
                
                while(std::getline(ss, segment, ',')) {
                    parts.push_back(segment);
                }

                int col_type = std::stoi(parts[1]); 
                int col_size = std::stoi(parts[2]);
                if (col_type == BIGTEXT || col_type == SMALLTEXT){
                    if (values[i].size() > col_size) throw std::invalid_argument("Text too long for column " + parts[0]);

                    std::memcpy(&buffer[current_offset], values[i].c_str(), values[i].size());
                }
                else if (col_type == ID || col_type == BIGINT || col_type == SMALLINT) {      
                    try {
                        long long val = std::stoll(values[i]);

                        if(col_type == SMALLINT){
                            if(val>32767) throw std::invalid_argument("Top big for SMALLINT");
                            if(val<(-32767)) throw std::invalid_argument("Top small for SMALLINT");
                        }else if (col_type == ID){
                            if(val>2147483647) throw std::invalid_argument("Top big for ID");
                            if(val<(-2147483647)) throw std::invalid_argument("Top small for ID");
                        }
                        // } else {
                        //     if(val>9223372036854775808) throw std::invalid_argument("Top big for BIGINT");
                        //     if(val<(-9223372036854775808)) throw std::invalid_argument("Top small for BIGINT");
                        // }

                        if(col_size == 2){
                            short s_val = static_cast<short>(val);
                            std::memcpy(&buffer[current_offset], &s_val, 2);
                        } else if(col_size == 4){
                            int i_val = static_cast<int>(val);
                            std::memcpy(&buffer[current_offset], &i_val, 4);
                        } else {
                            std::memcpy(&buffer[current_offset], &val, 8);
                        }
                    } catch (...) {
                        throw std::invalid_argument("Invalid number for column " + parts[0]);
                    }
                }
                else if (col_type == FLOAT) {
                    std::string clean_val = values[i];
                    std::replace(clean_val.begin(), clean_val.end(), ',', '.');

                    float f_val = _strict_stof(clean_val); 

                    std::memcpy(&buffer[current_offset], &f_val, sizeof(float));
                }
                current_offset += col_size;
            }
            return buffer;
        }
         
        void create_page() {}

        void page_availability() {

        }
};