#include "fs_utils.hpp"

using namespace std;

map<string, string> str_to_map(string const &str) {
    map<string, string> strmap;

    // split each line into k/v pairs
    size_t start_pos = 0;
    size_t end_pos = str.find('\n');
    while (end_pos != string::npos) {
        string line = str.substr(start_pos, end_pos-start_pos);
        string key, value;
        tie(key, value) = strsplit(line, ' ');
        strmap[key] = value;

        // get bounds of next line
        start_pos = end_pos + 1;
        end_pos = str.find('\n', start_pos);
    }

    // handle last line, which may or may not be \n terminated
    if (start_pos < str.size()-1) {
        string line = str.substr(start_pos);
        string key, value;
        tie(key, value) = strsplit(line, ' ');
        strmap[key] = value;
    }

    return strmap;
};

string file_to_string(char const * const filename) {
    // open the file for reading
    FILE *fh = fopen(filename, "r");

    if (fh == nullptr) {
        cerr << "Error: Can't open " << filename << endl;
        exit(1);
    }

    // get the file size
    fseek(fh, 0, SEEK_END);
    size_t fsize = (size_t) ftell(fh);
    fseek(fh, 0, SEEK_SET);

    string contents(fsize, '\0');

    // get the file's contents as a string
    fread(&contents[0], sizeof(contents[0]), fsize, fh);

    // close the file
    fclose(fh);

    // remove trailing whitespace
    auto rit = contents.rbegin();
    auto eit = contents.rend();
    size_t end_pos = fsize;
    for (; rit != eit; ++rit, --end_pos) {
        if (*rit != '\n' && *rit != ' ')
            break;
    }
    contents = contents.substr(0, end_pos);

    return contents;
};

// splits a string (once) by a delimeter, returning a 2-tuple of strings
std::tuple<std::string, std::string>
strsplit(string const &str, char const delim) {
    size_t const pos_1 = str.find(delim);
    if (pos_1 == string::npos) {
        cerr << "Error: no '" << delim << "' in '" << str << "'" << endl;
        exit(2);
    }

    size_t pos_2 = pos_1 + 1;
    while (str[pos_2] == delim && pos_2 < str.size())
        ++pos_2;

    if (pos_2 == str.size()) {
        cerr << "Error: nothing to split" << endl;
    }

    return make_tuple(
        str.substr(0, pos_1),
        str.substr(pos_2)
    );
};

