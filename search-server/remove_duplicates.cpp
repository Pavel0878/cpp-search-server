#include "remove_duplicates.h"


using namespace std;


void RemoveDuplicates(SearchServer& search_server) {
	set<int> dublicat;
    int num_doc = search_server.GetDocumentCount();
    for (const int document_id : search_server) {
        if (document_id < num_doc || !dublicat.empty()) {
            map<string, double> word_dublicat = search_server.GetWordFrequencies(document_id);
            for (const int doc_id : search_server) {
                if (document_id < doc_id) {
                    map<string, double> word_dublicat1 = search_server.GetWordFrequencies(doc_id);
                    int siz = word_dublicat1.size();
                    int num = 0;
                   for (auto [str, _] : word_dublicat) {
                       word_dublicat1.erase(str);
                        if (word_dublicat.count(str) == 1) {
                            ++num;
                        }
                    }
                    if (num == siz && word_dublicat1.empty()) { dublicat.insert(doc_id); }
                }
            }
        }
    }
    for (const int id : dublicat) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
    }
}
