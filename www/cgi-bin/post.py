#!/usr/bin/env python3
import os
import sys
 
method = os.environ.get("REQUEST_METHOD", "")
content_type = os.environ.get("CONTENT_TYPE", "")
content_length = os.environ.get("CONTENT_LENGTH", "0")
 
body = ""
if method == "POST":
    try:
        length = int(content_length)
    except ValueError:
        length = 0
    if length > 0:
        body = sys.stdin.read(length)
 
print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h1>CGI POST test</h1>")
print("<p>Method: " + method + "</p>")
print("<p>Content-Type: " + content_type + "</p>")
print("<p>Content-Length: " + content_length + "</p>")
print("<p>Body received: " + body + "</p>")
print("</body></html>")
