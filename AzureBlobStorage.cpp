// Acaisoft.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <ctime>
#include <windows.h>
#include <was/storage_account.h>
#include <was/blob.h>
#include <was/table.h>
#include <cpprest/filestream.h>  
#include <cpprest/containerstream.h> 
#include <json/json.h>

using namespace std;
using namespace utility::conversions;

// Blob Container
const string container_name{ "kopiec-krecika" };
string accName { "default" };// mich90
string accKey { "default" };// "O2pAHx492uXwWP66VxJSWdnGpcuZFH33Xemn0mJxSiIi3J+AJa9gc4r3zj9BHh04VdnoVTDo6MtOaeXg5B4vAQ=="

string jsonName{ "data.json" };
azure::storage::cloud_blob_container container;

// Log Table
const utility::string_t storage_connection_string_table(U("DefaultEndpointsProtocol=https;AccountName=mich90;AccountKey=r8lcHWsdBqdPzsw1zAQOWga713kCAB9lagi9zrSHRNsqSr1Pzw66yrWUP0JmbJ9pmpj22zO3q7lK7QXhyCWTIw==;TableEndpoint=https://mich90.table.cosmos.azure.com:443/;"));
string tableKey;
utility::string_t logsTab = to_string_t("log4Aca");

long GetFileSize(std::string filename)
{
	struct stat stat_buf;
	int rc = stat(filename.c_str(), &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}
void veryfyAcc(string c, azure::storage::cloud_blob_container &container)
{
	const utility::string_t storage_connection_string = utility::conversions::to_string_t(c);
	//azure::storage::cloud_blob_container container;
	try
	{
		// Retrieve storage account from connection string.
		azure::storage::cloud_storage_account storage_account = azure::storage::cloud_storage_account::parse(storage_connection_string);

		// Create the blob client.
		azure::storage::cloud_blob_client blob_client = storage_account.create_cloud_blob_client();

		// Retrieve a reference to a container.
		container = blob_client.get_container_reference(to_string_t(container_name));

		// Create the container if it doesn't already exist.
		container.create_if_not_exists();
	}
	catch (const std::exception& e)
	{
		std::wcout << U("Error: ") << e.what() << std::endl;
	}
	
}
void storeCredentialToFile()
{
	//Load acc data to file
	std::ofstream jsonFile(jsonName, std::ifstream::binary);
	Json::Value event;
	event["accName"] = accName;
	event["accKey"] = accKey;
	jsonFile << event;	
	jsonFile.close();

}
void LoadCredentialsFromFile()
{
	ifstream ifile(jsonName);
	Json::CharReaderBuilder  reader;
	Json::Value root;
	string errs;
	Json::parseFromStream(reader, ifile, &root, &errs);
	//cout << root << endl;;
	accName = root["accName"].asCString();
	accKey = root["accKey"].asCString();
	ifile.close();
}
void logIt(string fileName, string operation)
{
	// Retrieve the storage account from the connection string.
	azure::storage::cloud_storage_account storage_account_table = azure::storage::cloud_storage_account::parse(storage_connection_string_table);
	// Create the table client.
	azure::storage::cloud_table_client table_client = storage_account_table.create_cloud_table_client();
	

	// Retrieve a reference to a table.
	azure::storage::cloud_table table = table_client.get_table_reference(logsTab);
	// Create the table if it doesn't exist.
	table.create_if_not_exists();

	time_t rawtime;
	time(&rawtime);

	azure::storage::table_entity record(to_string_t((fileName)), to_string_t((ctime(&rawtime))));
	azure::storage::table_entity::properties_type& properties = record.properties();
	properties.reserve(2);
	properties[U("FileSize")] = azure::storage::entity_property(to_string_t(to_string(GetFileSize(fileName))));
	properties[U("Operation")] = azure::storage::entity_property(to_string_t(operation));

	// Create the table operation that inserts the customer entity.
	azure::storage::table_operation insert_operation = azure::storage::table_operation::insert_entity(record);

	// Execute the insert operation.
	azure::storage::table_result insert_result = table.execute(insert_operation);
	

}
bool isJsonFileExist() 
{
	ifstream s(jsonName.c_str());
	if (s.is_open())
	{
		return true;
	}
	return false;
}
int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		isJsonFileExist() ? LoadCredentialsFromFile() : storeCredentialToFile();

		for (int i = 1; i < argc; ++i)
		{
			std::string tmp (argv[i]);
			if (tmp == "--account_name" || tmp == "-an")
			{
				if (argc > i + 1)
				{
					accName = static_cast<string>(argv[++i]);
					cout << "Account Name = " << accName << endl;
					storeCredentialToFile();
				}
				else
				{
					std::cerr << "--account_name option requires one argument." << std::endl;
					return 0;
				}
			}
			if (tmp == "--account_key" || tmp == "-ak")
			{
				if (argc > i + 1)
				{
					accKey = static_cast<string>(argv[++i]);
					cout << "Account Key = " << accKey << endl;
					storeCredentialToFile();
				}
				else
				{
					std::cerr << "--account_key option requires one argument." << std::endl;
					return 0;
				}
			}
		
			string connectionString = "DefaultEndpointsProtocol=https;AccountName=" + accName + ";AccountKey=" + accKey + ";EndpointSuffix=core.windows.net";
			const utility::string_t storage_connection_string = to_string_t(connectionString);

			veryfyAcc(connectionString, container); //connect to container & verifie
			
			if (tmp == "--add")
			{
				if (argc > i + 1)
				{
					string fileName = static_cast<string>(argv[++i]);
					cout << "Copying " << fileName << " to Azure service ...";

					utility::string_t myBlob = to_string_t(string("Blobof" + fileName));

					try
					{
						// Retrieve reference to a blob named "my-blob-1".
						azure::storage::cloud_block_blob blockBlob = container.get_block_blob_reference(to_string_t(myBlob));
						// Create or overwrite the "my-blob-1" blob with contents from a local file.
						concurrency::streams::istream input_stream = concurrency::streams::file_stream<uint8_t>::open_istream(to_string_t(fileName)).get();
						blockBlob.upload_from_stream(input_stream);
						input_stream.close().wait();
						logIt(fileName, "Add");
						cout << "OK" << endl;
					}
					catch (std::exception& ex)
					{
						cout << "Failed " << ex.what() << endl;
					}
				}
				else
				{
					std::cerr << "--add option requires one argument." << std::endl;
					return 0;
				}
			}
			if (tmp == "--delete")
			{
				if (argc > i + 1)
				{
					string fileName = static_cast<string>(argv[++i]);
					cout << "Delete " << fileName << " to Azure service ...";

					utility::string_t myBlob = to_string_t(string("Blobof" + fileName));

					try
					{
						// Retrieve reference to a blob named "my-blob-1".
						azure::storage::cloud_block_blob blockBlob = container.get_block_blob_reference(to_string_t(myBlob));

						// Delete the blob.
						blockBlob.delete_blob();
						logIt(fileName, "Delete");

						cout << "OK" << endl;
					}
					catch (std::exception& ex)
					{
						cout << "Failed " << ex.what() << endl;
					}
				}
				else
				{
					std::cerr << "--delete option requires one argument." << std::endl;
					return 0;
				}
			}

			if (tmp == "--logs")
			{
				// Retrieve the storage account from the connection string.
				azure::storage::cloud_storage_account storage_account_tab = azure::storage::cloud_storage_account::parse(storage_connection_string_table);

				// Create the table client.
				azure::storage::cloud_table_client table_client = storage_account_tab.create_cloud_table_client();

				// Create a cloud table object for the table.
				azure::storage::cloud_table table = table_client.get_table_reference(to_string_t(logsTab));

				// Define the query, and select only the Email property.
				azure::storage::table_query query;
				std::vector<utility::string_t> columns;

				columns.push_back(to_string_t("FileSize"));
				columns.push_back(to_string_t("Operation"));

				query.set_select_columns(columns);

				// Execute the query.
				azure::storage::table_query_iterator it = table.execute_query(query);

				// Display the results.
				azure::storage::table_query_iterator end_of_results;

				for (; it != end_of_results; ++it)
				{
					std::wcout << U("FileName: ") << it->partition_key() << U(", Time: ") << it->row_key();

					const azure::storage::table_entity::properties_type& properties = it->properties();
					for (auto prop_it = properties.begin(); prop_it != properties.end(); ++prop_it)
					{
						std::wcout << ", " << prop_it->first << ": " << prop_it->second.str()<<endl;
					}

					std::wcout << std::endl;
				}
			}

		}
	}
	


		return 0;
}
