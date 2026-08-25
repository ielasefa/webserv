/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlePost.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/03 00:40:37 by iel-asef          #+#    #+#             */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"

static std::string toLower(const std::string &str)
{
    std::string result = str;

    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(result[i])));

    return result;
}

static bool getHeader(const HttpRequest &req,
                      const std::string &name,
                      std::string &value)
{
    std::string wanted = toLower(name);

    for (std::map<std::string, std::string>::const_iterator it =
             req.headers.begin();
         it != req.headers.end();
         ++it)
    {
        if (toLower(it->first) == wanted)
        {
            value = it->second;
            return true;
        }
    }

    return false;
}

static bool parseUnsignedSize(const std::string &str, size_t &value)
{
    if (str.empty())
        return false;

    const size_t maxValue = static_cast<size_t>(-1);
    value = 0;

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(str[i])))
            return false;

        size_t digit = static_cast<size_t>(str[i] - '0');

        if (value > (maxValue - digit) / 10)
            return false;

        value = value * 10 + digit;
    }

    return true;
}

static bool parseHexSize(const std::string &str, size_t &value)
{
    if (str.empty())
        return false;

    const size_t maxValue = static_cast<size_t>(-1);
    value = 0;

    for (size_t i = 0; i < str.size(); ++i)
    {
        unsigned int digit;

        if (str[i] >= '0' && str[i] <= '9')
            digit = static_cast<unsigned int>(str[i] - '0');
        else if (str[i] >= 'a' && str[i] <= 'f')
            digit = static_cast<unsigned int>(str[i] - 'a' + 10);
        else if (str[i] >= 'A' && str[i] <= 'F')
            digit = static_cast<unsigned int>(str[i] - 'A' + 10);
        else
            return false;

        if (value > (maxValue - digit) / 16)
            return false;

        value = value * 16 + digit;
    }

    return true;
}

enum ChunkDecodeResult
{
    CHUNK_DECODE_OK,
    CHUNK_DECODE_MALFORMED,
    CHUNK_DECODE_TOO_LARGE
};

static ChunkDecodeResult decodeChunkedBody(const std::string &raw,
                                           std::string &decoded,
                                           size_t maxBodySize)
{
    decoded.clear();

    size_t pos = 0;

    while (true)
    {
        size_t lineEnd = raw.find("\r\n", pos);

        if (lineEnd == std::string::npos)
            return CHUNK_DECODE_MALFORMED;

        std::string sizeLine = raw.substr(pos, lineEnd - pos);

        size_t semicolon = sizeLine.find(';');

        if (semicolon != std::string::npos)
            sizeLine = sizeLine.substr(0, semicolon);

        if (sizeLine.empty())
            return CHUNK_DECODE_MALFORMED;

        size_t chunkSize = 0;

        if (!parseHexSize(sizeLine, chunkSize))
            return CHUNK_DECODE_MALFORMED;

        pos = lineEnd + 2;

        if (chunkSize == 0)
        {
            
            if (pos == raw.size())
                return CHUNK_DECODE_OK;

            size_t trailerEnd = raw.find("\r\n\r\n", pos);

            if (trailerEnd != std::string::npos)
                return CHUNK_DECODE_OK;

            if (pos + 2 == raw.size() &&
                raw.compare(pos, 2, "\r\n") == 0)
                return CHUNK_DECODE_OK;

            return CHUNK_DECODE_MALFORMED;
        }

        if (chunkSize > raw.size() - pos)
            return CHUNK_DECODE_MALFORMED;

        if (maxBodySize > 0)
        {
            if (decoded.size() > maxBodySize)
                return CHUNK_DECODE_TOO_LARGE;

            if (chunkSize > maxBodySize - decoded.size())
                return CHUNK_DECODE_TOO_LARGE;
        }

        decoded.append(raw, pos, chunkSize);

        pos += chunkSize;

        if (pos + 2 > raw.size())
            return CHUNK_DECODE_MALFORMED;

        if (raw.compare(pos, 2, "\r\n") != 0)
            return CHUNK_DECODE_MALFORMED;

        pos += 2;
    }
}

static bool isChunkedEncoding(const std::string &value)
{
    std::string lower = toLower(value);

    return lower.find("chunked") != std::string::npos;
}

static bool containsUnsupportedTransferEncoding(const std::string &value)
{
    std::string lower = toLower(value);

    
    size_t start = 0;

    while (start < lower.size())
    {
        size_t comma = lower.find(',', start);

        std::string token;

        if (comma == std::string::npos)
            token = lower.substr(start);
        else
            token = lower.substr(start, comma - start);

        while (!token.empty() &&
               std::isspace(static_cast<unsigned char>(token[0])))
            token.erase(0, 1);

        while (!token.empty() &&
               std::isspace(static_cast<unsigned char>(
                   token[token.size() - 1])))
            token.erase(token.size() - 1);

        if (!token.empty() && token != "chunked")
            return true;

        if (comma == std::string::npos)
            break;

        start = comma + 1;
    }

    return false;
}

static bool isSafeFilename(const std::string &filename)
{
    if (filename.empty())
        return false;

    if (filename == "." || filename == "..")
        return false;

    if (filename.find("..") != std::string::npos)
        return false;

    if (filename.find('/') != std::string::npos)
        return false;

    if (filename.find('\\') != std::string::npos)
        return false;

    if (filename.find('\0') != std::string::npos)
        return false;

    return true;
}

static bool extractBoundary(const std::string &contentType,
                            std::string &boundary)
{
    boundary.clear();

    std::string lower = toLower(contentType);

    size_t pos = lower.find("boundary=");

    if (pos == std::string::npos)
        return false;

    pos += 9;

    if (pos >= contentType.size())
        return false;

    if (contentType[pos] == '"')
    {
        ++pos;

        size_t end = contentType.find('"', pos);

        if (end == std::string::npos)
            return false;

        boundary = contentType.substr(pos, end - pos);
    }
    else
    {
        size_t end = contentType.find(';', pos);

        if (end == std::string::npos)
            boundary = contentType.substr(pos);
        else
            boundary = contentType.substr(pos, end - pos);

        while (!boundary.empty() &&
               std::isspace(static_cast<unsigned char>(
                   boundary[boundary.size() - 1])))
            boundary.erase(boundary.size() - 1);
    }

    return !boundary.empty();
}

static std::string getMultipartFilename(const std::string &headers)
{
    std::string lower = toLower(headers);

    size_t pos = lower.find("filename=\"");

    if (pos == std::string::npos)
        return "";

    pos += 10;

    size_t end = headers.find('"', pos);

    if (end == std::string::npos)
        return "";

    return headers.substr(pos, end - pos);
}

static bool saveMultipartFiles(const std::string &body,
                               const std::string &boundary,
                               const std::string &baseDir,
                               size_t &filesSaved)
{
    filesSaved = 0;

    const std::string delimiter = "--" + boundary;

    size_t pos = 0;

    while (true)
    {
        size_t boundaryPos = body.find(delimiter, pos);

        if (boundaryPos == std::string::npos)
            break;

        size_t afterBoundary = boundaryPos + delimiter.size();

        if (afterBoundary + 2 <= body.size() &&
            body.compare(afterBoundary, 2, "--") == 0)
            break;

        if (afterBoundary + 2 > body.size() ||
            body.compare(afterBoundary, 2, "\r\n") != 0)
            return false;

        size_t headersStart = afterBoundary + 2;

        size_t headersEnd = body.find("\r\n\r\n", headersStart);

        if (headersEnd == std::string::npos)
            return false;

        std::string partHeaders =
            body.substr(headersStart, headersEnd - headersStart);

        size_t contentStart = headersEnd + 4;

        size_t nextBoundary =
            body.find("\r\n" + delimiter, contentStart);

        if (nextBoundary == std::string::npos)
            return false;

        std::string filename =
            getMultipartFilename(partHeaders);

        if (!filename.empty())
        {
            if (!isSafeFilename(filename))
                return false;

            std::string target = baseDir;

            if (!target.empty() &&
                target[target.size() - 1] != '/')
                target += '/';

            target += filename;

            if (isDirectory(target) || isSymlink(target))
                return false;

            std::ofstream file(
                target.c_str(),
                std::ios::binary | std::ios::out);

            if (!file.is_open())
                return false;

            file.write(
                body.data() + contentStart,
                static_cast<std::streamsize>(
                    nextBoundary - contentStart));

            if (!file.good())
            {
                file.close();
                return false;
            }

            file.close();

            ++filesSaved;
        }

        pos = nextBoundary + 2;
    }
    return filesSaved > 0;
}

static size_t getBodyLimit(const ServerConfig &serv,
                           const LocationConfig &loc)
{
    if (loc.has_client_max_body_size)
        return loc.client_max_body_size;

    return serv.client_max_body_size;
}

std::string handlePOST(const HttpRequest &req,
                       const ServerConfig &serv)
{
    /*
     * 1. Basic path validation
     */
    if (req.path.empty())
        return errorResponse(400, "", &serv);

    if (req.path.find("..") != std::string::npos)
        return errorResponse(403, "", &serv);

    std::string cleanPath = normalizePath(req.path);

    if (cleanPath.empty())
        return errorResponse(400, "", &serv);

    /*
     * 2. Match location and get body limit
     */
    LocationConfig loc = serv.matchLocation(cleanPath);

    size_t maxBodySize = getBodyLimit(serv, loc);

    /*
     * 3. Content-Length / Transfer-Encoding
     */
    std::string contentLengthHeader;
    std::string transferEncodingHeader;

    bool hasContentLength =
        getHeader(req, "Content-Length", contentLengthHeader);

    bool hasTransferEncoding =
        getHeader(req, "Transfer-Encoding", transferEncodingHeader);

    /*
     * Having both is invalid.
     */
    if (hasContentLength && hasTransferEncoding)
        return errorResponse(400, "", &serv);

    /*
     * POST body length must be known.
     */
    if (!hasContentLength && !hasTransferEncoding)
        return errorResponse(411, "", &serv);

    std::string body;

    /*
     * 4. Chunked body
     */
    if (hasTransferEncoding)
    {
        if (containsUnsupportedTransferEncoding(
                transferEncodingHeader))
        {
            return errorResponse(501, "", &serv);
        }

        if (!isChunkedEncoding(transferEncodingHeader))
            return errorResponse(501, "", &serv);

        ChunkDecodeResult decodeResult =
            decodeChunkedBody(
                req.body,
                body,
                maxBodySize
            );

        if (decodeResult == CHUNK_DECODE_TOO_LARGE)
            return errorResponse(413, "", &serv);

        if (decodeResult != CHUNK_DECODE_OK)
            return errorResponse(400, "", &serv);

        /*
         * IMPORTANT:
         * Equal to the limit is valid.
         *
         * 2048 > 2048 => false
         */
        if (maxBodySize > 0 &&
            body.size() > maxBodySize)
        {
            return errorResponse(413, "", &serv);
        }
    }

    /*
     * 5. Content-Length body
     */
    else
    {
        size_t contentLength = 0;

        if (!parseUnsignedSize(
                contentLengthHeader,
                contentLength))
        {
            return errorResponse(400, "", &serv);
        }

        /*
         * IMPORTANT:
         * Use > and NOT >=
         *
         * limit = 2048
         * body  = 2048
         * => accepted
         */
        if (maxBodySize > 0 &&
            contentLength > maxBodySize)
        {
            return errorResponse(413, "", &serv);
        }

        /*
         * Received body must match Content-Length.
         */
        if (req.body.size() != contentLength)
            return errorResponse(400, "", &serv);

        body = req.body;
    }

    /*
     * 6. Determine destination
     */
    std::string baseDir;

    if (loc.allow_upload)
    {
        if (loc.upload_path.empty())
            return errorResponse(500, "", &serv);

        baseDir = loc.upload_path;

        if (!isDirectory(baseDir))
            return errorResponse(500, "", &serv);
    }
    else
    {
        baseDir = buildPath(cleanPath, loc);
    }

    /*
     * 7. Content-Type
     */
    std::string contentType;

    bool hasContentType =
        getHeader(req, "Content-Type", contentType);

    /*
     * 8. Multipart upload
     */
    if (hasContentType &&
        toLower(contentType).find(
            "multipart/form-data") != std::string::npos)
    {
        std::string boundary;

        if (!extractBoundary(contentType, boundary))
            return errorResponse(400, "", &serv);

        size_t filesSaved = 0;

        /*
         * Multipart needs a directory.
         */
        std::string multipartDir = baseDir;

        if (!isDirectory(multipartDir))
        {
            size_t slash = multipartDir.find_last_of('/');

            if (slash != std::string::npos)
                multipartDir =
                    multipartDir.substr(0, slash);
        }

        if (multipartDir.empty() ||
            !isDirectory(multipartDir))
        {
            return errorResponse(500, "", &serv);
        }

        if (!saveMultipartFiles(
                body,
                boundary,
                multipartDir,
                filesSaved))
        {
            return errorResponse(400, "", &serv);
        }

        return buildResponse(
            201,
            "",
            "text/plain"
        );
    }

    /*
     * 9. Determine filename
     */
    std::string name;

    size_t slash = cleanPath.find_last_of('/');

    if (slash == std::string::npos)
        name = cleanPath;
    else
        name = cleanPath.substr(slash + 1);

    /*
     * Detect POST directly on a directory:
     *
     * POST /
     * POST /upload/
     * POST /upload
     *
     * In these cases we generate a filename instead
     * of returning 405.
     */
    bool pathEndsWithSlash =
        !req.path.empty() &&
        req.path[req.path.size() - 1] == '/';

    bool locationDirectoryRequest =
        !loc.path.empty() &&
        cleanPath == normalizePath(loc.path);

    bool baseIsDirectory =
        isDirectory(baseDir);

    if (name.empty() ||
        pathEndsWithSlash ||
        (locationDirectoryRequest && baseIsDirectory))
    {
        std::ostringstream generated;

        generated
            << time(NULL)
            << "_"
            << getpid()
            << "_"
            << std::rand()
            << ".txt";

        name = generated.str();
    }

    if (!isSafeFilename(name))
        return errorResponse(400, "", &serv);

    /*
     * 10. Build final target
     */
    std::string target;

    if (loc.allow_upload)
    {
        target = baseDir;

        if (!target.empty() &&
            target[target.size() - 1] != '/')
        {
            target += '/';
        }

        target += name;
    }
    else
    {
        /*
         * If buildPath() points to a directory,
         * save the generated/requested file inside it.
         */
        if (isDirectory(baseDir))
        {
            target = baseDir;

            if (!target.empty() &&
                target[target.size() - 1] != '/')
            {
                target += '/';
            }

            target += name;
        }
        else
        {
            /*
             * URI already represents a file.
             */
            target = baseDir;
        }
    }

    if (target.empty())
        return errorResponse(400, "", &serv);

    /*
     * Do not overwrite a directory or symlink.
     */
    if (isDirectory(target) ||
        isSymlink(target))
    {
        return errorResponse(403, "", &serv);
    }

    /*
     * 11. Create/write file
     */
    std::ofstream file(
        target.c_str(),
        std::ios::binary | std::ios::out
    );

    if (!file.is_open())
        return errorResponse(500, "", &serv);

    if (!body.empty())
    {
        file.write(
            body.data(),
            static_cast<std::streamsize>(
                body.size()
            )
        );
    }

    if (!file.good())
    {
        file.close();
        return errorResponse(500, "", &serv);
    }

    file.close();

    /*
     * 12. POST success
     */
    return buildResponse(
        201,
        "",
        "text/plain"
    );
}