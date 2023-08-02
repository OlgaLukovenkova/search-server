#include "document.h"

using namespace std;

Document::Document(int id_, double relevance_, int rating_) :
    id(id_), relevance(relevance_), rating(rating_)
{

}

std::ostream& operator<<(std::ostream& os, const Document& document) {
    os << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return os;
}