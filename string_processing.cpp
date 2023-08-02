#include "string_processing.h"

using namespace std;
vector<string_view> SplitIntoWords(string_view text) {
    vector<string_view> words;

    auto not_space = text.find_first_not_of(" "s);
    text.remove_prefix(min(text.size(), not_space));

    while (!text.empty()) {
        auto space = text.find_first_of(" "s);
        words.push_back(text.substr(0, min(text.size(), space)));
        auto not_space = text.find_first_not_of(" "s, space);
        text.remove_prefix(min(text.size(), not_space));
    }

    //OTHER VERSION
    /*size_t index = 0;
    size_t start = 0;
    size_t len = 0;
    for (const char c : text) {
        if (c == ' ') {
            if (len != 0) {
                words.push_back(text.substr(start, len));
                len = 0;
            }
            start = index + 1;
        }
        else {
            ++ len;
        }
        ++ index;
    }
    if (len != 0) {
        words.push_back(text.substr(start, len));
    }*/

    return words;
}