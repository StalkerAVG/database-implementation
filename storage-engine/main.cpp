#include <iostream>
#include "storage_engine.hpp"
#include <string>
#include "table_manager.hpp"
#include "record.hpp"

int main(){
    // ak::StorageEngine obj;
    // obj.create_db("test3"); // Example of DB creation

	// ak::Column col1("col1", ak::ID, true);
	// ak::Column col2("col2", ak::SMALLTEXT); // Example of column creation with different constructors

	// obj.create_table("test3", "table1", {col1, col2}); // Example of creation
	// obj.drop_table("test", "table3"); // Example of deletion
	
	ak::TableManager tblmgr;

	// tblmgr.add_record("test3", "table1", {"5", "Its me Mario"}); // Example of record insertion
	ak::Record record = tblmgr.retrieve_record("test3", "table1", 5);
	std::cout << record << std::endl; // Example or printig out record
	// tblmgr.delete_record("test3", "table1", 1); // Example of record deletion
	ak::Record record2 = tblmgr.retrieve_record("test3", "table1", 4);
	std::cout << record2 << std::endl; 
}