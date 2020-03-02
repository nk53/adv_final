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

#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <mongocxx/instance.hpp>

#include <usr_interrupt_handler.hpp>
#include <runtime_utils.hpp>

#include "micro-service/microsvc_controller.hpp"
#include "fs_utils.hpp"

using namespace web;
using namespace cfx;

using namespace boost::filesystem;

using std::string;

int main(int argc, const char * argv[]) {
    InterruptHandler::hookSIGINT();

    // Get mongo credentials
    string auth_str = file_to_string("../mongo_auth.txt");
    // Initialize mongo driver
    mongocxx::instance inst{};

    // Setup server
    path document_root = path("..") / "public";
    Cache *cache = new Cache(document_root);
    MicroserviceController server(cache, &auth_str);
    server.setEndpoint("http://host_auto_ip4:80/");
    
    try {
        // Wait for server initialization...
        server.accept().wait();
        std::cout << "Server now listening at: "
                  << server.endpoint() << '\n';
        
        // Server shutdown signalled by SIGINT
        InterruptHandler::waitForUserInterrupt();
        server.shutdown().wait();
    }
    catch(std::exception & e) {
        std::cerr << "Failed to start server: " << e.what() << '\n';
    }
    catch(...) {
        RuntimeUtils::printStackTrace();
    }

    return 0;
}
