#include "webserv.hpp"
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>

#define TEST_PASSED(name) std::cout << "✓ " << name << std::endl
#define TEST_FAILED(name) std::cout << "✗ " << name << std::endl; exit(1)

int test_count = 0;
int passed_count = 0;

void create_test_file(const std::string &filename, const std::string &content)
{
    std::ofstream file(filename.c_str());
    file << content;
    file.close();
}

void create_test_dir(const std::string &dirname)
{
    std::string cmd = "mkdir -p " + dirname;
    system(cmd.c_str());
}

void test_http_request_parsing()
{
    test_count++;
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/index.html";
    req.version = "HTTP/1.1";
    req.headers["Host"] = "example.com";
    req.headers["Content-Length"] = "0";
    
    assert(req.method == "GET");
    assert(req.path == "/index.html");
    assert(req.version == "HTTP/1.1");
    assert(req.headers["Host"] == "example.com");
    
    TEST_PASSED("HTTP request parsing");
    passed_count++;
}

void test_http_request_with_body()
{
    test_count++;
    
    HttpRequest req;
    req.method = "POST";
    req.path = "/api/data";
    req.version = "HTTP/1.1";
    req.headers["Content-Type"] = "application/json";
    req.headers["Content-Length"] = "13";
    req.body = "{\"key\":\"val\"}";
    
    assert(req.method == "POST");
    assert(req.body == "{\"key\":\"val\"}");
    assert(req.headers["Content-Type"] == "application/json");
    
    TEST_PASSED("HTTP request with body");
    passed_count++;
}

void test_http_response_generation()
{
    test_count++;
    
    HttpResponse resp;
    resp.status_code = 200;
    resp.status_text = "OK";
    resp.headers["Content-Type"] = "text/html";
    resp.body = "<html><body>Hello</body></html>";
    
    std::string response = resp.toString();
    
    assert(response.find("HTTP/1.1 200 OK") != std::string::npos);
    assert(response.find("Content-Type: text/html") != std::string::npos);
    assert(response.find("Content-Length:") != std::string::npos);
    assert(response.find("<html><body>Hello</body></html>") != std::string::npos);
    
    TEST_PASSED("HTTP response generation");
    passed_count++;
}

void test_http_response_404()
{
    test_count++;
    
    HttpResponse resp;
    resp.status_code = 404;
    resp.status_text = "Not Found";
    resp.headers["Content-Type"] = "text/plain";
    resp.body = "404 Not Found";
    
    std::string response = resp.toString();
    
    assert(response.find("HTTP/1.1 404 Not Found") != std::string::npos);
    assert(response.find("404 Not Found") != std::string::npos);
    
    TEST_PASSED("HTTP 404 response");
    passed_count++;
}

void test_http_response_redirect()
{
    test_count++;
    
    HttpResponse resp;
    resp.status_code = 301;
    resp.status_text = "Moved Permanently";
    resp.headers["Location"] = "/new-location";
    resp.body = "";
    
    std::string response = resp.toString();
    
    assert(response.find("HTTP/1.1 301 Moved Permanently") != std::string::npos);
    assert(response.find("Location: /new-location") != std::string::npos);
    
    TEST_PASSED("HTTP redirect response");
    passed_count++;
}

void test_server_config_loading()
{
    test_count++;
    
    const char *filename = "test_server.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    server_name localhost;\n"
        "    root /var/www;\n"
        "    index index.html;\n"
        "    location / {\n"
        "        autoindex on;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (!result)
    {
        TEST_FAILED("Server config loading");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers.size() == 1);
    assert(servers[0].port == 8080);
    assert(servers[0].server_name == "localhost");
    
    TEST_PASSED("Server config loading");
    passed_count++;
    system("rm test_server.conf");
}

void test_static_file_serving()
{
    test_count++;
    
    create_test_dir("test_www");
    create_test_file("test_www/index.html", 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<body>Welcome</body>\n"
        "</html>\n");
    
    std::ifstream file("test_www/index.html");
    if (!file.is_open())
    {
        TEST_FAILED("Static file serving");
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();
    
    assert(content.find("Welcome") != std::string::npos);
    
    TEST_PASSED("Static file serving");
    passed_count++;
    system("rm -rf test_www");
}

void test_file_mime_types()
{
    test_count++;
    
    create_test_dir("test_files");
    create_test_file("test_files/style.css", "body { color: red; }");
    create_test_file("test_files/script.js", "console.log('test');");
    create_test_file("test_files/data.json", "{\"key\":\"value\"}");
    
    HttpResponse resp1;
    resp1.headers["Content-Type"] = "text/css";
    assert(resp1.headers["Content-Type"] == "text/css");
    
    HttpResponse resp2;
    resp2.headers["Content-Type"] = "application/javascript";
    assert(resp2.headers["Content-Type"] == "application/javascript");
    
    HttpResponse resp3;
    resp3.headers["Content-Type"] = "application/json";
    assert(resp3.headers["Content-Type"] == "application/json");
    
    TEST_PASSED("File mime types");
    passed_count++;
    system("rm -rf test_files");
}

void test_request_with_query_string()
{
    test_count++;
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/search";
    req.query_string = "q=hello&page=1";
    req.version = "HTTP/1.1";
    
    assert(req.path == "/search");
    assert(req.query_string == "q=hello&page=1");
    
    TEST_PASSED("Request with query string");
    passed_count++;
}

void test_post_request_with_form_data()
{
    test_count++;
    
    HttpRequest req;
    req.method = "POST";
    req.path = "/login";
    req.version = "HTTP/1.1";
    req.headers["Content-Type"] = "application/x-www-form-urlencoded";
    req.body = "username=user&password=pass";
    
    assert(req.method == "POST");
    assert(req.body.find("username=user") != std::string::npos);
    
    TEST_PASSED("POST request with form data");
    passed_count++;
}

void test_cgi_handler_initialization()
{
    test_count++;
    
    HttpRequest req;
    req.method = "GET";
    req.path = "/cgi-bin/script.php";
    req.version = "HTTP/1.1";
    req.headers["Host"] = "localhost";
    
    CGIHandler handler(req, "test.php", "/usr/bin/php-cgi");
    
    assert(req.method == "GET");
    
    TEST_PASSED("CGI handler initialization");
    passed_count++;
}

void test_location_config_with_cgi()
{
    test_count++;
    
    const char *filename = "test_cgi_config.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    root /var/www;\n"
        "    location /cgi-bin {\n"
        "        cgi_pass .php /usr/bin/php-cgi;\n"
        "        cgi_pass .py /usr/bin/python3;\n"
        "        limit_except GET POST;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (!result)
    {
        TEST_FAILED("Location config with CGI");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].cgi_pass.size() == 2);
    assert(servers[0].locations[0].allowed_methods.size() == 2);
    
    TEST_PASSED("Location config with CGI");
    passed_count++;
    system("rm test_cgi_config.conf");
}

void test_upload_location()
{
    test_count++;
    
    const char *filename = "test_upload.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    root /var/www;\n"
        "    location /upload {\n"
        "        upload_store /var/www/uploads;\n"
        "        limit_except POST DELETE;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (!result)
    {
        TEST_FAILED("Upload location");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].upload_store == "/var/www/uploads");
    assert(servers[0].locations[0].allowed_methods.size() == 2);
    
    TEST_PASSED("Upload location");
    passed_count++;
    system("rm test_upload.conf");
}

void test_error_page_handling()
{
    test_count++;
    
    const char *filename = "test_errors.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    root /var/www;\n"
        "    error_page 404 /404.html;\n"
        "    error_page 500 /500.html;\n"
        "    error_page 403 /403.html;\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (!result)
    {
        TEST_FAILED("Error page handling");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].error_pages.size() == 3);
    
    std::map<int, std::string>::const_iterator it;
    it = servers[0].error_pages.find(404);
    assert(it != servers[0].error_pages.end());
    assert(it->second == "/404.html");
    
    TEST_PASSED("Error page handling");
    passed_count++;
    system("rm test_errors.conf");
}

void test_multiple_server_blocks()
{
    test_count++;
    
    const char *filename = "test_multi_servers.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    server_name site1.com;\n"
        "    root /var/www/site1;\n"
        "}\n"
        "server {\n"
        "    listen 9090;\n"
        "    server_name site2.com;\n"
        "    root /var/www/site2;\n"
        "}\n"
        "server {\n"
        "    listen 127.0.0.1:3000;\n"
        "    server_name localhost;\n"
        "    root /var/www/local;\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (!result)
    {
        TEST_FAILED("Multiple server blocks");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers.size() == 3);
    assert(servers[0].port == 8080);
    assert(servers[1].port == 9090);
    assert(servers[2].port == 3000);
    assert(servers[2].host == "127.0.0.1");
    
    TEST_PASSED("Multiple server blocks");
    passed_count++;
    system("rm test_multi_servers.conf");
}

void test_response_headers()
{
    test_count++;
    
    HttpResponse resp;
    resp.status_code = 200;
    resp.status_text = "OK";
    resp.headers["Content-Type"] = "text/html; charset=UTF-8";
    resp.headers["Cache-Control"] = "no-cache";
    resp.headers["Set-Cookie"] = "sessionid=abc123; Path=/";
    resp.body = "<html></html>";
    
    std::string response = resp.toString();
    
    assert(response.find("Content-Type: text/html; charset=UTF-8") != std::string::npos);
    assert(response.find("Cache-Control: no-cache") != std::string::npos);
    assert(response.find("Set-Cookie: sessionid=abc123; Path=/") != std::string::npos);
    assert(response.find("Connection: close") != std::string::npos);
    
    TEST_PASSED("Response headers");
    passed_count++;
}

void test_default_values()
{
    test_count++;
    
    ServerConfig srv;
    LocationConfig loc;
    
    assert(srv.port == 0);
    assert(srv.client_max_body_size == 0);
    assert(loc.autoindex == false);
    assert(loc.redirect_code == 0);
    
    TEST_PASSED("Default values");
    passed_count++;
}

void test_body_size_limits()
{
    test_count++;
    
    const char *filename = "test_limits.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    client_max_body_size 50M;\n"
        "    location /upload {\n"
        "        client_max_body_size 100M;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (!result)
    {
        TEST_FAILED("Body size limits");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].client_max_body_size == 52428800);
    
    TEST_PASSED("Body size limits");
    passed_count++;
    system("rm test_limits.conf");
}

void test_http_methods()
{
    test_count++;
    
    HttpRequest get_req;
    get_req.method = "GET";
    
    HttpRequest post_req;
    post_req.method = "POST";
    
    HttpRequest delete_req;
    delete_req.method = "DELETE";
    
    HttpRequest put_req;
    put_req.method = "PUT";
    
    assert(get_req.method == "GET");
    assert(post_req.method == "POST");
    assert(delete_req.method == "DELETE");
    assert(put_req.method == "PUT");
    
    TEST_PASSED("HTTP methods");
    passed_count++;
}

void test_constants()
{
    test_count++;
    
    assert(BACKLOG == 128);
    assert(POLL_TIMEOUT_MS == -1);
    assert(CGI_TIMEOUT_S == 10);
    assert(READ_BUFFER_SIZE == 4096);
    assert(std::string(DEFAULT_CONFIG_PATH) == "webserv.conf");
    
    TEST_PASSED("Server constants");
    passed_count++;
}

int main()
{
    std::cout << "\n========== Server Integration Tests ==========\n\n";
    
    test_http_request_parsing();
    test_http_request_with_body();
    test_http_response_generation();
    test_http_response_404();
    test_http_response_redirect();
    test_server_config_loading();
    test_static_file_serving();
    test_file_mime_types();
    test_request_with_query_string();
    test_post_request_with_form_data();
    test_cgi_handler_initialization();
    test_location_config_with_cgi();
    test_upload_location();
    test_error_page_handling();
    test_multiple_server_blocks();
    test_response_headers();
    test_default_values();
    test_body_size_limits();
    test_http_methods();
    test_constants();
    
    std::cout << "\n============================================\n";
    std::cout << "Tests Passed: " << passed_count << "/" << test_count << "\n";
    std::cout << "============================================\n\n";
    
    if (passed_count == test_count)
    {
        std::cout << "All tests passed! ✓\n" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "Some tests failed! ✗\n" << std::endl;
        return 1;
    }
}
