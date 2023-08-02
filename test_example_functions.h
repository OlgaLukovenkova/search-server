#pragma once
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <map>
#include "search_server.h"

using namespace std::string_literals;

//check expr == true
void Assert(bool expr, const std::string& expr_str, const std::string& file, unsigned line, const std::string& func, const std::string& hint = ""s);

#define ASSERT(expr) Assert((expr), #expr, __FILE__, __LINE__, __FUNCTION__)
#define ASSERT_HINT(expr, hint) Assert((expr), #expr, __FILE__, __LINE__, __FUNCTION__, (hint))

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec) {
    bool is_not_first = false;
    os << "["s;
    for (const T& element : vec) {
        os << (is_not_first ? ", "s : ""s) << element;
        is_not_first = true;
    }
    os << "]"s;
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    bool is_not_first = false;
    os << "{"s;
    for (const T& element : s) {
        os << (is_not_first ? ", "s : ""s) << element;
        is_not_first = true;
    }
    os << "}"s;
    return os;
}

template <typename T1, typename T2>
std::ostream& operator<<(std::ostream& os, const std::map<T1, T2>& m) {
    bool is_not_first = false;
    os << "{"s;
    for (const auto& [key, value] : m) {
        os << (is_not_first ? ", "s : ""s) << key << ": "s << value;
        is_not_first = true;
    }
    os << "}"s;
    return os;
}

//check t == u
template <typename T, typename U>
void AssertEqual(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
    const std::string& func, unsigned line, const std::string& hint = ""s) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqual((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqual((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

//print information about passed tests
template <typename Test>
void RunTestImpl(Test test_func, const std::string& test_func_name) {
    test_func();
    std::cerr << test_func_name << " OK"s << std::endl;

}

#define RUN_TEST(func)  RunTestImpl((func), #func)

// -------- Начало модульных тестов поисковой системы ----------

// check adding new documents to server
void TestAddingNewDocument();

// check simple search TOP documents (test sorting and MAX_RESULT_DOCUMENT_COUNT = 5)
void TestSearchDocument();

// check rating calculaion
void TestRatingCalc();

// check relevance calculaion
void TestRelevanceCalc();

// check excluding stop words
void TestExcludeStopWordsFromAddedDocumentContent();

// check excluding documents with minus words
void TestExcludeDocsWithMinusWords();

// check search for matching words
void TestMatchingWords();

// check search TOP documents (test search by predicate and search for selected status)
void TestComplexSearchDocument();

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer();


