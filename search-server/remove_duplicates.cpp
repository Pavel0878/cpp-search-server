#include "remove_duplicates.h"


using namespace std;


void RemoveDuplicates(SearchServer& search_server) {
	set<int> dublicat;
    map<set<string>,int> dublicat_id;
    for (const int document_id : search_server) {
        map<string, double> word_dublicat = search_server.GetWordFrequencies(document_id);
        set<string> dubl;
        for (auto str : word_dublicat) {
            dubl.insert(str.first);
        }
        if (dublicat_id.count(dubl)) {
            dublicat.insert(document_id);
        }
        else {
            dublicat_id[dubl] = document_id ;
        }
    }
    
    for (const int id : dublicat) {
        search_server.RemoveDocument(id);
        cout << "Found duplicate document id "s << id << endl;
    }
}
