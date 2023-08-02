#include "test_example_functions.h"

using namespace std;

void Assert(bool expr, const string& expr_str, const string& file, unsigned line, const string& func, const string& hint) {
    if (!expr) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void TestAddingNewDocument() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server(""s);
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 0, "There must be 0 documents before adding"s);

    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_EQUAL_HINT(server.GetDocumentCount(), 1, "There must be 1 document after adding"s);
}

// check simple search TOP documents (test sorting and MAX_RESULT_DOCUMENT_COUNT = 5)
void TestSearchDocument() {
    SearchServer server(""s);
    ASSERT_HINT(server.FindTopDocuments("cat city blue eyes"s).empty(), "There are no documents"s);

    {
        server.AddDocument(2, "big city life"s, DocumentStatus::ACTUAL, { 1,2,3 });
        const auto found_docs = server.FindTopDocuments("cat city blue eyes"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1, "1 document corresponds to query"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 2, "Wrong document is found"s);
    }
    {
        server.AddDocument(1, "white cat in the city"s, DocumentStatus::ACTUAL, { 1,2,3 });
        const auto found_docs = server.FindTopDocuments("cat city blue eyes"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 2, "2 documents correspond to query"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 1, "Sorting order is wrong (by relevance)"s);
    }
    {
        server.AddDocument(3, "kind dog"s, DocumentStatus::ACTUAL, { 1,2,3 });
        const auto found_docs = server.FindTopDocuments("cat city blue eyes"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 2, "2 documents correspond to query"s);
    }
    {
        server.AddDocument(4, "kind cat with big blue eyes"s, DocumentStatus::ACTUAL, { 1,2,3 });
        const auto found_docs = server.FindTopDocuments("cat city blue eyes"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 3, "3 documents correspond to query"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 4, "Sorting order is wrong (by relevance)"s);
    }
    {
        server.AddDocument(5, "grey cat with big blue eyes"s, DocumentStatus::ACTUAL, { 1,2 });
        const auto found_docs = server.FindTopDocuments("cat city blue eyes"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 4, "4 documents correspond to query"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 4, "Sorting order is wrong (by rating)"s);
    }
    {
        server.AddDocument(6, "cat with eyes"s, DocumentStatus::ACTUAL, { 1,2 });
        server.AddDocument(7, "cat"s, DocumentStatus::ACTUAL, { 1,2 });
        const auto found_docs = server.FindTopDocuments("cat city blue eyes"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 5, "6 documents correspond to query, but TOP = 5"s);
    }
}

// check rating calculaion
void TestRatingCalc() {
    SearchServer server(""s);
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
    ASSERT_EQUAL_HINT(server.FindTopDocuments("cat"s)[0].rating, (1 + 1 + 2) / 3, "Rating calculation is wrong"s);
    server.AddDocument(2, "grey black red cat"s, DocumentStatus::ACTUAL, { });
    ASSERT_EQUAL_HINT(server.FindTopDocuments("cat"s)[1].rating, 0, "Rating calculation is wrong (empty rating case)"s);
}

// check relevance calculaion
void TestRelevanceCalc() {
    SearchServer server(""s);
    server.AddDocument(1, "cat and cat in the city"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
    server.AddDocument(2, "red cat"s, DocumentStatus::ACTUAL, { });
    const auto found_docs = server.FindTopDocuments("cat in city"s);
    ASSERT_HINT(abs(found_docs[0].relevance - 0.231049) < 1e-6, "Relevance calculation is wrong"s);
    ASSERT_HINT(abs(found_docs[1].relevance - 0.0) < 1e-6, "Relevance calculation is wrong"s);
}

// check excluding stop words
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server(""s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL_HINT(found_docs.size(), 1, "1 documet corresponds to query"s);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL_HINT(doc0.id, doc_id, "Wrong document is found"s);
    }
    {
        SearchServer server("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop-words are not excluded"s);
    }
}

// check excluding documents with minus words
void TestExcludeDocsWithMinusWords() {
    SearchServer server(""s);
    server.AddDocument(1, "cat and cat in the city"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
    server.AddDocument(2, "red cat"s, DocumentStatus::ACTUAL, { });
    const auto found_docs = server.FindTopDocuments("cat city red -red"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "1 document corresponds to query"s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 1, "Wrong document is found"s);
}

// check search for matching words
void TestMatchingWords() {
    SearchServer server(""s);
    
    server.AddDocument(1, "cat and cat in the city"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
    string s = "cat in city"s;
    auto res = server.MatchDocument(s, 1);
    ASSERT_EQUAL_HINT(get<0>(res).size(), 3, "3 words must be matched"s);
    vector<string_view> words = { "cat"sv, "city"sv, "in"sv };
    ASSERT_EQUAL_HINT(get<0>(res), words, "Wrong words are matched"s);
    res = server.MatchDocument("cat in -city"s, 1);
    ASSERT_HINT(get<0>(res).empty(), "No words are matched"s);
}

// check search TOP documents (test search by predicate and search for selected status)
void TestComplexSearchDocument() {
    SearchServer server(""s);
    server.AddDocument(1, "cat and cat in the city"s, DocumentStatus::ACTUAL, { 1, 1, 2 });
    server.AddDocument(2, "grey cat and dog"s, DocumentStatus::BANNED, { 1, 2 });
    server.AddDocument(3, "big city life"s, DocumentStatus::ACTUAL, { 1, 0, 2 });
    server.AddDocument(4, "cat and dog"s, DocumentStatus::BANNED, { 3, 2 });

    {
        const auto found_docs = server.FindTopDocuments("cat city"s, DocumentStatus::BANNED);
        ASSERT_EQUAL_HINT(found_docs.size(), 2, "2 documents correspond to query (with selected STATUS)"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 4, "Sorting order is wrong or wrong documents are found (with selected STATUS)"s);
    }
    {
        auto found_docs = server.FindTopDocuments("cat city"s,
            [](int document_id, DocumentStatus status, int rating) {
                return document_id % 2 == 0;
            }
        );
        ASSERT_EQUAL_HINT(found_docs.size(), 2, "2 documents correspond to query (by predicate)"s);
        ASSERT_EQUAL_HINT(found_docs[0].id, 4, "Sorting order is wrong or wrong documents are found (by predicate)"s);
    }
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestAddingNewDocument);
    RUN_TEST(TestSearchDocument);
    RUN_TEST(TestRatingCalc);
    RUN_TEST(TestRelevanceCalc);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocsWithMinusWords);
    RUN_TEST(TestMatchingWords);
    RUN_TEST(TestComplexSearchDocument);
}
