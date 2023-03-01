
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

/* Подставьте вашу реализацию класса SearchServer сюда */

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
    int rating;
};
enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
        // Ваша реализация данного метода
    }
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        // Ваша реализация данного метода
    }
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        //DocumentStatus status = DocumentStatus::ACTUAL) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);
        constexpr double eps = (1e-6);
        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < eps) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
        // Ваша реализация данного метода
    }
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_ = DocumentStatus::ACTUAL) const {
        return FindTopDocuments(raw_query, [status_](int document_id, DocumentStatus status, int rating) { return status == status_; });
        // Ваша реализация данного метода
    }

    //vector<Document> FindTopDocuments(const string& raw_query) const {
        //return FindTopDocuments(raw_query, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; });
        // Ваша реализация данного метода
    //}
    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
        // Ваша реализация данного метода
    }
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
        // Ваша реализация данного метода
    }
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
        //int rating_sum = 0;
        //for (const int rating : ratings) {
            //rating_sum += rating;
        //}
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename Predicate>

    vector<Document> FindAllDocuments(const Query& query, const Predicate& predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                    //if (documents_.at(document_id).status == status) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
    // Реализация приватных методов вашей поисковой системы
};



/*
   Подставьте сюда вашу реализацию макросов
   ASSERT, ASSERT_EQUAL, ASSERT_EQUAL_HINT, ASSERT_HINT и RUN_TEST
*/

template <typename Key, typename Value>

ostream& operator<<(ostream& out, const pair<Key, Value>& container) {
    //out << '(';
    out << container.first << ": "s << container.second;
    //out << ')';
    return out;
}


template < typename Container>

void Print(ostream& out, const Container& container) {
    bool first = false;
    for (const auto& element : container) {
        if (first) {
            out << ", "s;
        }
        first = true;
        out << element;
    }
}

template <typename Key, typename Value>

ostream& operator<<(ostream& out, const map<Key, Value>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template <typename Element>

ostream& operator<<(ostream& out, const vector<Element>& container) {
    out << '[';
    Print(out, container);
    out << ']';
    return out;
}

template <typename Element>

ostream& operator<<(ostream& out, const set<Element>& container) {
    out << '{';
    Print(out, container);
    out << '}';
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
    const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
    const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))


template <typename T>
void RunTestImpl(T& t, const string& func) { /* Напишите недостающий код */
    t();
    cerr << func << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func) // напишите недостающий код

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов

/*
Разместите код остальных тестов здесь
*/
void TestAddingDocuments() {
    const int doc_id = 42;
    const string content = "пушистый кот пушистый хвост"s;
    const vector<int> ratings = { 7, 2, 7 };
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    const auto found_docs = server.FindTopDocuments("хвост"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, doc_id);
    //assert(found_docs.size() == 1);
    //const Document& doc0 = found_docs[0];
    //assert(doc0.id == doc_id);
}

void TestStopWordSupport() {
    const int doc_id = 42;
    const string content = "пушистый кот пушистый хвост"s;
    const vector<int> ratings = { 7, 2, 7 };
    SearchServer server;
    server.SetStopWords("кот"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    //assert(server.FindTopDocuments("кот"s).empty());
    ASSERT_HINT(server.FindTopDocuments("кот"s).empty(),
        "Стоп-слова должны быть исключены из документов"s);
}

void TestSupportForNegativeKeywords() {
    const int doc_id = 42;
    const string content = "пушистый кот пушистый хвост"s;
    const vector<int> ratings = { 7, 2, 7 };
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    const auto found_docs = server.FindTopDocuments("-кот пушистый"s);
    //assert(found_docs.size() == 0);
    ASSERT_HINT(found_docs.empty(),
        "Минус-слова должны исключать документы из поиска"s);
}

void TectDocumentMatching() {
    const int doc_id = 42;
    const string content = "пушистый кот пушистый хвост"s;
    const string request = "пушистый кот хвост"s;
    const string request_ = "белый -кот модный"s;
    const vector<int> ratings = { 7, 2, 7 };
    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    const auto [word, status] = server.MatchDocument(request, doc_id);
    //assert(word.size() == 3);
    ASSERT_EQUAL_HINT(word.size(), 3, "Не возвращены все слова из поискового запроса");
    const auto [word_, status_] = server.MatchDocument(request_, 0);
    //assert(word_.empty());
    ASSERT_HINT(word_.empty(), "Не должен возвращать слова из поискового запроса"s);
}

void TestSortingDocumentsByRelevance() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    //assert(found_docs.size() == 2);
    ASSERT_EQUAL_HINT(found_docs.size(), 2, "Не верное колличество документов при проверке на релевантность");
    const Document& doc0 = found_docs[0];
    //assert(doc0.id == 1);
    ASSERT_EQUAL_HINT(doc0.id, 1, "Не верно отсортированы документы по релевантности");
    //const Document& doc0 = found_docs[0];
    //assert(doc0.id == 0);
}

void TestCalculatingTheDocumentRating() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    //server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    //assert(found_docs.size() == 2);
    ASSERT_EQUAL_HINT(found_docs.size(), 2, "Не верное колличество документов при проверке на рейтинг");
    const Document& doc0 = found_docs[0];
    //assert(doc0.id == 0);
    ASSERT_EQUAL_HINT(doc0.id, 0, "Не верно считает рейтинг");
}

void TestFilteringPredicateSearch() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    //server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; });
    //assert(found_docs.size() == 2);
    ASSERT_EQUAL_HINT(found_docs.size(), 2, "Не верное колличество документов при проверке с предикатом");
    const Document& doc0 = found_docs[0];
    //assert(doc0.id == 0);
    ASSERT_EQUAL_HINT(doc0.id, 0, "Не верно обрабатывает предикат");
}

void TestSearchForDocumentsWithTheStatus() {
    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::IRRELEVANT, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);
    //assert(found_docs.size() == 1);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Не верное колличество документов с  заданным статусом");
    const Document& doc0 = found_docs[0];
    //assert(doc0.id == 3);
    ASSERT_EQUAL_HINT(doc0.id, 3, "Не верно обрабатывает статус документов");
}

void TestCorrectCalculationOfRelevance() {
    SearchServer server;
    double IDF1 = log(3.0 / 1);
    double IDF2 = log(3.0 / 1);
    double IDF3 = log(3.0 / 2);
    double FreqsIDF1 = IDF1 * 2 / 4;
    double FreqsIDF2 = IDF2 * 0 / 4;
    double FreqsIDF3 = IDF3 * 1 / 4;
    double relev = FreqsIDF1 + FreqsIDF2 + FreqsIDF3;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    const auto found_docs = server.FindTopDocuments("пушистый ухоженный кот"s);
    //assert(found_docs.size() == 3);
    ASSERT_EQUAL_HINT(found_docs.size(), 3, "Не верное колличество документов при проверке на релевантность");
    const Document& doc0 = found_docs[0];
    //cout << relev << endl;
    //cout << doc0.relevance;
    //assert(doc0.relevance == relev);
    ASSERT_EQUAL_HINT(doc0.relevance, relev, "Не верно считает релевантность");
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddingDocuments); // добавление документов
    RUN_TEST(TestStopWordSupport); //Поддержка стоп-слов
    RUN_TEST(TestSupportForNegativeKeywords); // Поддержка минус-слов
    RUN_TEST(TectDocumentMatching); // Матчинг документов
    RUN_TEST(TestSortingDocumentsByRelevance); // Сортировка документов по релевантности
    RUN_TEST(TestCalculatingTheDocumentRating); // Вычисление рейтинга документов
    RUN_TEST(TestFilteringPredicateSearch);// фильтрация поиска с предикатом
    RUN_TEST(TestSearchForDocumentsWithTheStatus); // Поиск документов, имеющих статус
    RUN_TEST(TestCorrectCalculationOfRelevance);  // Корректное вычисление релевантности
    //Не забудьте вызывать остальные тесты здесь
}

//search_server.SetStopWords("и в на"s);
    //server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    //server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    //server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    //server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
//search_server.FindTopDocuments("пушистый ухоженный кот"s)
//server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })
//server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED))


// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}