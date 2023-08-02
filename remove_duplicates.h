#pragma once
#include <set>
#include <map>
#include "search_server.h"

std::set<std::string_view> ExtractKeys(const std::map<std::string_view, double>& words);
void RemoveDuplicates(SearchServer& search_server);

