#include "cache.hpp"

using namespace std;

//server_contents empty_pair = server_contents(string(""), "");
string empty_str = string("");

Cache::Cache(path &document_root) : document_root(document_root) {
    auto index_path = document_root / "index.html";
    
    // maps file extensions to mime types
    mime_map["css"] = "text/css; charset=utf-8";
    mime_map["html"] = "text/html; charset=utf-8";
    mime_map["js"] = "text/javascript; charset=utf-8";

    // ensure root and index actually exist
    if (!exists(document_root)) {
        this->document_root = path();
        cache["index.html"] = string("");
        cerr << "Error: document root does not exist!" << endl;
    } else if (!exists(index_path)) {
        cache["index.html"] = string("");
        cerr << "Error: index.html does not exist!" << endl;
    } else {
        // read and store the index
        FILE *index_file = fopen(index_path.c_str(), "r");
        auto fsize = file_size(index_path);
        string index_contents(fsize, '\0');
        fread(&index_contents[0], sizeof index_contents[0], fsize,
                index_file);
        fclose(index_file);
        cache["index.html"] = index_contents;
    }
};

string const &Cache::fetch_index() {
    return cache["index.html"];
};

string &Cache::operator[] (string const &key) {
    auto it = cache.find(key);
    if (it == cache.end()) {
        // does the path exist?
        path file_path = document_root / key;
        if (!exists(file_path)) {
            cout << "File not found: " << file_path.string() << endl;
            return empty_str;
        } else {
            // cache it
            FILE *target_file = fopen(file_path.string().c_str(), "r");
            auto fsize = file_size(file_path.string());
            string file_contents(fsize, '\0');
            fread(&file_contents[0], sizeof file_contents[0], fsize,
                    target_file);
            fclose(target_file);
            cache[key] = file_contents;

            cout << "New file cached: " << file_path.string() << endl;

            return cache[key];
        }
    }
    else
        return it->second;
};
