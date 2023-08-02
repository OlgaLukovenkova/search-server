#include "search_server.h"
#include "string_processing.h"
#include <numeric>
#include <cmath>

using namespace std;

SearchServer::SearchServer(const string& text) {
    for (const auto& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Invalid stop-word (contains symbols from 0 to 31)");
        }
        stop_words_.insert(static_cast<string>(word));
    }
}

SearchServer::SearchServer(string_view text) {
    for (const auto& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Invalid stop-word (contains symbols from 0 to 31)");
        }
        stop_words_.insert(static_cast<string>(word));
    }
}

void SearchServer::AddDocument(int document_id, string_view document, const DocumentStatus& status, const vector<int>& ratings) {
    if (document_id < 0) {
        throw invalid_argument("Document ID is wrong (below 0)");
    }

    if (documents_.count(document_id) > 0) {
        throw invalid_argument("Document ID has already been created");
    }

    DocumentData data{ ComputeAverageRating(ratings), status, static_cast<string>(document) };
    documents_.emplace(document_id, data);
    order_of_adding_.insert(document_id);

    const vector<string_view> words = SplitIntoWordsNoStop(documents_.at(document_id).text);
    const double inv_word_count = 1.0 / words.size();

    for (const auto& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }

    documents_.emplace(document_id, move(data));
    order_of_adding_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, const DocumentStatus& status) const {
    return FindTopDocuments(raw_query,
        [status](const int document_id, const DocumentStatus& local_status, const int rating) {
            return status == local_status;
        });
}

//REMOVE
/*vector<Document>SearchServer::FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}*/

size_t SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, const int document_id) const {
    if (!documents_.count(document_id)) {
        throw out_of_range("Document ID is not found"s);
    }

    Query query = ParseQuery(raw_query);

    RemoveDuplicates(execution::seq, query.minus_words);
    RemoveDuplicates(execution::seq, query.plus_words);

    vector<string_view> matched_words;

    for (const auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { matched_words, documents_.at(document_id).status };
        }
    }

    matched_words.reserve(query.plus_words.size());
    for (const auto& word : query.plus_words) {

        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }        
    }
    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy& policy, string_view raw_query, const int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy& policy, string_view raw_query, const int document_id) const {
    if (!documents_.count(document_id)) {
        throw out_of_range("Document ID is not found"s);
    }

    Query query = ParseQuery(raw_query);

    vector<string_view> matched_words;
    
    auto& words = document_to_word_freqs_.at(document_id);

    if (any_of(policy, query.minus_words.begin(),
        query.minus_words.end(),
        [&words](string_view word) {
            return words.count(word);
        })) {
        return { matched_words, documents_.at(document_id).status };
    }

    matched_words.resize(query.plus_words.size());

    auto last = copy_if(policy, query.plus_words.begin(),
        query.plus_words.end(),
        matched_words.begin(),
        [&words](string_view word) {
            return words.count(word);
        });

    matched_words.erase(last, matched_words.end());
    RemoveDuplicates(policy, matched_words);

    return { matched_words, documents_.at(document_id).status };
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.find(document_id) == document_to_word_freqs_.end()) {
        static const map<string_view, double> empty_map;
        return empty_map;
    }

    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (order_of_adding_.find(document_id) == order_of_adding_.end()) {
        return;
    }

    order_of_adding_.erase(document_id); // O(log N)
    documents_.erase(document_id); // O (log N)
    for (const auto& [word, _] : document_to_word_freqs_.at(document_id)) { // w
        word_to_document_freqs_[word].erase(document_id); // O(log W) search word + O(log N) erase doc
    }
    document_to_word_freqs_.erase(document_id); //O(log N)
}

void SearchServer::RemoveDocument(const execution::sequenced_policy& policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const execution::parallel_policy& policy, int document_id) {
    if (order_of_adding_.find(document_id) == order_of_adding_.end()) {
        return;
    }

    order_of_adding_.erase(document_id); // O(log N)
    documents_.erase(document_id); // O (log N)

    vector<string_view> words;
    words.resize(document_to_word_freqs_.at(document_id).size());
    transform(document_to_word_freqs_.at(document_id).begin(), 
        document_to_word_freqs_.at(document_id).end(), 
        words.begin(), 
        [](std::pair<string_view, double> p) {
            return p.first; 
        });

    std::for_each(policy,
        words.begin(),
        words.end(),
        [&](string_view str) {
            word_to_document_freqs_[str].erase(document_id);
        });

    document_to_word_freqs_.erase(document_id); //O(log N)
}

bool SearchServer::IsValidWord(string_view word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

bool SearchServer::IsStopWord(string_view word) const {
    return stop_words_.count(static_cast<string>(word)) > 0;
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
    vector<string_view> words;
    for (auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Invalid word (contains symbols from 0 to 31)");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
    bool is_minus = false;

    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }

    if (text.empty()) {
        throw invalid_argument("Empty query minus-word");
    }

    if (!IsValidWord(text)) {
        throw invalid_argument("Invalid query word (contains symbols from 0 to 31)");
    }

    if (text[0] == '-') {
        throw invalid_argument("Invalid query minus-word (--)");
    }

    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
    Query query;
    auto words = SplitIntoWords(text);

    for (auto word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}


