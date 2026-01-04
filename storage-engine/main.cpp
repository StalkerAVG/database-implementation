#include <iostream>
#include "storage_engine.hpp"
#include <string>
#include "table_manager.hpp"

int main(){
    // StorageEngine obj;
    // obj.create_db("test");

	// Column col1("col1", ID, true);
	// Column col2("col2", BIGTEXT);
	// Column col3("col3", FLOAT);
	
	// obj.create_table("test", "table5", {col1, col2}); 
	// obj.drop_table("test", "table3");
	
	TableManager tblmgr;

	tblmgr.add_record("test", "table5", {"3", "Its me Mario"});
}