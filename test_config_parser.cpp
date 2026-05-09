#include "ConfigParser.hpp"
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <fstream>

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

void test_simple_config()
{
    test_count++;
    const char *filename = "test_simple.conf";
    create_test_file(filename, 
        "server {\n"
        "    listen 8080;\n"
        "    server_name example.com;\n"
        "    root /var/www;\n"
        "    index index.html;\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Simple config parse");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers.size() == 1);
    assert(servers[0].port == 8080);
    assert(servers[0].server_name == "example.com");
    assert(servers[0].root == "/var/www");
    assert(servers[0].index == "index.html");
    
    TEST_PASSED("Simple config parse");
    passed_count++;
    system("rm test_simple.conf");
}

void test_multiple_servers()
{
    test_count++;
    const char *filename = "test_multi.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "}\n"
        "server {\n"
        "    listen 9090;\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Multiple servers");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers.size() == 2);
    assert(servers[0].port == 8080);
    assert(servers[1].port == 9090);
    
    TEST_PASSED("Multiple servers");
    passed_count++;
    system("rm test_multi.conf");
}

void test_host_port()
{
    test_count++;
    const char *filename = "test_host.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 127.0.0.1:8080;\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Host and port");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].host == "127.0.0.1");
    assert(servers[0].port == 8080);
    
    TEST_PASSED("Host and port");
    passed_count++;
    system("rm test_host.conf");
}

void test_locations()
{
    test_count++;
    const char *filename = "test_loc.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    root /var/www;\n"
        "    location /api {\n"
        "        root /var/api;\n"
        "    }\n"
        "    location /static {\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Locations");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations.size() == 2);
    assert(servers[0].locations[0].path == "/api");
    assert(servers[0].locations[0].root == "/var/api");
    assert(servers[0].locations[1].path == "/static");
    assert(servers[0].locations[1].root == "/var/www");
    
    TEST_PASSED("Locations");
    passed_count++;
    system("rm test_loc.conf");
}

void test_size_parsing()
{
    test_count++;
    const char *filename = "test_size.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    client_max_body_size 10M;\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Size parsing");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].client_max_body_size == 10485760);
    
    TEST_PASSED("Size parsing");
    passed_count++;
    system("rm test_size.conf");
}

void test_size_variants()
{
    test_count++;
    
    ConfigParser p1("temp1.conf");
    create_test_file("temp1.conf", "server { listen 8080; client_max_body_size 512K; }");
    p1.parse();
    size_t size_k = p1.getServers()[0].client_max_body_size;
    assert(size_k == 524288);
    
    ConfigParser p2("temp2.conf");
    create_test_file("temp2.conf", "server { listen 8080; client_max_body_size 1G; }");
    p2.parse();
    size_t size_g = p2.getServers()[0].client_max_body_size;
    assert(size_g == 1073741824);
    
    ConfigParser p3("temp3.conf");
    create_test_file("temp3.conf", "server { listen 8080; client_max_body_size 4096; }");
    p3.parse();
    size_t size_plain = p3.getServers()[0].client_max_body_size;
    assert(size_plain == 4096);
    
    TEST_PASSED("Size variants (K/M/G/plain)");
    passed_count++;
    system("rm temp1.conf temp2.conf temp3.conf");
}

void test_error_pages()
{
    test_count++;
    const char *filename = "test_errors.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    error_page 404 /404.html;\n"
        "    error_page 500 /500.html;\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Error pages");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].error_pages.size() == 2);
    assert(servers[0].error_pages.find(404) != servers[0].error_pages.end());
    assert(servers[0].error_pages.find(404)->second == "/404.html");
    assert(servers[0].error_pages.find(500) != servers[0].error_pages.end());
    assert(servers[0].error_pages.find(500)->second == "/500.html");
    
    TEST_PASSED("Error pages");
    passed_count++;
    system("rm test_errors.conf");
}

void test_cgi_pass()
{
    test_count++;
    const char *filename = "test_cgi.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    location /cgi-bin {\n"
        "        cgi_pass .php /usr/bin/php-cgi;\n"
        "        cgi_pass .py /usr/bin/python3;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("CGI pass");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].cgi_pass.size() == 2);
    assert(servers[0].locations[0].cgi_pass.find(".php") != servers[0].locations[0].cgi_pass.end());
    assert(servers[0].locations[0].cgi_pass.find(".php")->second == "/usr/bin/php-cgi");
    assert(servers[0].locations[0].cgi_pass.find(".py") != servers[0].locations[0].cgi_pass.end());
    assert(servers[0].locations[0].cgi_pass.find(".py")->second == "/usr/bin/python3");
    
    TEST_PASSED("CGI pass");
    passed_count++;
    system("rm test_cgi.conf");
}

void test_limit_except()
{
    test_count++;
    const char *filename = "test_limit.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    location /upload {\n"
        "        limit_except GET POST;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Limit except");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].allowed_methods.size() == 2);
    assert(servers[0].locations[0].allowed_methods[0] == "GET");
    assert(servers[0].locations[0].allowed_methods[1] == "POST");
    
    TEST_PASSED("Limit except");
    passed_count++;
    system("rm test_limit.conf");
}

void test_redirect()
{
    test_count++;
    const char *filename = "test_redirect.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    location /old {\n"
        "        return 301 /new;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Redirect");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].redirect_code == 301);
    assert(servers[0].locations[0].redirect == "/new");
    
    TEST_PASSED("Redirect");
    passed_count++;
    system("rm test_redirect.conf");
}

void test_upload_store()
{
    test_count++;
    const char *filename = "test_upload.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    location /upload {\n"
        "        upload_store /var/www/uploads;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Upload store");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].upload_store == "/var/www/uploads");
    
    TEST_PASSED("Upload store");
    passed_count++;
    system("rm test_upload.conf");
}

void test_autoindex()
{
    test_count++;
    const char *filename = "test_autoindex.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 8080;\n"
        "    location / {\n"
        "        autoindex on;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Autoindex ON");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].locations[0].autoindex == true);
    
    TEST_PASSED("Autoindex ON");
    passed_count++;
    system("rm test_autoindex.conf");
}

void test_complex_config()
{
    test_count++;
    const char *filename = "test_complex.conf";
    create_test_file(filename,
        "server {\n"
        "    listen 127.0.0.1:8080;\n"
        "    server_name example.com;\n"
        "    root /var/www;\n"
        "    index index.html;\n"
        "    client_max_body_size 10M;\n"
        "    error_page 404 /404.html;\n"
        "    location / {\n"
        "        autoindex on;\n"
        "    }\n"
        "    location /cgi-bin {\n"
        "        cgi_pass .php /usr/bin/php-cgi;\n"
        "        limit_except GET POST;\n"
        "    }\n"
        "    location /upload {\n"
        "        upload_store /var/www/uploads;\n"
        "        limit_except POST;\n"
        "    }\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Complex config");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers.size() == 1);
    assert(servers[0].port == 8080);
    assert(servers[0].locations.size() == 3);
    
    TEST_PASSED("Complex config");
    passed_count++;
    system("rm test_complex.conf");
}

void test_invalid_syntax()
{
    test_count++;
    const char *filename = "test_invalid.conf";
    create_test_file(filename,
        "server\n"
        "    listen 8080;\n"
        "}\n");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (result)
    {
        TEST_FAILED("Invalid syntax detection");
        return;
    }
    
    TEST_PASSED("Invalid syntax detection");
    passed_count++;
    system("rm test_invalid.conf");
}

void test_empty_file()
{
    test_count++;
    const char *filename = "test_empty.conf";
    create_test_file(filename, "");
    
    ConfigParser parser(filename);
    bool result = parser.parse();
    
    if (result)
    {
        TEST_FAILED("Empty file detection");
        return;
    }
    
    TEST_PASSED("Empty file detection");
    passed_count++;
    system("rm test_empty.conf");
}

void test_comments()
{
    test_count++;
    const char *filename = "test_comments.conf";
    create_test_file(filename,
        "# This is a comment\n"
        "server {\n"
        "    listen 8080;  # inline comment\n"
        "    root /var/www; # another comment\n"
        "}\n");
    
    ConfigParser parser(filename);
    if (!parser.parse())
    {
        TEST_FAILED("Comment handling");
        return;
    }
    
    const std::vector<ServerConfig> &servers = parser.getServers();
    assert(servers[0].port == 8080);
    assert(servers[0].root == "/var/www");
    
    TEST_PASSED("Comment handling");
    passed_count++;
    system("rm test_comments.conf");
}

int main()
{
    std::cout << "\n========== ConfigParser Unit Tests ==========\n\n";
    
    test_simple_config();
    test_multiple_servers();
    test_host_port();
    test_locations();
    test_size_parsing();
    test_size_variants();
    test_error_pages();
    test_cgi_pass();
    test_limit_except();
    test_redirect();
    test_upload_store();
    test_autoindex();
    test_complex_config();
    test_invalid_syntax();
    test_empty_file();
    test_comments();
    
    std::cout << "\n========================================\n";
    std::cout << "Tests Passed: " << passed_count << "/" << test_count << "\n";
    std::cout << "========================================\n\n";
    
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
