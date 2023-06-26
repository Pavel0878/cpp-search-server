#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status,
    const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string_view& word : words) {
        string_.push_back(string{ word });
        id_doc_[document_id].insert(string_.at(string_.size() - 1));
        word_to_document_freqs_[string_.at(string_.size() - 1)][document_id] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}


vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}


int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const map<string_view, double> word_freqs_empty = {};
    if (!document_ids_.count(document_id)) { return word_freqs_empty; }
    static map<string_view, double> word_freqs;
    word_freqs.clear();
    for (auto str : id_doc_.at(document_id)) {
        word_freqs[str] = word_to_document_freqs_.at(str).at(document_id);
    }
    return word_freqs;
}

void SearchServer::RemoveDocument(int document_id) {
    if (!document_ids_.count(document_id)) { return; }
    for (const string_view& word : id_doc_.at(document_id)) {
        word_to_document_freqs_[word].erase(document_id);
    }
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    id_doc_.erase(document_id);
}

void SearchServer::RemoveDocument(const execution::sequenced_policy& seq, int document_id) {
    if (!document_ids_.count(document_id)) { return; }
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const execution::parallel_policy& par, int document_id) {
    if (!document_ids_.count(document_id)) { return; }
    vector<string_view> words_erase;
    auto tmp = id_doc_.at(document_id);
    words_erase.resize(tmp.size());
    transform(par, tmp.begin(), tmp.end(), words_erase.begin(),
        [](auto str) {return str; });
    for_each(par, words_erase.begin(), words_erase.end(),
        [this, document_id](auto& st) {word_to_document_freqs_[st].erase(document_id); });

    documents_.erase(document_id);
    document_ids_.erase(document_id);
    id_doc_.erase(document_id);
}


set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query, true);

    vector<string_view> matched_words;

    for (const string_view& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            return { matched_words, documents_.at(document_id).status };
        }
    }

    for (const string_view& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return { matched_words, documents_.at(document_id).status };
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::sequenced_policy& seq,
    const string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const execution::parallel_policy& par,
    const string_view raw_query, int document_id) const {

    if (raw_query.empty()) { throw invalid_argument("MatchDocument is empty"s); }
    if (!document_ids_.count(document_id)) { throw out_of_range("Matchdocument Invalid document_id"s); }

    const auto query = ParseQuery(raw_query, false);
    vector<string_view> matched_words;

    if (any_of(par, query.minus_words.begin(), query.minus_words.end(), [this, document_id](auto& word) {
        return (word_to_document_freqs_.count(word) != 0 &&
            word_to_document_freqs_.at(word).count(document_id));
        })) {
        return { matched_words, documents_.at(document_id).status };
    }

    matched_words.resize(query.plus_words.size());


    auto dis =
        copy_if(par, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
            [this, document_id](auto& word) {
                return (word_to_document_freqs_.count(word)
                    && word_to_document_freqs_.at(word).count(document_id)); });

    matched_words.erase(dis, matched_words.end());
    sort(matched_words.begin(), matched_words.end());
    matched_words.erase(unique(matched_words.begin(), matched_words.end()), matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string{word} + " is invalid"s);
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
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string{text} + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const string_view text, bool sortir) const {
    Query result;
    for (const string_view& word : SplitIntoWords(text)) {
        auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (sortir) {
        sort(result.minus_words.begin(), result.minus_words.end());
        result.minus_words.erase(std::unique(result.minus_words.begin(), result.minus_words.end())
            , result.minus_words.end());
        sort(result.plus_words.begin(), result.plus_words.end());
        result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end())
            , result.plus_words.end());
    }
    return result;

}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
