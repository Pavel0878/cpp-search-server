#include "request_queue.h"

using namespace std;

     RequestQueue::RequestQueue(const SearchServer& search_server)
        :search_server(search_server)
    {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        auto documents_matched = search_server.FindTopDocuments(raw_query, document_predicate);
        if (documents_matched.empty()) { ++result_.empty_queries; }
        ++result_.time_;
        ++result_.number_requests;
        if (result_.time_ > min_in_day_) {
            while (result_.time_ != min_in_day_) {
                --result_.empty_queries;
                --result_.number_requests;
                --result_.time_;
            }
        }

        return documents_matched;
    }
    vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
        return AddFindRequest(
            raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
                return document_status == status;
            });
    }
    vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int RequestQueue::GetNoResultRequests() const {
        return result_.empty_queries;
    }
