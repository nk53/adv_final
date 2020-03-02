#include <iostream>
#include <map>
#include <string>

/**
 * Takes a string in CSV format and returns a map of K/V pairs.
 *
 * CSV delimiters used are: field: ' ', entry: '\n'. Assumes only two
 * fields per entry.
 */
std::map<std::string, std::string> str_to_map(std::string const &str);

/**
 * Opens the file referred by `filename` and returns its contents as a
 * string. Do not use more than once per file.
 */
std::string file_to_string(char const * const filename);

/**
 * Splits a string in two by `delim`, and returns the parts as a 2-tuple.
 */
std::tuple<std::string, std::string>
strsplit(std::string const &str, char const delim);
