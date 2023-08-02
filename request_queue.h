#pragma once
#include <vector>
#include <deque>
#include <string>
#include "search_server.h"
#include "document.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    size_t GetNoResultRequests() const;

private:
    struct QueryResult {
        size_t request_time;
        bool empty;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& server_;
    size_t time_;
    size_t empty_requests_;

    void NewRequest(std::vector<Document> request);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    auto request = server_.FindTopDocuments(raw_query, document_predicate);
    NewRequest(request);
    return request;
}
