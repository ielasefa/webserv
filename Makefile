NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC =	main.cpp \
		src/routing.cpp \
		src/file_serving.cpp \
		src/autoindex.cpp \
		src/utils.cpp \
		src/error.cpp \
		src/handlePost.cpp \
		src/handleRequest.cpp \
		src/response.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)
	rm -f test_config_parser test_integration

re: fclean all

test: test_config_parser test_integration

test_config_parser: test_config_parser.cpp ConfigParser.cpp ServerConfig.cpp LocationConfig.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_integration: test_integration.cpp ConfigParser.cpp ServerConfig.cpp LocationConfig.cpp CGIHandler.cpp webserv.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

test_clean:
	rm -f test_config_parser test_integration

.PHONY: all clean fclean re test test_config_parser test_integration test_clean