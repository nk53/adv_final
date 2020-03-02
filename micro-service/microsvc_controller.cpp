//
//  Created by Ivan Mejia on 12/24/16.
//
// MIT License
//
// Copyright (c) 2016 ivmeroLabs.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <boost/filesystem.hpp>
#include <std_micro_service.hpp>
#include <string>
#include <tuple>

#include "microsvc_controller.hpp"

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

#include "../fs_utils.hpp"

using namespace web;
using namespace http;

using namespace std;

string getMongoResponse(vector<string> path, string& mongo_credentials) {
    // just ignore anything past [1]
    string query_str = path[1];

    map<string, string> mongo_auth = str_to_map(mongo_credentials);

    string uri_str = string("mongodb://") +
        mongo_auth["user"] + ":" + mongo_auth["pass"] + "@" +
        mongo_auth["host"] + ":" + mongo_auth["port"] + "/" +
        mongo_auth["db_name"];

    mongocxx::client client{mongocxx::uri{uri_str}};

    auto session = client.start_session();
    auto coll = client["nktwitter"]["hist"];

    using bsoncxx::builder::basic::make_document;
    using bsoncxx::builder::basic::kvp;
    using bsoncxx::to_json;

    auto query = make_document(kvp("term", query_str));
    auto response = coll.find_one(move(query));

    if (response)
        return to_json(*response);
    else
        return "{\"success\": false}";
};

MicroserviceController::MicroserviceController(Cache *cache, string *mongo_auth)
: cache(cache), mongo_auth(mongo_auth) {}

void MicroserviceController::initRestOpHandlers() {
    _listener.support(methods::GET, std::bind(&MicroserviceController::handleGet, this, std::placeholders::_1));
    _listener.support(methods::PUT, std::bind(&MicroserviceController::handlePut, this, std::placeholders::_1));
    _listener.support(methods::POST, std::bind(&MicroserviceController::handlePost, this, std::placeholders::_1));
    _listener.support(methods::DEL, std::bind(&MicroserviceController::handleDelete, this, std::placeholders::_1));
    _listener.support(methods::PATCH, std::bind(&MicroserviceController::handlePatch, this, std::placeholders::_1));
}

void MicroserviceController::handleGet(http_request message) {
    // vector of strings
    auto path = requestPath(message);
    if (!path.empty()) {
        if (path[0] == "service" && path[1] == "test") {
            auto response = json::value::object();
            response["version"] = json::value::string("0.1.1");
            response["status"] = json::value::string("ready!");
            message.reply(status_codes::OK, response);
        } else if (path[0] == "files" && path.size() == 2) {
            string const &target = path[1];
            string &contents = (*cache)[target];

            boost::filesystem::path target_name(target);
            if (contents.empty()) {
                message.reply(status_codes::NotFound);
            } else {
                string extension = target_name.extension().string();
                string &mime_type = cache->mime_map[extension];
                message.reply(status_codes::OK, contents, mime_type);
            }
        } else if (path[0] == "data" && path.size() == 2) {
            string response = getMongoResponse(path, *mongo_auth);
            message.reply(status_codes::OK, response,
                    "text/javascript; charset=utf-8");
        } else {
            message.reply(status_codes::NotFound);
        }
    } else {
        message.reply(status_codes::OK, cache->fetch_index(),
                "text/html; charset=utf-8");
    }
}

void MicroserviceController::handlePatch(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::PATCH));
}

void MicroserviceController::handlePut(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::PUT));
}

void MicroserviceController::handlePost(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::POST));
}

void MicroserviceController::handleDelete(http_request message) {    
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::DEL));
}

void MicroserviceController::handleHead(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::HEAD));
}

void MicroserviceController::handleOptions(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::OPTIONS));
}

void MicroserviceController::handleTrace(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::TRCE));
}

void MicroserviceController::handleConnect(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::CONNECT));
}

void MicroserviceController::handleMerge(http_request message) {
    message.reply(status_codes::NotImplemented, responseNotImpl(methods::MERGE));
}

json::value MicroserviceController::responseNotImpl(const http::method & method) {
    auto response = json::value::object();
    response["serviceName"] = json::value::string("C++ Mircroservice Sample");
    response["http_method"] = json::value::string(method);
    return response ;
}
