#pragma once

#include <string>

class PageHeader {
    protected:
        int _page_id;
        int _size = 144;

    public:
        PageHeader(int page_id) : _page_id(page_id) {}
        PageHeader() : _page_id(0) {}
};


class Page {
    //Abstract class for different type of pages implementation (template)
    protected:
        int _size = 4096;
        std::string _table_path;

    public:
        Page(std::string table_path) : _table_path(table_path) {}

        void add_record(){}

        void delete_record(){}

        void create_page(){}
};