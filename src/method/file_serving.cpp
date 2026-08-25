/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file_serving.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: iel-asef <iel-asef@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/02 21:40:26 by iel-asef          #+#    #+#             */
/*   Updated: 2026/08/25 16:55:00 by iel-asef         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../webserv.hpp"


/*
 * ============================================================
 * FILESYSTEM HELPERS
 * ============================================================
 */

bool isSymlink(const std::string& path)
{
    struct stat s;

    if (lstat(path.c_str(), &s) != 0)
        return false;

    return S_ISLNK(s.st_mode);
}


static bool hasReadPermission(const struct stat& s)
{
    return (
        s.st_mode &
        (S_IRUSR | S_IRGRP | S_IROTH)
    ) != 0;
}


static bool hasExecutePermission(const struct stat& s)
{
    return (
        s.st_mode &
        (S_IXUSR | S_IXGRP | S_IXOTH)
    ) != 0;
}


/*
 * Detect whether this location has an index
 * different from the server default.
 *
 * Example:
 *
 * server:
 *     index index.htm;
 *
 * location /directory/Yeah/:
 *     index youpi.bad_extension;
 *
 * => location-specific index.
 *
 *
 * /empty_dir inherits index.htm from server:
 * => NOT treated as a special explicit index.
 */
static bool hasLocationSpecificIndex(
    const LocationConfig& loc,
    const ServerConfig* config)
{
    if (loc.index.empty())
        return false;

    if (config == NULL)
        return true;

    if (config->index.empty())
        return true;

    return loc.index != config->index;
}


/*
 * ============================================================
 * REDIRECT
 * ============================================================
 */

std::string redirect301(const std::string& to)
{
    std::vector<std::string> headers;

    headers.push_back(
        "Location: " + to
    );

    return buildResponse(
        301,
        "",
        "text/plain",
        headers
    );
}


/*
 * ============================================================
 * SERVE REGULAR FILE
 * ============================================================
 */

std::string serveFile(
    const std::string& path,
    const ServerConfig* config)
{
    /*
     * Symlink is forbidden.
     */
    if (isSymlink(path))
    {
        return errorResponse(
            403,
            "",
            config
        );
    }

    struct stat fileStat;

    /*
     * Check actual filesystem object.
     */
    if (stat(path.c_str(), &fileStat) != 0)
    {
        /*
         * Permission denied while resolving path.
         */
        if (errno == EACCES ||
            errno == EPERM)
        {
            return errorResponse(
                403,
                "",
                config
            );
        }

        return errorResponse(
            404,
            "",
            config
        );
    }

    /*
     * serveFile() only serves regular files.
     */
    if (!S_ISREG(fileStat.st_mode))
    {
        return errorResponse(
            404,
            "",
            config
        );
    }

    /*
     * chmod 000 file.
     */
    if (!hasReadPermission(fileStat))
    {
        return errorResponse(
            403,
            "",
            config
        );
    }

    std::string content =
        readFile(path);

    return buildResponse(
        200,
        content,
        getMimeType(path)
    );
}


/*
 * ============================================================
 * GET HANDLER
 * ============================================================
 */

std::string handleRequest(
    const HttpRequest& req,
    const LocationConfig& loc,
    const ServerConfig* config)
{
    /*
     * Request path should already be decoded +
     * normalized by dispatchRequest().
     *
     * Normalize again for safety.
     */
    std::string cleanPath =
        normalizePath(req.path);

    if (cleanPath.empty())
    {
        return errorResponse(
            400,
            "",
            config
        );
    }


    bool hasSlash =
        !req.path.empty() &&
        req.path[
            req.path.size() - 1
        ] == '/';


    std::string fullPath;


    /*
     * ========================================================
     * UPLOAD LOCATION
     * ========================================================
     */

    if (loc.allow_upload &&
        !loc.upload_path.empty())
    {
        std::string relative =
            cleanPath;

        /*
         * Remove location prefix.
         *
         * /upload/test.txt
         * =>
         * test.txt
         */
        if (loc.path != "/" &&
            relative.compare(
                0,
                loc.path.length(),
                loc.path
            ) == 0)
        {
            relative =
                relative.substr(
                    loc.path.length()
                );
        }

        while (!relative.empty() &&
               relative[0] == '/')
        {
            relative.erase(0, 1);
        }

        fullPath =
            loc.upload_path;

        if (!fullPath.empty() &&
            fullPath[
                fullPath.size() - 1
            ] != '/')
        {
            fullPath += '/';
        }

        fullPath += relative;
    }


    /*
     * ========================================================
     * NORMAL LOCATION
     * ========================================================
     */

    else
    {
        fullPath =
            buildPath(
                cleanPath,
                loc
            );
    }


    struct stat pathStat;


    /*
     * ========================================================
     * SYMLINK
     * ========================================================
     */

    if (lstat(
            fullPath.c_str(),
            &pathStat) == 0 &&
        S_ISLNK(pathStat.st_mode))
    {
        return errorResponse(
            403,
            "",
            config
        );
    }


    /*
     * ========================================================
     * PATH EXISTS?
     * ========================================================
     */

    if (stat(
            fullPath.c_str(),
            &pathStat) != 0)
    {
        /*
         * Existing path but inaccessible.
         */
        if (errno == EACCES ||
            errno == EPERM)
        {
            return errorResponse(
                403,
                "",
                config
            );
        }

        return errorResponse(
            404,
            "",
            config
        );
    }


    /*
     * ========================================================
     * REGULAR FILE
     * ========================================================
     */

    if (S_ISREG(pathStat.st_mode))
    {
        /*
         * chmod 000 file.
         */
        if (!hasReadPermission(pathStat))
        {
            return errorResponse(
                403,
                "",
                config
            );
        }

        return serveFile(
            fullPath,
            config
        );
    }


    /*
     * ========================================================
     * DIRECTORY
     * ========================================================
     */

    if (S_ISDIR(pathStat.st_mode))
    {
        /*
         * chmod 000 directory.
         *
         * Directory needs:
         * - read permission
         * - execute/search permission
         */
        if (!hasReadPermission(pathStat) ||
            !hasExecutePermission(pathStat))
        {
            return errorResponse(
                403,
                "",
                config
            );
        }


        /*
         * Directory URI requires trailing slash.
         *
         * /directory
         *
         * =>
         *
         * /directory/
         */
        if (!hasSlash &&
            cleanPath != "/")
        {
            return redirect301(
                cleanPath + "/"
            );
        }


        /*
         * ====================================================
         * UPLOAD DIRECTORY
         * ====================================================
         */

        if (loc.allow_upload &&
            !loc.upload_path.empty())
        {
            if (!loc.autoindex)
            {
                return errorResponse(
                    403,
                    "",
                    config
                );
            }

            std::vector<std::string> files =
                readDirectory(fullPath);

            return generateAutoIndex(
                cleanPath,
                fullPath,
                files
            );
        }


        /*
         * ====================================================
         * INDEX FILE
         * ====================================================
         */

        std::string indexPath =
            buildPath(
                cleanPath,
                loc,
                true
            );


        /*
         * Index exists.
         */
        if (isFile(indexPath))
        {
            struct stat indexStat;

            if (stat(
                    indexPath.c_str(),
                    &indexStat) != 0)
            {
                if (errno == EACCES ||
                    errno == EPERM)
                {
                    return errorResponse(
                        403,
                        "",
                        config
                    );
                }

                return errorResponse(
                    404,
                    "",
                    config
                );
            }


            /*
             * chmod 000 index file.
             */
            if (!hasReadPermission(indexStat))
            {
                return errorResponse(
                    403,
                    "",
                    config
                );
            }


            return serveFile(
                indexPath,
                config
            );
        }


        /*
         * ====================================================
         * LOCATION-SPECIFIC INDEX MISSING
         * ====================================================
         *
         * Example:
         *
         * server {
         *     index index.htm;
         *
         *     location /directory/Yeah/ {
         *         index youpi.bad_extension;
         *     }
         * }
         *
         * /directory/Yeah/ exists,
         * but youpi.bad_extension does not exist.
         *
         * => 404
         */
        if (hasLocationSpecificIndex(
                loc,
                config))
        {
            return errorResponse(
                404,
                "",
                config
            );
        }


        /*
         * ====================================================
         * NO INDEX + AUTOINDEX OFF
         * ====================================================
         *
         * /empty_dir/
         *
         * directory exists
         * no index
         * autoindex off
         *
         * => 403
         */
        if (!loc.autoindex)
        {
            return errorResponse(
                403,
                "",
                config
            );
        }


        /*
         * ====================================================
         * AUTOINDEX
         * ====================================================
         */

        std::vector<std::string> files =
            readDirectory(fullPath);

        return generateAutoIndex(
            cleanPath,
            fullPath,
            files
        );
    }


    /*
     * Unsupported filesystem object.
     */
    return errorResponse(
        404,
        "",
        config
    );
}