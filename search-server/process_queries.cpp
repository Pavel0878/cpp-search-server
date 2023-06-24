#include "process_queries.h"

using namespace std;

/* набор объектов Document */
vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
	vector<Document> document;
	for (auto& doc : ProcessQueries(search_server, queries)) {
		transform(doc.begin(), doc.end(), back_inserter(document),
			[](Document& doc) {
				return doc;
			});
	}
	return document;
}

vector<vector<Document>> ProcessQueries(
	const SearchServer& search_server,
	const vector<string>& queries) {
	vector<vector<Document>> documents_lists(queries.size());
	transform(execution::par, queries.begin(), queries.end(), documents_lists.begin(),
		[&search_server](const string& query) {return search_server.FindTopDocuments(query); });
	return documents_lists;
}