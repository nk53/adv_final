#include <boost/filesystem.hpp>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace boost::filesystem;

// holds file contents and full MIME type
//typedef std::tuple<std::string const &, char const *> const server_contents;
// holds file contents and char shorthand for a MIME type
//typedef std::tuple<std::string const, char const> const cache_contents;

char const MIME_HTML = 0;
char const MIME_JS   = 1;
char const MIME_CSS  = 2;

//constexpr char const * MIME_MAP[] = 
//   {"text/html; charset=utf-8",
//    "text/javascript; charset=utf-8",
//    "text/css; charset=utf-8"};

/**
 * For now, we only need to remember one page: the index
 */
class Cache {
private:
    path document_root;
    std::map<std::string, std::string> cache;
public:
    Cache(path &document_root);
    std::string const &fetch_index();
    std::string &operator[](std::string const &key);
    std::map<std::string, std::string> mime_map;
};
