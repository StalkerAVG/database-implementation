#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <bits/stdc++.h>
#include "storage_engine.hpp"
#include "slotted_page.hpp"
#include <iomanip>
#include <bitset>

namespace fs = std::filesystem;

class TableManager : protected StorageEngine{
    private:
    static void debug_hex(const std::vector<uint8_t>& data, std::string label) {
        std::cout << "--- " << label << " (" << data.size() << " bytes) ---\n";
        for (size_t i = 0; i < data.size(); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
            if ((i + 1) % 16 == 0) std::cout << "\n";
        }
        std::cout << std::dec << "\n\n";
    }

    static void debug_bits(const std::vector<uint8_t>& data) {
        std::cout << "--- BIT VIEW ---\n";
        for (auto byte : data) {
            std::cout << std::bitset<8>(byte) << " ";
        }
        std::cout << "\n\n";
    }
    public:

        void add_record(std::string db_name, std::string table_name, std::vector<std::string> values){
            std::string metadata_path = _main_directory+"/"+db_name+"/"+_meta_file;
            std::string table_path = _main_directory + "/" + db_name + "/" + table_name;

            MetadataManager mgr(metadata_path);
            std::vector<std::string> conf_vector = mgr.get_table_config(table_name);

            if (conf_vector.empty()) {
                throw std::invalid_argument("Cannot add record to non-existent table.");
            }

            if (!fs::exists(table_path)) {
                throw std::runtime_error("CRITICAL ERROR: Metadata exists but data file is missing.");
            }

            bool is_new_file = fs::is_empty(table_path);
            int cell_size = std::stoi(conf_vector[0]);
            int page_id = 0;
            SlottedPageHeader header;
            bool found_free_page = false;

            if(!is_new_file){
                std::ifstream file(table_path, std::ios::binary);
                if (!file.is_open()) throw std::runtime_error("Could not open table data");
                std::vector<uint8_t> buffer(144);
                int current_page_id = 0;

                while(1){
                    file.seekg(current_page_id * 4096, std::ios::beg);
                    file.read(reinterpret_cast<char*>(buffer.data()), 144);

                    if (file.gcount() < 144) break;

                    bool has_space = (buffer[12] == 1);
                    
                    if(has_space){
                        found_free_page = true;
                        header = SlottedPageHeader(buffer);
                        break;
                    }
                    current_page_id++;
                }
                page_id = current_page_id;
            }  

            if(!found_free_page){
                header = SlottedPageHeader(page_id, cell_size);
            }
            int row_index = header.check_for_free_space();

            SlottedPage page(table_path, header);
            std::vector<uint8_t> data_blob = page.serialize_row(values, conf_vector);
            std::vector<uint8_t> header_conf_test = header.getHeaderConfig();
            
            // Create a temporary vector for just the bitmap part (bytes 16 to end)
            if (header_conf_test.size() > 16) {
                std::vector<uint8_t> bitmap_only(header_conf_test.begin() + 16, header_conf_test.end());
                debug_bits(bitmap_only);
            }

            header.modify_bitmap(row_index, 0);
            std::vector<uint8_t> header_conf = header.getHeaderConfig();
            
            debug_hex(header_conf, "Header Config"); // Check if ID/Sizes are correct
            debug_hex(data_blob, "Row Data");        // Check if Text/Ints are packed right

            // Create a temporary vector for just the bitmap part (bytes 16 to end)
            if (header_conf.size() > 16) {
                std::vector<uint8_t> bitmap_only(header_conf.begin() + 16, header_conf.end());
                debug_bits(bitmap_only);
            }

            long long file_offset = (page_id * 4096) + header.getHeaderSize() + (row_index * header.getCellSize());

            std::fstream file(table_path, std::ios::in | std::ios::out | std::ios::binary);
            if(file.is_open()){
                file.seekp(page_id * 4096, std::ios::beg);
                file.write(reinterpret_cast<char*>(header_conf.data()), header.getHeaderSize());

                file.seekp(file_offset, std::ios::beg);
                file.write(reinterpret_cast<char*>(data_blob.data()), data_blob.size());
            }
        }

        void delete_record(){

        }

};