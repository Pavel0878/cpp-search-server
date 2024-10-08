#include "document.h"

using namespace std;
using namespace std::string_literals;

Document::Document(int id, double relevance, int rating)
    : id(id)
    , relevance(relevance)
    , rating(rating) {
}


std::ostream& operator<<(std::ostream& output, const Document& document) {
    return output << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
}