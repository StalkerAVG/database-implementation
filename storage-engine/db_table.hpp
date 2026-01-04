#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <bits/stdc++.h>

namespace fs = std::filesystem;

enum ColumnType {
    BIGTEXT = 0,
    ID = 1,
    SMALLTEXT = 2,
    BIGINT = 3,
    SMALLINT = 4,
    FLOAT = 5
};

class Column{
    private:
        std::string _name;
        ColumnType _type;
        bool _id_field;
        inline static const std::map<ColumnType, int> _size_dict = {
            {BIGTEXT, 1024},
            {ID, 4},
            {SMALLTEXT, 256},
            {BIGINT, 8},
            {SMALLINT, 2},
            {FLOAT, 4},
        };
        int _size;

    public:
        Column(std::string name, enum ColumnType type, bool id_field): _name(name), _type(type), _id_field(id_field){
            // Validation
            if(name.empty()) throw std::invalid_argument("Cannot create column with empty name");
            
            _size = sizeis();

            std::cout << "Column called " << name << " created succesfully.\n";
        }
		
		Column(std::string name, enum ColumnType type): Column(name, type, false){}

        bool is_unique() const{
            return _id_field;
        }

        int sizeis() const{
            return _size_dict.at(_type);
        }

        std::string getColumnConfig() const{
            return _name+","+std::to_string(_type)+","+std::to_string(sizeis());
        }

        ColumnType getColumnType() const{
            return _type;
        }
};


class Table{
    private:
        std::string _name;
        std::vector<Column> _columns;

    public:
        Table(std::string name, std::vector<Column>& columns): _name(name), _columns(columns){
            // Validation here
            int id_field_count = 0;
            int size_of_columns = 0;

            for(const auto& col : _columns){
                if (col.is_unique()){
                    id_field_count++;
                    if(col.getColumnType() == SMALLINT) throw std::invalid_argument("SMALLINT cannot be index field");
                }

                size_of_columns += col.sizeis();
            }

            
            if(size_of_columns>(4096-144)){
                std::cout << "Size of columns " << size_of_columns << std::endl;
                throw std::invalid_argument("Sum of all the columns size cannot be more than 3952");
            }

            std::cout<<id_field_count;
            if(id_field_count != 1){
             throw id_field_count < 1 ? std::invalid_argument("At least one column must be identification type of column") : std::invalid_argument("Cannot create more than one column of type identification");
            }

            if(name.empty()) throw std::invalid_argument("Name of the table cannot be empty");
        }

        std::string getName() const {
            return _name;
        }

        std::vector<Column> getColumns() const {
            return _columns;
        }
};