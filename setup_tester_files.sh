#!/bin/bash

ROOT="${1:-EngineX/www}"

echo "Using tester root: $ROOT"

# ---------------------------------------
# Create root
# ---------------------------------------
mkdir -p "$ROOT"

# ---------------------------------------
# Normal files
# ---------------------------------------
echo "Hello Webserv" > "$ROOT/index.html"

# ---------------------------------------
# Forbidden FILE tests
# ---------------------------------------
touch "$ROOT/forbidden.txt"
touch "$ROOT/forbidden.html"

chmod 000 "$ROOT/forbidden.txt"
chmod 000 "$ROOT/forbidden.html"

# ---------------------------------------
# Forbidden DIRECTORY test
# ---------------------------------------
# Restore permission first in case script
# was already executed before.
if [ -d "$ROOT/forbidden_dir" ]; then
    chmod 755 "$ROOT/forbidden_dir"
fi

mkdir -p "$ROOT/forbidden_dir"
touch "$ROOT/forbidden_dir/secret.txt"

chmod 000 "$ROOT/forbidden_dir"

# ---------------------------------------
# Directory without index
# autoindex OFF => must return 403
# ---------------------------------------
mkdir -p "$ROOT/empty_dir"

chmod 755 "$ROOT/empty_dir"

rm -f "$ROOT/empty_dir/index.html"
rm -f "$ROOT/empty_dir/index.htm"
rm -f "$ROOT/empty_dir/index"
rm -f "$ROOT/empty_dir/youpi.bad_extension"

# ---------------------------------------
# Directory tester structure
# Seen in tester strings
# ---------------------------------------
mkdir -p "$ROOT/directory/nop"
mkdir -p "$ROOT/directory/Yeah"

chmod 755 "$ROOT/directory"
chmod 755 "$ROOT/directory/nop"
chmod 755 "$ROOT/directory/Yeah"

# Files used by common 42 webserv tester
echo "YOU PI" > "$ROOT/directory/youpi.bad_extension"
echo "NOP FILE" > "$ROOT/directory/nop/file"
echo "OTHER POUAC" > "$ROOT/directory/nop/other.pouac"
echo "OTHER POUIC" > "$ROOT/directory/nop/other.pouic"

echo "HAPPY" > "$ROOT/directory/Yeah/happy.bad_extension"
echo "NOT HAPPY" > "$ROOT/directory/Yeah/not_happy.bad_extension"

chmod 644 "$ROOT/directory/youpi.bad_extension"
chmod 644 "$ROOT/directory/nop/file"
chmod 644 "$ROOT/directory/nop/other.pouac"
chmod 644 "$ROOT/directory/nop/other.pouic"
chmod 644 "$ROOT/directory/Yeah/happy.bad_extension"
chmod 644 "$ROOT/directory/Yeah/not_happy.bad_extension"

# ---------------------------------------
# Upload directory if tester needs POST
# ---------------------------------------
mkdir -p "$ROOT/upload"
chmod 755 "$ROOT/upload"

# ---------------------------------------
# Show result
# ---------------------------------------
echo
echo "================================"
echo "Tester files prepared"
echo "================================"

echo
echo "Forbidden files:"
ls -l "$ROOT/forbidden.txt" "$ROOT/forbidden.html"

echo
echo "Forbidden directory:"
ls -ld "$ROOT/forbidden_dir"

echo
echo "Empty directory:"
ls -ld "$ROOT/empty_dir"

echo
echo "Directory structure:"
find "$ROOT/directory" -maxdepth 3 -ls 2>/dev/null

echo
echo "Done."