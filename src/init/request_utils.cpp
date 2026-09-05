#include "../../webserv.hpp"

HttpRequest	parsing_header(std::string req_buffer)
{
	HttpRequest req;

	std::string line = req_buffer.substr(0, req_buffer.find("\r"));
	req.method = line.substr(0, line.find(" "));
	std::string other = line.substr(line.find(" ") + 1);
	if (other.find("?") != std::string::npos)
	{
		req.path = other.substr(0, other.find("?"));
		req.query_string = other.substr(other.find("?") + 1, other.find(" ") - other.find("?") - 1);
	}
	else
		req.path = other.substr(0, other.find(" "));

	if (other.find(" ") != std::string::npos)
		req.version = other.substr(other.find(" ") + 1);
	else
		req.version = "";

	std::string tmp = req_buffer.substr(req_buffer.find("\r\n") + 2);
	while (tmp.find("\r\n") != std::string::npos)
	{
		line = tmp.substr(0, tmp.find("\r\n"));
		if (line.empty())
			break;
		std::string key, value;
		size_t pos = line.find(": ");
		if (pos != std::string::npos)
		{
			key = line.substr(0, pos);
			value = line.substr(pos + 2);
		}
		req.headers[key] = value;
		tmp = tmp.substr(tmp.find("\r\n") + 2);
	}

	if (req.headers.find("Host") != req.headers.end())
	{
		tmp = req.headers["Host"];
		if (tmp.find(":") != std::string::npos)
			req.host = req.host.substr(0, req.host.find(":"));
		else
			req.host = tmp;
	}

	return (req);
}


bool is_bodyComplete(t_client &c)
{
	size_t i = c.req_buffer.find("\r\n\r\n");
	if (i == std::string::npos)
		return false;
	size_t current = i + 4;

	if (!c.is_chunked)
	{
		if (c.content_len == 0)
			return true;
		return c.req_buffer.size() - current >= c.content_len;
	}

	while (true)
	{
		size_t end = c.req_buffer.find("\r\n", current);
		if (end == std::string::npos)
			return false;

		std::string chunk_size = c.req_buffer.substr(current, end - current);

		size_t ext = chunk_size.find(';');
		if (ext != std::string::npos)
			chunk_size = chunk_size.substr(0, ext);

		char *p;
		unsigned long len = std::strtoul(chunk_size.c_str(), &p, 16);
		if (*p != '\0')
			break ;
		current = end + 2;

		if (len == 0)
		{
			if (c.req_buffer.compare(current, 2, "\r\n") == 0)
				return true;
		
			if (c.req_buffer.find("\r\n\r\n", current) != std::string::npos)
				return true;
		
			return false;
		}

		if (c.req_buffer.size() < current + len + 2)
			break ;
		if (c.req_buffer.compare(current + len, 2, "\r\n") != 0)
			break ;

		current += len + 2;
	}
	return false;
}

std::string combining_chunks(const std::string& buffer, size_t body_pos)
{
	std::string body;
	size_t current = body_pos;

	while (1)
	{
		size_t end = buffer.find("\r\n", current);
		std::string chunk_size = buffer.substr(current, end - current);

		size_t ext = chunk_size.find(';');
		if (ext != std::string::npos)
			chunk_size = chunk_size.substr(0, ext);

		char *p;
		unsigned long len = std::strtoul(chunk_size.c_str(), &p, 16);
		current = end + 2;

		if (len == 0)
			break;

		body += buffer.substr(current, len);
		current += len + 2;
	}

	return body;
}
