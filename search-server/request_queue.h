#pragma once

#include "search_server.h"

#include <deque>
#include <vector>
#include <string>
#include <iterator>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult
    {
        int time_ = 0; //время
        int number_requests = 0; // количество запросов
        int empty_queries = 0; // пустые запросы
    };
    QueryResult result_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server;
};