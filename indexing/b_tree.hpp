#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

struct IndexEntry {
    int key;    // 4 bytes
    int value;  // 4 bytes Packed location (page << 16 | slot)
    int child_id; // 4 bytes page id on the left

};

struct BTreePageHeader {
    int page_id; //4 bytes
    int is_leaf; // 4 bytes (int cos bool has padding issues)
    int num_keys; // 4 bytes  
    int parent_page_id; // 4 bytes
    int rightmost_child; // 4 bytes (only meaningful for internal nodes)
};


struct BTreePage {
    BTreePageHeader header;
    std::vector<IndexEntry> entries;
};

struct SplitResult {
    bool did_split;
    int promoted_key;
    int promoted_value;
    int new_right_page_id;
};

class BTree{
    private:
        std::string _main_directory = "cholopDB";
        int _node_size; // 4096-20 size of header, we can then store up to 339  records per leaf
        int _max_entries;
        int _file_header_size = 4;
        
        int _get_max_page_id(std::fstream& index_file) {
            index_file.seekg(0, std::ios::beg);
            int max_id;
            index_file.read(reinterpret_cast<char*>(&max_id), sizeof(int));
            return max_id;
        }

        int _allocate_new_page_id(std::fstream& index_file) {
            int current_max = _get_max_page_id(index_file);
            int new_id = current_max + 1;
        
            index_file.seekp(0, std::ios::beg); // update header
            index_file.write(reinterpret_cast<const char*>(&new_id), sizeof(int));
            
            return new_id;
        }

        void _retrieve_page(std::fstream& index_file, int page_id, BTreePage& page) {
            int offset = _file_header_size + page_id * 4096;
            index_file.seekg(offset, std::ios::beg);

            BTreePageHeader header;
            index_file.read(reinterpret_cast<char*>(&header), sizeof(BTreePageHeader));
            
            std::vector<IndexEntry> entries(header.num_keys);
            if (header.num_keys > 0) {
                index_file.read(reinterpret_cast<char*>(entries.data()), 
                header.num_keys * sizeof(IndexEntry));
            }
            page.header = header;
            page.entries = entries;
        }

        void _write_page(BTreePage& page, std::fstream& index_file){
            int offset = _file_header_size + page.header.page_id * 4096;
            index_file.seekp(offset, std::ios::beg);

            index_file.write(reinterpret_cast<const char*>(&page.header), 
                        sizeof(BTreePageHeader));

            if (page.header.num_keys > 0) {
                index_file.write(reinterpret_cast<const char*>(page.entries.data()), 
                page.header.num_keys * sizeof(IndexEntry));
            }

            // Pad the rest of the page with zeros to maintain 4KB size
            int bytes_written = sizeof(BTreePageHeader) + 
                            page.header.num_keys * sizeof(IndexEntry);
            int padding = 4096 - bytes_written;
            if (padding > 0) {
                std::vector<char> zeros(padding, 0);
                index_file.write(zeros.data(), padding);
            }
        }

        SplitResult _insert_into_leaf(BTreePage& leaf, int key, int value, 
                                    std::fstream& index_file) {
            IndexEntry new_entry;
            new_entry.key = key;
            new_entry.value = value;
            new_entry.child_id = -1;  // Not used in leaves
            
            // appending to rightmost, so just push_back
            leaf.entries.push_back(new_entry);
            leaf.header.num_keys++;

            if (leaf.header.num_keys <= _max_entries) {
                _write_page(leaf, index_file);
                return {false, 0, 0, -1};
            }
            
            // Split strategy: left gets [0, mid), promoted gets [mid], right gets [mid+1, end)
            int mid = leaf.header.num_keys / 2;
            
            // Create left page (reuses current page_id)
            BTreePage left_page;
            left_page.header.page_id = leaf.header.page_id;
            left_page.header.is_leaf = 1;
            left_page.header.parent_page_id = leaf.header.parent_page_id;
            left_page.header.rightmost_child = -1;  // Not used in leaves
            left_page.entries.assign(leaf.entries.begin(), leaf.entries.begin() + mid);
            left_page.header.num_keys = left_page.entries.size();
            
            // Create right page (gets new page_id)
            int right_page_id = _allocate_new_page_id(index_file);
            BTreePage right_page;
            right_page.header.page_id = right_page_id;
            right_page.header.is_leaf = 1;
            right_page.header.parent_page_id = leaf.header.parent_page_id;
            right_page.header.rightmost_child = -1;  // Not used in leaves

            // we skip the middle entry since its promoted
            right_page.entries.assign(leaf.entries.begin() + mid + 1, leaf.entries.end());
            right_page.header.num_keys = right_page.entries.size();
            
            // The middle entrys key is promoted to parent
            int promoted_key = leaf.entries[mid].key;
            int promoted_value = leaf.entries[mid].value;
            
            // Write both pages to disk
            _write_page(left_page, index_file);
            _write_page(right_page, index_file);
            
            return {true, promoted_key, promoted_value, right_page_id};
        }

        SplitResult _insert_into_internal(BTreePage& internal, int promoted_key, 
            int promoted_value, int new_right_page_id, std::fstream& index_file) {
            IndexEntry new_entry;
            new_entry.key = promoted_key;
            new_entry.value = promoted_value;
            new_entry.child_id = internal.header.rightmost_child;
            
            internal.entries.push_back(new_entry);
            internal.header.num_keys++;
            internal.header.rightmost_child = new_right_page_id;
            
            if (internal.header.num_keys <= _max_entries) {
                _write_page(internal, index_file);
                return {false, 0, 0, -1};
            }
            
            int mid = internal.header.num_keys / 2;
            
            BTreePage left_page;
            left_page.header.page_id = internal.header.page_id;
            left_page.header.is_leaf = 0;
            left_page.header.parent_page_id = internal.header.parent_page_id;
            left_page.entries.assign(internal.entries.begin(), internal.entries.begin() + mid);
            left_page.header.num_keys = left_page.entries.size();
            left_page.header.rightmost_child = internal.entries[mid].child_id;
            
            int right_page_id = _allocate_new_page_id(index_file);
            BTreePage right_page;
            right_page.header.page_id = right_page_id;
            right_page.header.is_leaf = 0;
            right_page.header.parent_page_id = internal.header.parent_page_id;
            right_page.entries.assign(internal.entries.begin() + mid + 1, internal.entries.end());
            right_page.header.num_keys = right_page.entries.size();
            right_page.header.rightmost_child = internal.header.rightmost_child;
            
            // Promote the middle entry
            int promoted_key_up = internal.entries[mid].key;
            int promoted_value_up = internal.entries[mid].value;
            
            _write_page(left_page, index_file);
            _write_page(right_page, index_file);
            
            return {true, promoted_key_up, promoted_value_up, right_page_id};
        }

        SplitResult _insert_recursive(int page_id, int key, int value, 
            std::fstream& index_file) {
            BTreePage page;
            _retrieve_page(index_file, page_id, page);
            
            if (page.header.is_leaf) {
                // Base case: were at a leaf, insert here
                return _insert_into_leaf(page, key, value, index_file);
            } else {
                // Recursive case: go to rightmost child
                SplitResult child_result = _insert_recursive(page.header.rightmost_child, 
                                            key, value, index_file);
                
                if (!child_result.did_split) {
                    return {false, 0, 0, 0};
                }
                
                return _insert_into_internal(page, child_result.promoted_key, child_result.promoted_value,
                    child_result.new_right_page_id, index_file);
            }
        }
        
        BTreePage _find_rightmost_leaf(std::fstream& index_file) {
            BTreePage page;
            int page_id = 0;  // Start at root
        
            while (true) {
                _retrieve_page(index_file, page_id, page);
                
                if (page.header.is_leaf) {
                    return page;
                }
                
                // Follow rightmost child
                page_id = page.header.rightmost_child;
            }
        }
        
        void _handle_root_split(const SplitResult& result, std::fstream& index_file) {
            int left_child_id = _allocate_new_page_id(index_file);
            int right_child_id = result.new_right_page_id;

            BTreePage left_child;
            _retrieve_page(index_file, 0, left_child);  // Read modified Page 0
            left_child.header.page_id = left_child_id;  // Change ID to new slot
            left_child.header.parent_page_id = 0;       // Parent will be Root (0)
            _write_page(left_child, index_file);        // Save to new location

            BTreePage right_child;
            _retrieve_page(index_file, right_child_id, right_child);
            right_child.header.parent_page_id = 0;
            _write_page(right_child, index_file);

            BTreePage new_root;
            new_root.header.page_id = 0;
            new_root.header.is_leaf = 0;  // Root is always internal after split
            new_root.header.num_keys = 1;
            new_root.header.parent_page_id = -1;
            new_root.header.rightmost_child = right_child_id;

            IndexEntry entry;
            entry.key = result.promoted_key;
            entry.value = result.promoted_value;
            entry.child_id = left_child_id;
            new_root.entries.push_back(entry);

            _write_page(new_root, index_file);
        }
        
        void _update_existing_key(int key, int value, std::fstream& index_file) {
            BTreePage leaf = _search_page(key, index_file);
                    
            // Binary search for the key in the page
            int left = 0, right = leaf.header.num_keys - 1;
            int found_index = -1;
            
            while (left <= right) {
                int mid = left + (right - left) / 2;
                
                if (leaf.entries[mid].key == key) {
                    found_index = mid;
                    break;
                }
                
                if (leaf.entries[mid].key < key) {
                    left = mid + 1;
                } else {
                    right = mid - 1;
                }
            }
            
            if (found_index == -1) {
                throw std::runtime_error("Key not found for update");
            }

            leaf.entries[found_index].value = value;
            _write_page(leaf, index_file);
        }

        BTreePage _search_page(int key, std::fstream& index_file) {
            BTreePage page;
            int page_id = 0;
            
            while (true) {
                _retrieve_page(index_file, page_id, page);

                for (int i = 0; i < page.header.num_keys; i++) {
                    if (page.entries[i].key == key) {
                        return page;  // Found in this page
                    }
                }
                
                if (page.header.is_leaf) {
                    return page;  // Return leaf even if not found
                }
                
                int next_page_id = page.header.rightmost_child;
                
                for (int i = 0; i < page.header.num_keys; i++) {
                    if (key < page.entries[i].key) {
                        next_page_id = page.entries[i].child_id;
                        break;
                    }
                }
                
                page_id = next_page_id;
            }
        }


    public:
        BTree() {
            _max_entries = (4096-sizeof(BTreePageHeader)) / sizeof(IndexEntry);
            _node_size = sizeof(IndexEntry);
        }

        void insert(int key, int row_ptr_page, int row_ptr_slot,
            std::string db_name, std::string table_name)
            {
            std::string index_file_path = _main_directory + "/" + db_name + "/" + table_name + ".idx";
            std::fstream index_file(index_file_path, std::ios::in | std::ios::out | std::ios::binary);

            if (!index_file.is_open()) {
                throw std::runtime_error("Index file not found");
            }

            BTreePage rightmost_leaf = _find_rightmost_leaf(index_file);
        
            bool is_new_max = false;
            if (rightmost_leaf.header.num_keys == 0) { // empty index, new max
                is_new_max = true;
            } else {
                int max_key = rightmost_leaf.entries[rightmost_leaf.header.num_keys - 1].key;
                is_new_max = (key > max_key);
            }
            
            int value = (row_ptr_page << 16) | row_ptr_slot;
            if (is_new_max) {
                // New max
                SplitResult result = _insert_recursive(0, key, value, index_file);

                if (result.did_split) {
                    // Root split occurred
                    _handle_root_split(result, index_file);
                }
            } else {
                // If exists update it
                _update_existing_key(key, value, index_file);
            }
            
            index_file.close();
        }

        std::pair<int, int> search(int key, const std::string& db_name, 
                    const std::string& table_name) {
            std::string index_file_path = _main_directory + "/" + db_name + "/" + 
                                        table_name + ".idx";
            std::fstream index_file(index_file_path, std::ios::in | std::ios::binary);
            
            if (!index_file.is_open()) {
                throw std::runtime_error("Index file not found: " + index_file_path);
            }
            
            BTreePage page;
            int page_id = 0;  // Start at root
            
            while (true) {
                _retrieve_page(index_file, page_id, page);

                for (int i = 0; i < page.header.num_keys; i++) {
                    if (page.entries[i].key == key) {
                        int value = page.entries[i].value;
                        int row_page = value >> 16;
                        int row_slot = value & 0xFFFF;
                        index_file.close();
                        return {row_page, row_slot};
                    }
                }
                
                // Not found in current node
                if (page.header.is_leaf) {
                    // at a leaf and didnt find it - doesnt exist
                    index_file.close();
                    return {-1, -1};
                }
                
                // Internal node - navigate to appropriate child
                int next_page_id = page.header.rightmost_child;  // Default to rightmost
                
                for (int i = 0; i < page.header.num_keys; i++) {
                    if (key < page.entries[i].key) {
                        next_page_id = page.entries[i].child_id;
                        break;
                    }
                }
                
                page_id = next_page_id;
            }
        }

        void create_index_file(const std::string& db_name, const std::string& table_name) {
            std::string index_file_path = _main_directory + "/" + db_name + "/" + table_name + ".idx";
            
            std::ofstream index_file(index_file_path, std::ios::binary);
            
            if (!index_file.is_open()) {
                throw std::runtime_error("Could not create index file: " + index_file_path);
            }
        
            int max_page_id = 0;
            index_file.write(reinterpret_cast<const char*>(&max_page_id), sizeof(int));
            
            BTreePage root;
            root.header.page_id = 0;
            root.header.is_leaf = 1;  // Initially a leaf
            root.header.num_keys = 0;
            root.header.parent_page_id = -1;  // No parent
            root.header.rightmost_child = -1;  // Not used for leaf
            
            // Write root header
            index_file.write(reinterpret_cast<const char*>(&root.header), 
                            sizeof(BTreePageHeader));
            
            // Pad to 4KB
            int padding = 4096 - sizeof(BTreePageHeader);
            std::vector<char> zeros(padding, 0);
            index_file.write(zeros.data(), padding);
            
            index_file.close();
        }
};