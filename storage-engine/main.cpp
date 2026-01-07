#include <iostream>
#include "storage_engine.hpp"
#include <string>
#include "table_manager.hpp"
#include "record.hpp"

int main(){
    // StorageEngine obj;
    // obj.create_db("test3");

	// Column col1("col1", ID, true);
	// Column col2("col2", SMALLTEXT);
	
	// obj.create_table("test3", "table1", {col1, col2}); 
	// // obj.drop_table("test", "table3");
	
	TableManager tblmgr;

	// tblmgr.add_record("test3", "table1", {"4", "Its me Mario"});
	Record record = tblmgr.retrieve_record("test3", "table1", 1);
	std::cout << record << std::endl; 
	tblmgr.delete_record("test3", "table1", 1);
	Record record2 = tblmgr.retrieve_record("test3", "table1", 1);
	std::cout << record2 << std::endl; 
}