#pragma once

#include <string>

namespace ak {

/**
* @brief Base class of page header
*/
class PageHeader {
    protected:
        int _page_id; ///< Id of the page
        int _size = 144; ///< Size of the header

    public:
        /**
        * @brief Constructor with page id
        * @param page_id Page indentifier
        */
        PageHeader(int page_id) : _page_id(page_id) {}

        /**
        * @brief Default constructor
        */
        PageHeader() : _page_id(0) {}

        virtual ~PageHeader() = default;
        
        int getPageId() const { return _page_id; }
        int getSize() const { return _size; }
};

/**
* @brief Base class of DB pages
*/
class Page {
    protected:
        int _size = 4096; ///< Size in bytes
        std::string _table_path; ///< Local path to the table

    public:
        /**
        * @brief Default constructor
        */
        Page() : _table_path("") {}
        
        /**
        * @brief Constructor with path to the table
        * @param table_path Local path to the table
        */
        Page(std::string table_path) : _table_path(table_path) {}

        virtual ~Page() = default;
        
        void add_record(){}

        void delete_record(){}

        void create_page(){}
};

}