#include "process_queries.h"
#include <execution>

using namespace std;

vector<vector<Document>> ProcessQueries( const SearchServer& search_server, const vector<string>& queries ) {
	vector<vector<Document>> res;
	res.resize(queries.size());

	transform(execution::par, queries.begin(), queries.end(), res.begin(), [&search_server](const string& query) {
		return search_server.FindTopDocuments(query);
		});
	return res;
}

list<Document> ProcessQueriesJoined(const SearchServer& search_server, const vector<string>& queries) {
	list<Document> res;

	auto arr = ProcessQueries(search_server, queries);

	for (const auto& el : arr) {
		res.insert(res.end(), el.begin(), el.end());
	}

	return res;
}
