#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h1>CGI Works!</h1>")

method = os.environ.get("REQUEST_METHOD", "")
print("<p>Method: " + method + "</p>")

if method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", 0))
    body = sys.stdin.read(length)
    print("<p>Body: " + body + "</p>")

print("</body></html>")