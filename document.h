#pragma once
#include <iostream>

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

// descriptor of document
struct Document {
    int id;
    double relevance;
    int rating;

    Document(int id_ = 0, double relevance_ = 0.0, int rating_ = 0);
};

std::ostream& operator<<(std::ostream& os, const Document& document);
