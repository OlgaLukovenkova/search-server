#include "remove_duplicates.h"
#include <iostream>

using namespace std; 

set<string_view> ExtractKeys(const map<string_view, double>& words) { // w * O(log w)
    set<string_view> keys;
    for (const auto& [word, _] : words) { //w
        keys.insert(word); // O(log w)
    }
    return keys;
}

void RemoveDuplicates(SearchServer& search_server) {
    set<set<string_view>> unique_docs;
    vector<int> docs_for_remove;
    docs_for_remove.reserve(search_server.GetDocumentCount());
    for (const int id : search_server) { //N
        set<string_view> words = ExtractKeys(search_server.GetWordFrequencies(id)); //O(log N) get freqs + w*O(log w) extract
        if (unique_docs.find(words) == unique_docs.end()) { // O(log N * w * l)
            unique_docs.insert(words); // O(log N)
        }
        else {
            docs_for_remove.push_back(id); // O(1)
        }
    }

    for (const int id : docs_for_remove) { // N
        search_server.RemoveDocument(id); // w * (O(log W) + O(log N))
        cout << "Found duplicate document id "s << id << endl;
    }
}
