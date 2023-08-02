#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) :
    server_(search_server), time_(0), empty_requests_(0) {

}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    auto request = server_.FindTopDocuments(raw_query, status);
    NewRequest(request);
    return request;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    auto request = server_.FindTopDocuments(raw_query);
    NewRequest(request);
    return request;
}

size_t RequestQueue::GetNoResultRequests() const {
    return empty_requests_;
}

void RequestQueue::NewRequest(vector<Document> request) {
    ++time_;
    while (!requests_.empty() && min_in_day_ <= time_ - requests_.front().request_time) {
        if (requests_.front().empty) {
            --empty_requests_;
        }
        requests_.pop_front();
    }
    bool isEmpty = request.empty();
    if (isEmpty) {
        ++empty_requests_;
    }
    requests_.push_back({ time_, isEmpty });
}
