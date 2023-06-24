
#include "string_processing.h"

//#include <cassert>

using namespace std;

vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> words;
    text.remove_prefix(min(text.find_first_not_of(" "), text.size()));
    while (!text.empty()) {
        size_t space = text.find(" ");
        int64_t pos = 0;
        words.push_back(text.substr(pos, space));
        text.remove_prefix(min(text.size(), space));
        text.remove_prefix(min(text.find_first_not_of(" "), text.size()));
    }
    return words;
}
