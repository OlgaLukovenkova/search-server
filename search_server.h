#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <set>
#include <map>
#include <string>
#include <stdexcept>
#include <execution>
#include "document.h"
#include "concurrent_map.h"
#include "log_duration.h"

using namespace std::string_literals; //for ""s

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
const int THREAD_COUNT = 16;

template <class ExecutionPolicy>
void RemoveDuplicates(ExecutionPolicy&& policy, std::vector<std::string_view>& vec) {
    sort(policy, vec.begin(), vec.end());
    auto last = unique(policy, vec.begin(), vec.end());
    vec.erase(last, vec.end());
}

class SearchServer {
public:
    //construct SearchServer from string containing stop-words
    explicit SearchServer(const std::string& text);
    explicit SearchServer(std::string_view text);

    //construct SearchServer from container of stop-words (set, vector, etc.)
    template<class Contaner>
    explicit SearchServer(const Contaner& words);

    //adding document in our base
    void AddDocument(int document_id, std::string_view document, const DocumentStatus& status, const std::vector<int>& ratings);

    auto begin() const {
        return order_of_adding_.cbegin();
    }

    auto end() const {
        return order_of_adding_.cend();
    }

    //finding top MAX_RESULT_DOCUMENT_COUNT documents by status - ver. 1
    std::vector<Document>  FindTopDocuments(std::string_view raw_query, const DocumentStatus& status = DocumentStatus::ACTUAL) const;
    template <typename ExecutionPolicy>
    std::vector<Document>  FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, const DocumentStatus& status = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(policy, raw_query,
            [status](const int document_id, const DocumentStatus& local_status, const int rating) {
                return status == local_status;
            });
    }

    //finding top MAX_RESULT_DOCUMENT_COUNT documents with status ACTUAL - ver. 2
    //std::vector<Document> FindTopDocuments(std::string_view raw_query) const; -> merge with ver. 1

    //finding top MAX_RESULT_DOCUMENT_COUNT documents by predicate - ver. 3
    template <typename Predicate>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, Predicate predicate) const;
    template <typename ExecutionPolicy, typename Predicate>
    std::vector<Document> FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, Predicate predicate) const; 

    //getting count of documents in base
    size_t GetDocumentCount() const;

    //search of words matched with query in document with id
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, const int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy, std::string_view raw_query, const int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy, std::string_view raw_query, const int document_id) const; 
    
    //return frequencies of all words in document
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);
    void RemoveDocument(const std::execution::parallel_policy& policy, int document_id);


private:

    struct DocumentData {
        int rating;
        DocumentStatus status;
        std::string text;
    };

    //order of adding documents, keep document ID
    std::set<int> order_of_adding_;

    std::set<std::string, std::less<>> stop_words_;

    // dictionary for calculations: word -> {(id, tf)}
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    
    // dictionary: id -> {(word, tf)}
    std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;

    // base of documents: id -> data
    std::map<int, DocumentData> documents_;

    // does word contain symbols from 0 to 31 ? true/false
    static bool IsValidWord(std::string_view word);

    // is it stop word? true/false
    bool IsStopWord(std::string_view word) const;

    // splitting text into words without stop words
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    // calculating average rating
    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus = false;
        bool is_stop = false;
    };

    // getting query word without '-'
    QueryWord ParseQueryWord(std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    // parsing query into words
    Query ParseQuery(std::string_view text) const; 

    // calculating IDF
    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    // finding ALL documents according to query
    template <typename Predicate>
    std::vector<Document> FindAllDocuments(const Query& query, Predicate predicate) const;

    template <typename ExecutionPolicy, typename Predicate>
    std::vector<Document> FindAllDocuments(const ExecutionPolicy& policy, const Query& query, Predicate predicate) const; 
};


template<class Contaner>
SearchServer::SearchServer(const Contaner& words) {
    for (const auto& word : words) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Invalid stop-word (contains symbols from 0 to 31)");
        }
        stop_words_.insert(static_cast<std::string>(word)); 
    }
}

template <typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, Predicate predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, predicate);
}

template <typename ExecutionPolicy, typename Predicate>
std::vector<Document> SearchServer::FindTopDocuments(const ExecutionPolicy& policy, std::string_view raw_query, Predicate predicate) const {
    Query query = ParseQuery(raw_query);

    RemoveDuplicates(policy, query.minus_words);
    RemoveDuplicates(policy, query.plus_words);

    std::vector<Document> matched_documents = FindAllDocuments(policy, query, predicate);

    std::sort(policy, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            return lhs.relevance > rhs.relevance;
        });

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

template <typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Predicate predicate) const {
    return FindAllDocuments(std::execution::seq, query, predicate);
}

template <typename ExecutionPolicy, typename Predicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExecutionPolicy& policy, const Query& query, Predicate predicate) const {
    if constexpr (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
        std::map<int, double> document_to_relevance;

        for (const auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const DocumentData data = documents_.at(document_id);
                if (predicate(document_id, data.status, data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const auto& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }


        std::vector<Document> matched_documents;
        for (const auto& [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
        }

        return matched_documents;
    }

    ConcurrentMap<int, double> document_to_relevance(THREAD_COUNT);

    auto fn_plus = [&](std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);

        for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const DocumentData data = documents_.at(document_id);
            if (predicate(document_id, data.status, data.rating)) {
                document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
            }
        }
    };

    std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), fn_plus);

    auto fn_minus = [&](std::string_view word) {
        if (word_to_document_freqs_.count(word) == 0) {
            return;
        }
        for (const auto& [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    };

    std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), fn_minus);

    std::vector<Document> matched_documents;
    auto whole = document_to_relevance.BuildOrdinaryMap();
    matched_documents.reserve(whole.size());
    for (const auto& [document_id, relevance] : whole) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }

    return matched_documents;
}



