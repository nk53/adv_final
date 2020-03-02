#include <cpprest/http_client.h>
#include <cpprest/json.h>

using namespace web;
using namespace http;
using namespace client;
using namespace json;
using namespace utility;

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <cstdint>
#include <cctype>
#include <exception>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "fs_utils.hpp"

using namespace std;

/**
 * Given a date/time string returned by Twitter, returns the time in
 * seconds since UNIX epoch.
 */
int parse_twitter_time(char const * time_str) {
    // Thanks to Tom Parker for the format string ...
    char format[] = "%a %b %d %H:%M:%S %z %Y";
    struct tm time_struct;

    strptime(time_str, format, &time_struct);

    return mktime(&time_struct);
}

/**
 * All interaction with Mongo happens here. TODO: Split this into multiple
 * functions for improved readability.
 *
 * In detail, the following tasks are handled here:
 *  1) Initialize mongocxx driver
 *  2) Connect to MongoDB
 *  3) Parse Twitter response to obtain id_str, tag, create_date
 *  4) Find Twitter statuses already stored in MongoDB
 *  5) Update tags for statuses already stored
 *  6) Insert new entries for new statuses
 *  7) Produce and insert histogram of create_dates binned by day of week
 */
void send_json_to_mongo(string& response_json_str, string& query_term) {
    string mongo_user, mongo_pass;
    string mongo_credentials = file_to_string("../mongo_auth.txt");
    map<string, string> mongo_auth = str_to_map(mongo_credentials);

    string uri_str = string("mongodb://") +
        mongo_auth["user"] + ":" + mongo_auth["pass"] + "@" +
        mongo_auth["host"] + ":" + mongo_auth["port"] + "/" +
        mongo_auth["db_name"];

    cout << "Attempting connection with " << mongo_auth["host"]
         << " ..." << endl;

    mongocxx::instance inst{};
    mongocxx::client client{mongocxx::uri{uri_str}};

    auto session = client.start_session();
    auto coll = client["nktwitter"]["statuses"];

    using bsoncxx::builder::stream::close_array;
    using bsoncxx::builder::stream::close_document;
    using bsoncxx::builder::stream::document;
    using bsoncxx::builder::stream::finalize;
    using bsoncxx::builder::stream::open_array;
    using bsoncxx::builder::stream::open_document;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::make_array;
    using bsoncxx::to_json;
    using bsoncxx::from_json;

    bool const debug = false;

    // extract "statuses" array from Twitter response
    if (debug)
        cout << "Extracting statuses" << endl;
    bsoncxx::document::value parsed_str = from_json(response_json_str);
    auto statuses_iter = parsed_str.view().find("statuses");
    bsoncxx::document::element statuses_elem = (*statuses_iter);
    bsoncxx::array::view statuses_view = statuses_elem.get_array();

    // determine which statuses already exist in database
    if (debug)
        cout << "Finding which statuses are already in database" << endl;
    map<string, bool> insert_ids; // true for ids to insert
    auto find_ids = bsoncxx::builder::basic::array{};
    for (auto &&elem : statuses_view) {
        // extracts C++ string from a JSON string field
        string id_str = elem["id_str"].get_utf8().value.to_string();
        // try to find this id_str in DB
        find_ids.append(make_document(kvp("id_str", id_str)));
        // initially assume this status must be inserted
        insert_ids[id_str] = true;
    }
    auto filter = bsoncxx::builder::basic::document{};
    filter.append(kvp("$or", find_ids.extract()));
    auto cursor = coll.find(session, filter.extract());

    // add this query_term to the list of terms for each tweet
    if (debug)
        cout << "Building duplicate ID list" << endl;
    size_t update_count = 0;
    auto update_ids = bsoncxx::builder::basic::array{};
    for (bsoncxx::document::view doc : cursor) {
        bsoncxx::array::view term_array = doc["terms"].get_array().value;
        bool found_term = false;
        for (auto term : term_array) {
            string term_str = term.get_utf8().value.to_string();
            if (term_str == query_term) {
                found_term = true;
                break;
            }
        }
        // need to mark this ID as not for insertion
        string id_str = doc["id_str"].get_utf8().value.to_string();
        insert_ids[id_str] = false;
        // if term already exists for this post, just ignore
        if (!found_term) {
            ++update_count;
            // append this term to the list of terms for this tweet
            update_ids.append(id_str);
        }
    }

    // perform the update
    if (update_count) {
        filter = bsoncxx::builder::basic::document{};

        // i.e., update the statuses where id_str is one of update_ids
        filter.append(kvp("id_str", make_document(kvp("$in", update_ids))));
        bsoncxx::document::value update = document{} << "$push"
            << open_document << "terms" << query_term << close_document
            << finalize;

        if (debug) {
            cout << "Updating database with the following:" << endl;
            cout << "\tFilter: " << to_json(filter) << endl;
            cout << "\tUpdate: " << to_json(update) << endl;
        }

        cout << update_count << " status tag list(s) were updated" << endl;

        coll.update_many(session,
                filter.extract(),
                move(update));
    }

    // build a list of new documents containing only the metadata we want
    vector<bsoncxx::document::value> insert_documents;
    for (auto &&elem : statuses_view) {
        // get the underlying document
        bsoncxx::document::view elem_view = elem.get_document();

        // don't insert statuses already in database
        string id_str = elem_view["id_str"].get_utf8()
            .value.to_string();
        if (!insert_ids[id_str])
            continue;

        string created_at = elem_view["created_at"].get_utf8()
            .value.to_string();

        // convert time string to time since epoch
        int created_unix_time = parse_twitter_time(created_at.c_str());

        // also save the day-of-week in upper-case
        string created_day = created_at.substr(0, 3);
        for (auto &c : created_day)
            c = toupper(c);

        bsoncxx::document::value condensed_status = make_document(
            kvp("id_str", id_str),
            kvp("created_at", created_unix_time),
            kvp("created_day", created_day),
            kvp("terms", make_array(query_term))
       );

        insert_documents.push_back(condensed_status);
    }

    auto insert_result = coll.insert_many(session, insert_documents);

    // get the new histogram for this tag
    auto hist_results = coll.aggregate(
        mongocxx::pipeline().match(
            make_document(kvp("terms", query_term))
        ).group(
            make_document(
                kvp("_id", "$created_day"),
                kvp("count", make_document(kvp("$sum", 1)))
            )
        )
    );

    // insert the new histogram into nktwitter.hist
    cout << "Resulting histogram: " << endl;
    auto hist_builder = bsoncxx::builder::basic::document{};
    for (auto doc : hist_results) {
        cout << to_json(doc) << endl;
        string const day_str = doc["_id"].get_utf8().value.to_string();
        int const day_count = doc["count"].get_int32().value;
        hist_builder.append(kvp(day_str, day_count));
    }

    // tag the histogram by query_term
    auto hist_coll = client["nktwitter"]["hist"];
    auto hist_filter = make_document(kvp("term", query_term));
    auto hist_replacement = make_document(
        kvp("term", query_term),
        kvp("histogram", hist_builder.extract()));

    auto hist_ups_result = hist_coll.find_one_and_replace(
        session, move(hist_filter), move(hist_replacement),
        mongocxx::options::find_one_and_replace{}.upsert(true));

    if (hist_ups_result)
        cout << "Updated existing histogram" << endl;
    else
        cout << "Inserted new histogram" << endl;
};

/**
 * 1) Obtains a term to search from stdin
 * 2) Queries Twitter for 100 tweets matching the term
 * 3) Passes Twitter response to send_json_to_mongo()
 */
int main() {
    string host_url = "https://api.twitter.com";
    string url = "/1.1/search/tweets.json";
    string query = string("?q=");
    string term;

    // get desired term to search
    cout << "Note: Maximum query length is 255" << endl;
    cout << "Enter Twitter search term(s): ";
    char buffer[255] = {'\0'};
    cin.getline(buffer, 255);
    term = move(buffer);
    query += term + "&count=100";

    uri search_uri(url + query);

    http_request request("GET");
    request.set_request_uri(search_uri);
    http_headers &headers = request.headers();
    string bearer_token = file_to_string("../bearer.txt");
    string auth = string("Bearer " + bearer_token);
    headers.add<string>("Authorization", auth);

    cout << "Twitter Request: GET " << url + query << endl;

    utf8string response_json_str;
    http_client client(host_url);
    client.request(request).then([&](http_response response) {
        // just in case ...
        response.content_ready().wait();
        try {
            response.extract_utf8string().then([&](utf8string value) {
                response_json_str = value;
            }).wait();
        } catch (exception const &e) {
            cerr << "Can't extract UTF-8: " << endl;
        }
    }).wait();

    // ask user for confirmation before proceeding
    string snippet = response_json_str.substr(0, 58);
    cout << "Got response: " << snippet;
    if (snippet.size() < response_json_str.size())
        cout << " ...";
    cout << endl;

    cout << "Continue [y/N]? ";
    char response = 'n'; // default: NO
    cin.get(response);
    response = tolower(response);
    if (response != 'y') {
        cout << "Closing ..." << endl;
        exit(0);
    }

    send_json_to_mongo(response_json_str, term);

    return 0;
}
