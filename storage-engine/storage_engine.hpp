#pragma once

#include <iostream>
#include <filesystem>
#include <string>
#include "db_table.hpp"
#include "metadata_manager.hpp"

namespace fs = std::filesystem;

class StorageEngine{
    protected:
        std::string _main_directory = "cholopDB";
        std::string _meta_file = "metadata.meta";

    public:
        void create_db(std::string db_name){
            if (!fs::exists(_main_directory)) fs::create_directory(_main_directory);
            
            std::string db_directory = _main_directory+"/"+db_name;
            if (fs::create_directory(db_directory)){
                std::cout << "Database successfully created.\n";
            }else {
                std::cout << "Database with this name already exists.\n";
            }
        };

        void create_table(std::string db_name, std::string table_name, std::vector<Column> columns){
            std::string db_directory = _main_directory+"/"+db_name;
            if(!fs::exists(db_directory)) {
                std::cerr << "The database: " << db_name << " does not exists.\n";
                return;
            }

			std::string table_path = _main_directory+"/"+db_name+"/"+table_name;
			
            // Validation
			Table validate_table(table_name, columns);
            if (!fs::exists(table_path)){
                std::ofstream file(table_path);
                if (file) std::cout << "Table was created succesfully.\n";
                else {
                    std::cerr << "Error creating table at: " << table_path << std::endl;
                    return;
                }
            }
            else {
                throw std::invalid_argument("Table with this name already exists");
            }
            std::string meta_path = _main_directory + "/" + db_name + "/" + _meta_file;
        
            MetadataManager meta_mgr(meta_path);
            meta_mgr.add_table_entry(validate_table);
		}
        
        void drop_table(std::string db_name, std::string table_name){
            std::string meta_path = _main_directory + "/" + db_name + "/" + _meta_file;
        
            MetadataManager meta_mgr(meta_path);
            meta_mgr.remove_table_entry(table_name);

            std::string table_path = _main_directory + "/" + db_name + "/" + table_name;

            if (fs::remove(table_path)) { 
                std::cout << "Table deleted successfully.\n";
            } else {
                std::cout << "Table not found or could not be deleted.\n";
            }
        }

        void drop_db(std::string db_name){
            std::string db_path = _main_directory+"/"+db_name;
            if(fs::remove_all(db_path)){
                std::cout << "Database was succesfully deleted.\n";
            }
        }

};