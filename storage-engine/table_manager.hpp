#pragma once

#include <iostream>
#include <string>
#include <filesystem>
#include <bits/stdc++.h>
#include "storage_engine.hpp"
#include "slotted_page.hpp"
#include <iomanip>
#include <bitset>
#include "../indexing/b_tree.hpp"
#include "record.hpp"

namespace fs = std::filesystem;

class TableManager : protected StorageEngine{
    private:
        BTree index;

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
    
        int _find_id_column_index(std::vector<std::string>& conf_vector) {
            int number_of_columns = std::stoi(conf_vector[1]);
            
            for (int i = 0; i < number_of_columns; i++) {
                std::stringstream ss(conf_vector[i + 2]);
                std::string segment;
                std::vector<std::string> parts;
                
                while(std::getline(ss, segment, ',')) {
                    parts.push_back(segment);
                }
                
                int col_type = std::stoi(parts[1]);
                if (col_type == ID) {
                    return i;
                }
            }
            return 0;
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

            int id_column_index = _find_id_column_index(conf_vector);
            int key = std::stoi(values[id_column_index]);

            // uniqueness check
            std::pair<int, int> existing = index.search(key, db_name, table_name);
            if (existing.first != -1 || existing.second != -1) {
                throw std::invalid_argument("Duplicate key: record with ID " + std::to_string(key) + " already exists");
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
            } else {
                throw std::runtime_error("Could not open table file for writing");
            }

            index.insert(key, page_id, row_index, db_name, table_name); 
        }

        void delete_record(std::string db_name, std::string table_name, int key){
            std::string metadata_path = _main_directory + "/" + db_name + "/" + _meta_file;
            std::string table_path = _main_directory + "/" + db_name + "/" + table_name;

            std::pair<int, int> position = index.search(key, db_name, table_name);
            
            if (position.first == -1 && position.second == -1) {
                throw std::runtime_error("Record with key " + std::to_string(key) + " not found");
            }
            
            int page_id = position.first;
            int slot_index = position.second;

            std::fstream file(table_path, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open table file");
            }

            // Read header
            std::vector<uint8_t> buffer(144);
            file.seekg(page_id * 4096, std::ios::beg);
            file.read(reinterpret_cast<char*>(buffer.data()), 144);
            
            SlottedPageHeader header(buffer);
            
            // Mark the slot as free in bitmap
            header.modify_bitmap(slot_index, true);  // true = mark as free
            
            // Write updated header back
            std::vector<uint8_t> header_conf = header.getHeaderConfig();
            file.seekp(page_id * 4096, std::ios::beg);
            file.write(reinterpret_cast<char*>(header_conf.data()), header.getHeaderSize());
            
            // zero out the cell data
            MetadataManager mgr(metadata_path);
            std::vector<std::string> conf_vector = mgr.get_table_config(table_name);
            int cell_size = std::stoi(conf_vector[0]);
            
            long long cell_offset = (page_id * 4096) + header.getHeaderSize() + (slot_index * cell_size);
            std::vector<char> zeros(cell_size, 0);
            file.seekp(cell_offset, std::ios::beg);
            file.write(zeros.data(), cell_size);
            
            file.close();
            
            // I do not remove from B-tree (tombstone approach)
            // The slot is now free and can be reused
            std::cout << "Record with key " << key << " deleted successfully.\n";
        }

        Record retrieve_record(std::string db_name, std::string table_name, int key) {
            std::string metadata_path = _main_directory + "/" + db_name + "/" + _meta_file;
            std::string table_path = _main_directory + "/" + db_name + "/" + table_name;

            // Search for record location using B-tree
            std::pair<int, int> position = index.search(key, db_name, table_name);
            
            if (position.first == -1 && position.second == -1) {
                throw std::runtime_error("Record with key " + std::to_string(key) + " not found");
            }
            
            int page_id = position.first;
            int slot_index = position.second;

            // Get cell size from metadata
            MetadataManager mgr(metadata_path);
            std::vector<std::string> conf_vector = mgr.get_table_config(table_name);
            int cell_size = std::stoi(conf_vector[0]);
            int header_size = 144;

            long long cell_offset = (page_id * 4096) + header_size + (slot_index * cell_size);
            
            std::ifstream file(table_path, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open table file");
            }
            
            std::vector<uint8_t> record_data(cell_size);
            file.seekg(cell_offset, std::ios::beg);
            file.read(reinterpret_cast<char*>(record_data.data()), cell_size);
            file.close();
            
            Record record;
            record.data = record_data;
            record.conf_vector = conf_vector;
            
            return record;
        }
};