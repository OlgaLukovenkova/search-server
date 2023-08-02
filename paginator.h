#pragma once
#include <iostream>
#include <vector>

//Class Page = 2 iterators [begin_; end_) of Documents
template <typename Iterator>
class Page {
public:
    Page(Iterator begin, Iterator end);

    auto begin() const;

    auto end() const;

    //return page size (docs)
    size_t size() const;

private:
    Iterator begin_;
    Iterator end_;
};

template <typename Iterator>
class Paginator
{
public:
    //construct vector of pages for search results
    Paginator(Iterator begin, Iterator end, int page_size);

    auto begin() const;

    auto end() const;

private:
    std::vector<Page<Iterator>> pages_;
};

template <typename Iterator>
Page<Iterator>::Page(Iterator begin, Iterator end) :
    begin_(begin), end_(end) {

}

template <typename Iterator>
auto Page<Iterator>::begin() const {
    return begin_;
}

template <typename Iterator>
auto Page<Iterator>::end() const {
    return end_;
}

template <typename Iterator>
size_t Page<Iterator>::size() const {
    return std::distance(begin_, end_);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const Page<Iterator>& page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        os << *it;
    }
    return os;
}

template<typename Iterator>
Paginator<Iterator>::Paginator(Iterator begin, Iterator end, int page_size) {
    int page_count = distance(begin, end) / page_size;

    Iterator last = begin;
    for (int p = 0; p < page_count; ++p) {
        Iterator page_begin = last;

        Iterator page_end = last;
        std::advance(page_end, page_size);

        Page page(page_begin, page_end);
        pages_.push_back(page);

        last = page_end;
    }

    if (last != end) {
        Page page(last, end);
        pages_.push_back(page);
    }
}

template<typename Iterator>
auto Paginator<Iterator>::begin() const {
    return pages_.begin();
}

template<typename Iterator>
auto Paginator<Iterator>::end() const {
    return pages_.end();
}

//function created Paginator for search results
template <typename Container>
auto Paginate(const Container& c, int page_size) {
    return Paginator(begin(c), end(c), page_size);
}

