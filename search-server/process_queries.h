#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <execution>
#include <iterator>

#include "search_server.h"


/* набор объектов Document */
std::vector<Document>ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);



std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries);
