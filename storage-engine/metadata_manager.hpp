#pragma once

#include <iostream>
#include <filesystem>
#include <string>
#include <bits/stdc++.h>

namespace fs = std::filesystem;

namespace ak {

/**
 * @brief Class for managing tables metadata
 */
class MetadataManager{
    private:
        std::string _base_path; ///< Path to the metadata file

        void _clean_string(std::string& s) {
            if (!s.empty() && s.back() == '\r') {
                s.pop_back();
            }
        }

    public:
        /**
         * @brief Deafult constructor
         */
        MetadataManager() : _base_path("") {}
        
        /**
         * @brief Constructor with a path
         * @param path Path to a metadata file
         */
        MetadataManager(std::string path) : _base_path(path) {}
        
        void add_table_entry(Table& table_to_add){
            std::ofstream file(_base_path, std::ios::app);
            if (file.is_open()){
                std::string metadata_string = table_to_add.getName()+"\n";
                std::string columns_config;
                int size_of_columns = 0;
                std::vector columns = table_to_add.getColumns();

                for(const auto& col: columns){
                    columns_config += col.getColumnConfig()+"\n";
                    size_of_columns += col.sizeis();
                }

                std::string size_string = std::to_string(size_of_columns);

                metadata_string += size_string+"\n"+std::to_string(columns.size())+"\n"+columns_config;
                file << metadata_string;

                file.close();

                std::cout << "Metadata updated succesfully.\n";
            } else {
                std::cerr<< "Failed to update metadata.\n";
            }
        }

        void remove_table_entry(std::string table_name) {
            std::string temp_metadata = _base_path+".tmp";

            std::ifstream metafile(_base_path, std::ios::binary);
            std::ofstream tempmeta(temp_metadata, std::ios::binary);
            
            if (!metafile.is_open() || !tempmeta.is_open()) {
                std::cerr << "Error opening metadata file.\n";
                return;
            }
            
            std::string current_name;
            std::string column_count_str;
            std::string skip_string;
            std::string cell_size;

            while(std::getline(metafile, current_name)) {
                std::getline(metafile, cell_size);
                std::getline(metafile, column_count_str);
                
                int column_count = std::stoi(column_count_str);
                _clean_string(current_name);
                if(current_name == table_name) {
                    for(int i = 0; i<column_count; i++){
                        std::getline(metafile, skip_string);
                    }

                    std::cout << "Metadata cleared.\n";
                    tempmeta << metafile.rdbuf();

                    break;
                } else {
                    tempmeta << current_name << "\n";
                    tempmeta << cell_size << "\n";
                    tempmeta << column_count_str << "\n";
                    
                    for(int i = 0; i<column_count; i++){
                        std::getline(metafile, skip_string);
                        tempmeta << skip_string << "\n";
                    }
                }
                metafile.peek();
            }
            metafile.close();
            tempmeta.close();
            fs::remove(_base_path);
            fs::rename(temp_metadata, _base_path);
        }

        std::vector<std::string> get_table_config(std::string table_name){
            std::ifstream metafile(_base_path, std::ios::binary);
            std::vector<std::string> conf_vector;

            if(!metafile.is_open()){
                std::cerr << "Error opening metadata file.\n";
                return conf_vector;
            }

            std::string current_name;
            std::string skip_trans_string;
            std::string column_cnt_str;
            std::string cell_size;

            while(std::getline(metafile, current_name)){
                std::getline(metafile, cell_size);
                std::getline(metafile, column_cnt_str);
                
                int column_count = std::stoi(column_cnt_str);
                _clean_string(current_name);
                if(current_name == table_name) {
                    conf_vector.push_back(cell_size);
                    conf_vector.push_back(column_cnt_str);

                    for(int i = 0; i<column_count; i++){
                        std::getline(metafile, skip_trans_string);
                        conf_vector.push_back(skip_trans_string);
                    }

                    return conf_vector;
                } else {
                    for(int i = 0; i<column_count; i++){
                        std::getline(metafile, skip_trans_string);
                    }
                }
            }
            metafile.peek();
            metafile.close();
            std::cout<<"No table with this name exists in metadata file.\n";
            return conf_vector;
        }
};

}