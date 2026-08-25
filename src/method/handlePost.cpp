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


/*
 * ============================================================
 * STRING HELPERS
 * ============================================================
 */

static std::string toLower(const std::string &str)
{
    std::string result = str;

    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = static_cast<char>(
            std::tolower(
                static_cast<unsigned char>(result[i])
            )
        );
    }

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


/*
 * ============================================================
 * NUMBER PARSING
 * ============================================================
 */

static bool parseUnsignedSize(const std::string &str,
                              size_t &value)
{
    if (str.empty())
        return false;

    const size_t maxValue =
        static_cast<size_t>(-1);

    value = 0;

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (!std::isdigit(
                static_cast<unsigned char>(str[i])))
        {
            return false;
        }

        size_t digit =
            static_cast<size_t>(
                str[i] - '0'
            );

        if (value >
            (maxValue - digit) / 10)
        {
            return false;
        }

        value =
            value * 10 + digit;
    }

    return true;
}


static bool parseHexSize(const std::string &str,
                         size_t &value)
{
    if (str.empty())
        return false;

    const size_t maxValue =
        static_cast<size_t>(-1);

    value = 0;

    for (size_t i = 0; i < str.size(); ++i)
    {
        unsigned int digit;

        if (str[i] >= '0' &&
            str[i] <= '9')
        {
            digit =
                static_cast<unsigned int>(
                    str[i] - '0'
                );
        }
        else if (str[i] >= 'a' &&
                 str[i] <= 'f')
        {
            digit =
                static_cast<unsigned int>(
                    str[i] - 'a' + 10
                );
        }
        else if (str[i] >= 'A' &&
                 str[i] <= 'F')
        {
            digit =
                static_cast<unsigned int>(
                    str[i] - 'A' + 10
                );
        }
        else
        {
            return false;
        }

        if (value >
            (maxValue - digit) / 16)
        {
            return false;
        }

        value =
            value * 16 + digit;
    }

    return true;
}


/*
 * ============================================================
 * CHUNKED BODY
 * ============================================================
 */

enum ChunkDecodeResult
{
    CHUNK_DECODE_OK,
    CHUNK_DECODE_MALFORMED,
    CHUNK_DECODE_TOO_LARGE
};


static ChunkDecodeResult decodeChunkedBody(
    const std::string &raw,
    std::string &decoded,
    size_t maxBodySize)
{
    decoded.clear();

    size_t pos = 0;

    while (true)
    {
        size_t lineEnd =
            raw.find("\r\n", pos);

        if (lineEnd == std::string::npos)
            return CHUNK_DECODE_MALFORMED;

        std::string sizeLine =
            raw.substr(
                pos,
                lineEnd - pos
            );

        /*
         * Chunk extension:
         *
         * A;foo=bar
         *
         * =>
         *
         * A
         */
        size_t semicolon =
            sizeLine.find(';');

        if (semicolon != std::string::npos)
        {
            sizeLine =
                sizeLine.substr(
                    0,
                    semicolon
                );
        }

        if (sizeLine.empty())
            return CHUNK_DECODE_MALFORMED;

        size_t chunkSize = 0;

        if (!parseHexSize(
                sizeLine,
                chunkSize))
        {
            return CHUNK_DECODE_MALFORMED;
        }

        pos = lineEnd + 2;

        /*
         * Final chunk.
         */
        if (chunkSize == 0)
        {
            /*
             * 0\r\n\r\n
             */
            if (pos + 2 <= raw.size() &&
                raw.compare(
                    pos,
                    2,
                    "\r\n") == 0)
            {
                return CHUNK_DECODE_OK;
            }

            /*
             * Trailers:
             *
             * 0\r\n
             * X-Test: value\r\n
             * Foo: bar\r\n
             * \r\n
             */
            size_t trailerEnd =
                raw.find(
                    "\r\n\r\n",
                    pos
                );

            if (trailerEnd !=
                std::string::npos)
            {
                return CHUNK_DECODE_OK;
            }

            return CHUNK_DECODE_MALFORMED;
        }

        /*
         * Prevent overflow / incomplete chunk.
         */
        if (pos > raw.size())
            return CHUNK_DECODE_MALFORMED;

        if (chunkSize >
            raw.size() - pos)
        {
            return CHUNK_DECODE_MALFORMED;
        }

        /*
         * Body limit.
         */
        if (maxBodySize > 0)
        {
            if (decoded.size() >
                maxBodySize)
            {
                return CHUNK_DECODE_TOO_LARGE;
            }

            if (chunkSize >
                maxBodySize - decoded.size())
            {
                return CHUNK_DECODE_TOO_LARGE;
            }
        }

        decoded.append(
            raw,
            pos,
            chunkSize
        );

        pos += chunkSize;

        /*
         * Chunk data must finish with CRLF.
         */
        if (pos + 2 > raw.size())
            return CHUNK_DECODE_MALFORMED;

        if (raw.compare(
                pos,
                2,
                "\r\n") != 0)
        {
            return CHUNK_DECODE_MALFORMED;
        }

        pos += 2;
    }
}


static bool isChunkedEncoding(
    const std::string &value)
{
    std::string lower =
        toLower(value);

    return lower.find("chunked") !=
        std::string::npos;
}


static bool containsUnsupportedTransferEncoding(
    const std::string &value)
{
    std::string lower =
        toLower(value);

    size_t start = 0;

    while (start < lower.size())
    {
        size_t comma =
            lower.find(',', start);

        std::string token;

        if (comma == std::string::npos)
            token = lower.substr(start);
        else
            token =
                lower.substr(
                    start,
                    comma - start
                );

        while (!token.empty() &&
               std::isspace(
                   static_cast<unsigned char>(
                       token[0])))
        {
            token.erase(0, 1);
        }

        while (!token.empty() &&
               std::isspace(
                   static_cast<unsigned char>(
                       token[
                           token.size() - 1
                       ])))
        {
            token.erase(
                token.size() - 1
            );
        }

        if (!token.empty() &&
            token != "chunked")
        {
            return true;
        }

        if (comma == std::string::npos)
            break;

        start = comma + 1;
    }

    return false;
}


/*
 * ============================================================
 * FILE SECURITY
 * ============================================================
 */

static bool isSafeFilename(
    const std::string &filename)
{
    if (filename.empty())
        return false;

    if (filename == "." ||
        filename == "..")
    {
        return false;
    }

    if (filename.find("..") !=
        std::string::npos)
    {
        return false;
    }

    if (filename.find('/') !=
        std::string::npos)
    {
        return false;
    }

    if (filename.find('\\') !=
        std::string::npos)
    {
        return false;
    }

    if (filename.find('\0') !=
        std::string::npos)
    {
        return false;
    }

    return true;
}


/*
 * ============================================================
 * MULTIPART BOUNDARY
 * ============================================================
 */

static bool extractBoundary(
    const std::string &contentType,
    std::string &boundary)
{
    boundary.clear();

    std::string lower =
        toLower(contentType);

    size_t pos =
        lower.find("boundary=");

    if (pos == std::string::npos)
        return false;

    pos += 9;

    if (pos >= contentType.size())
        return false;

    /*
     * Quoted boundary.
     */
    if (contentType[pos] == '"')
    {
        ++pos;

        size_t end =
            contentType.find('"', pos);

        if (end == std::string::npos)
            return false;

        boundary =
            contentType.substr(
                pos,
                end - pos
            );
    }
    else
    {
        size_t end =
            contentType.find(';', pos);

        if (end == std::string::npos)
        {
            boundary =
                contentType.substr(pos);
        }
        else
        {
            boundary =
                contentType.substr(
                    pos,
                    end - pos
                );
        }

        /*
         * Trim trailing spaces.
         */
        while (!boundary.empty() &&
               std::isspace(
                   static_cast<unsigned char>(
                       boundary[
                           boundary.size() - 1
                       ])))
        {
            boundary.erase(
                boundary.size() - 1
            );
        }

        /*
         * Trim leading spaces.
         */
        while (!boundary.empty() &&
               std::isspace(
                   static_cast<unsigned char>(
                       boundary[0])))
        {
            boundary.erase(0, 1);
        }
    }

    return !boundary.empty();
}


/*
 * Search ONLY for a real multipart delimiter.
 *
 * Valid:
 *
 * --BOUNDARY\r\n
 *
 * \r\n--BOUNDARY\r\n
 *
 * \r\n--BOUNDARY--
 *
 *
 * Invalid / ignored:
 *
 * hello--BOUNDARYworld
 *
 * \r\n--BOUNDARY-FAKE
 *
 * --BOUNDARYXYZ
 */
static size_t findMultipartBoundary(
    const std::string &body,
    const std::string &delimiter,
    size_t start)
{
    size_t pos = start;

    while (true)
    {
        pos =
            body.find(
                delimiter,
                pos
            );

        if (pos == std::string::npos)
            return std::string::npos;

        /*
         * Boundary must start at beginning
         * or after CRLF.
         */
        bool validBefore = false;

        if (pos == 0)
        {
            validBefore = true;
        }
        else if (pos >= 2 &&
                 body[pos - 2] == '\r' &&
                 body[pos - 1] == '\n')
        {
            validBefore = true;
        }

        size_t after =
            pos + delimiter.size();

        bool validAfter = false;

        /*
         * Normal delimiter:
         *
         * --BOUNDARY\r\n
         */
        if (after + 2 <= body.size() &&
            body.compare(
                after,
                2,
                "\r\n") == 0)
        {
            validAfter = true;
        }

        /*
         * Closing delimiter:
         *
         * --BOUNDARY--
         */
        else if (after + 2 <= body.size() &&
                 body.compare(
                     after,
                     2,
                     "--") == 0)
        {
            size_t closingEnd =
                after + 2;

            /*
             * Closing boundary should either
             * finish the body or be followed
             * by CRLF.
             */
            if (closingEnd ==
                body.size())
            {
                validAfter = true;
            }
            else if (closingEnd + 2 <=
                         body.size() &&
                     body.compare(
                         closingEnd,
                         2,
                         "\r\n") == 0)
            {
                validAfter = true;
            }
        }

        if (validBefore &&
            validAfter)
        {
            return pos;
        }

        /*
         * Fake boundary inside file body.
         */
        pos += delimiter.size();
    }
}


/*
 * ============================================================
 * MULTIPART HEADERS
 * ============================================================
 */

static std::string getMultipartFilename(
    const std::string &headers)
{
    std::string lower =
        toLower(headers);

    size_t pos =
        lower.find("filename=\"");

    if (pos == std::string::npos)
        return "";

    pos += 10;

    size_t end =
        headers.find('"', pos);

    if (end == std::string::npos)
        return "";

    return headers.substr(
        pos,
        end - pos
    );
}


/*
 * ============================================================
 * SAVE MULTIPART FILES
 * ============================================================
 */

static bool saveMultipartFiles(
    const std::string &body,
    const std::string &boundary,
    const std::string &baseDir,
    size_t &filesSaved)
{
    filesSaved = 0;

    const std::string delimiter =
        "--" + boundary;

    /*
     * Find FIRST VALID boundary.
     *
     * Don't use body.find() directly.
     */
    size_t pos =
        findMultipartBoundary(
            body,
            delimiter,
            0
        );

    if (pos == std::string::npos)
        return false;


    while (pos != std::string::npos)
    {
        size_t boundaryPos = pos;

        size_t afterBoundary =
            boundaryPos +
            delimiter.size();

        /*
         * Closing boundary.
         *
         * --BOUNDARY--
         */
        if (afterBoundary + 2 <=
                body.size() &&
            body.compare(
                afterBoundary,
                2,
                "--") == 0)
        {
            break;
        }

        /*
         * Normal part boundary must be:
         *
         * --BOUNDARY\r\n
         */
        if (afterBoundary + 2 >
                body.size() ||
            body.compare(
                afterBoundary,
                2,
                "\r\n") != 0)
        {
            return false;
        }

        size_t headersStart =
            afterBoundary + 2;

        size_t headersEnd =
            body.find(
                "\r\n\r\n",
                headersStart
            );

        if (headersEnd ==
            std::string::npos)
        {
            return false;
        }

        std::string partHeaders =
            body.substr(
                headersStart,
                headersEnd -
                headersStart
            );

        size_t contentStart =
            headersEnd + 4;

        /*
         * Find NEXT REAL boundary.
         *
         * Fake boundary-like strings inside
         * file content are ignored.
         */
        size_t nextBoundary =
            findMultipartBoundary(
                body,
                delimiter,
                contentStart
            );

        if (nextBoundary ==
            std::string::npos)
        {
            return false;
        }

        /*
         * A real multipart delimiter is normally:
         *
         * file-data\r\n--BOUNDARY
         *
         * Remove that CRLF from file content.
         */
        size_t contentEnd =
            nextBoundary;

        if (contentEnd >= 2 &&
            body[contentEnd - 2] == '\r' &&
            body[contentEnd - 1] == '\n')
        {
            contentEnd -= 2;
        }

        if (contentEnd < contentStart)
            return false;

        std::string filename =
            getMultipartFilename(
                partHeaders
            );

        /*
         * Multipart part may be a normal
         * form field and not a file.
         */
        if (!filename.empty())
        {
            if (!isSafeFilename(filename))
                return false;

            std::string target =
                baseDir;

            if (!target.empty() &&
                target[
                    target.size() - 1
                ] != '/')
            {
                target += '/';
            }

            target += filename;

            /*
             * Do not overwrite directory
             * or symlink.
             */
            if (isDirectory(target) ||
                isSymlink(target))
            {
                return false;
            }

            std::ofstream file(
                target.c_str(),
                std::ios::binary |
                std::ios::out
            );

            if (!file.is_open())
                return false;

            if (contentEnd > contentStart)
            {
                file.write(
                    body.data() +
                    contentStart,
                    static_cast<
                        std::streamsize
                    >(
                        contentEnd -
                        contentStart
                    )
                );
            }

            if (!file.good())
            {
                file.close();
                return false;
            }

            file.close();

            ++filesSaved;
        }

        /*
         * IMPORTANT:
         *
         * Process nextBoundary itself.
         *
         * Do NOT:
         *
         * pos = nextBoundary + 2;
         */
        pos = nextBoundary;
    }

    return filesSaved > 0;
}


/*
 * ============================================================
 * BODY LIMIT
 * ============================================================
 */

static size_t getBodyLimit(
    const ServerConfig &serv,
    const LocationConfig &loc)
{
    if (loc.has_client_max_body_size)
        return loc.client_max_body_size;

    return serv.client_max_body_size;
}


/*
 * ============================================================
 * POST
 * ============================================================
 */

std::string handlePOST(
    const HttpRequest &req,
    const ServerConfig &serv)
{
    /*
     * 1. Path.
     */
    if (req.path.empty())
        return errorResponse(
            400,
            "",
            &serv
        );

    std::string cleanPath =
        normalizePath(req.path);

    if (cleanPath.empty())
        return errorResponse(
            400,
            "",
            &serv
        );


    /*
     * 2. Location.
     */
    LocationConfig loc =
        serv.matchLocation(cleanPath);

    if (loc.path.empty())
    {
        return errorResponse(
            404,
            "",
            &serv
        );
    }

    size_t maxBodySize =
        getBodyLimit(
            serv,
            loc
        );


    /*
     * 3. Body framing headers.
     */
    std::string contentLengthHeader;
    std::string transferEncodingHeader;

    bool hasContentLength =
        getHeader(
            req,
            "Content-Length",
            contentLengthHeader
        );

    bool hasTransferEncoding =
        getHeader(
            req,
            "Transfer-Encoding",
            transferEncodingHeader
        );


    /*
     * Client request containing BOTH is invalid.
     *
     * With fixed multiplexing:
     *
     * decoded chunked requests should have
     * Transfer-Encoding removed and a new
     * Content-Length inserted.
     */
    if (hasContentLength &&
        hasTransferEncoding)
    {
        return errorResponse(
            400,
            "",
            &serv
        );
    }


    if (!hasContentLength &&
        !hasTransferEncoding)
    {
        return errorResponse(
            411,
            "",
            &serv
        );
    }


    std::string body;


    /*
     * ========================================================
     * 4. RAW CHUNKED BODY
     *
     * This remains as fallback.
     *
     * Normally multiplexing.cpp already
     * unchunks before dispatchRequest().
     * ========================================================
     */
    if (hasTransferEncoding)
    {
        if (containsUnsupportedTransferEncoding(
                transferEncodingHeader))
        {
            return errorResponse(
                501,
                "",
                &serv
            );
        }

        if (!isChunkedEncoding(
                transferEncodingHeader))
        {
            return errorResponse(
                501,
                "",
                &serv
            );
        }

        ChunkDecodeResult decodeResult =
            decodeChunkedBody(
                req.body,
                body,
                maxBodySize
            );

        if (decodeResult ==
            CHUNK_DECODE_TOO_LARGE)
        {
            return errorResponse(
                413,
                "",
                &serv
            );
        }

        if (decodeResult !=
            CHUNK_DECODE_OK)
        {
            return errorResponse(
                400,
                "",
                &serv
            );
        }

        if (maxBodySize > 0 &&
            body.size() >
            maxBodySize)
        {
            return errorResponse(
                413,
                "",
                &serv
            );
        }
    }


    /*
     * ========================================================
     * 5. CONTENT-LENGTH BODY
     * ========================================================
     */
    else
    {
        size_t contentLength = 0;

        if (!parseUnsignedSize(
                contentLengthHeader,
                contentLength))
        {
            return errorResponse(
                400,
                "",
                &serv
            );
        }

        /*
         * Equal to limit is allowed.
         */
        if (maxBodySize > 0 &&
            contentLength >
            maxBodySize)
        {
            return errorResponse(
                413,
                "",
                &serv
            );
        }

        if (req.body.size() !=
            contentLength)
        {
            return errorResponse(
                400,
                "",
                &serv
            );
        }

        body = req.body;
    }


    /*
     * ========================================================
     * 6. DESTINATION
     * ========================================================
     */
    std::string baseDir;

    if (loc.allow_upload)
    {
        if (loc.upload_path.empty())
        {
            return errorResponse(
                500,
                "",
                &serv
            );
        }

        baseDir =
            loc.upload_path;

        if (!isDirectory(baseDir))
        {
            return errorResponse(
                500,
                "",
                &serv
            );
        }
    }
    else
    {
        baseDir =
            buildPath(
                cleanPath,
                loc
            );
    }


    /*
     * ========================================================
     * 7. CONTENT-TYPE
     * ========================================================
     */
    std::string contentType;

    bool hasContentType =
        getHeader(
            req,
            "Content-Type",
            contentType
        );


    /*
     * ========================================================
     * 8. MULTIPART
     * ========================================================
     */
    if (hasContentType &&
        toLower(contentType).find(
            "multipart/form-data") !=
            std::string::npos)
    {
        std::string boundary;

        if (!extractBoundary(
                contentType,
                boundary))
        {
            return errorResponse(
                400,
                "",
                &serv
            );
        }

        size_t filesSaved = 0;

        std::string multipartDir =
            baseDir;

        /*
         * If URI points to a file path,
         * use parent directory.
         */
        if (!isDirectory(
                multipartDir))
        {
            size_t slash =
                multipartDir.find_last_of('/');

            if (slash !=
                std::string::npos)
            {
                multipartDir =
                    multipartDir.substr(
                        0,
                        slash
                    );
            }
        }

        if (multipartDir.empty() ||
            !isDirectory(
                multipartDir))
        {
            return errorResponse(
                500,
                "",
                &serv
            );
        }

        if (!saveMultipartFiles(
                body,
                boundary,
                multipartDir,
                filesSaved))
        {
            return errorResponse(
                400,
                "",
                &serv
            );
        }

        return buildResponse(
            201,
            "",
            "text/plain"
        );
    }


    /*
     * ========================================================
     * 9. NORMAL POST FILE
     * ========================================================
     */
    std::string name;

    size_t slash =
        cleanPath.find_last_of('/');

    if (slash == std::string::npos)
    {
        name = cleanPath;
    }
    else
    {
        name =
            cleanPath.substr(
                slash + 1
            );
    }


    bool pathEndsWithSlash =
        !req.path.empty() &&
        req.path[
            req.path.size() - 1
        ] == '/';


    bool locationDirectoryRequest =
        !loc.path.empty() &&
        cleanPath ==
        normalizePath(loc.path);


    bool baseIsDirectory =
        isDirectory(baseDir);


    /*
     * POST directly to directory.
     *
     * Generate filename.
     */
    if (name.empty() ||
        pathEndsWithSlash ||
        (locationDirectoryRequest &&
         baseIsDirectory))
    {
        std::ostringstream generated;

        generated
            << time(NULL)
            << "_"
            << getpid()
            << "_"
            << std::rand()
            << ".txt";

        name =
            generated.str();
    }


    if (!isSafeFilename(name))
    {
        return errorResponse(
            400,
            "",
            &serv
        );
    }


    /*
     * ========================================================
     * 10. TARGET
     * ========================================================
     */
    std::string target;

    if (loc.allow_upload)
    {
        target = baseDir;

        if (!target.empty() &&
            target[
                target.size() - 1
            ] != '/')
        {
            target += '/';
        }

        target += name;
    }
    else
    {
        if (isDirectory(baseDir))
        {
            target =
                baseDir;

            if (!target.empty() &&
                target[
                    target.size() - 1
                ] != '/')
            {
                target += '/';
            }

            target += name;
        }
        else
        {
            target =
                baseDir;
        }
    }


    if (target.empty())
    {
        return errorResponse(
            400,
            "",
            &serv
        );
    }


    if (isDirectory(target) ||
        isSymlink(target))
    {
        return errorResponse(
            403,
            "",
            &serv
        );
    }


    /*
     * ========================================================
     * 11. WRITE FILE
     * ========================================================
     */
    std::ofstream file(
        target.c_str(),
        std::ios::binary |
        std::ios::out
    );

    if (!file.is_open())
    {
        return errorResponse(
            500,
            "",
            &serv
        );
    }

    if (!body.empty())
    {
        file.write(
            body.data(),
            static_cast<
                std::streamsize
            >(
                body.size()
            )
        );
    }

    if (!file.good())
    {
        file.close();

        return errorResponse(
            500,
            "",
            &serv
        );
    }

    file.close();


    /*
     * ========================================================
     * 12. SUCCESS
     * ========================================================
     */
    return buildResponse(
        201,
        "",
        "text/plain"
    );
}