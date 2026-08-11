NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude -g3

SRC = webserv.cpp \
      src/method/routing.cpp \
      src/method/file_serving.cpp \
      src/method/autoindex.cpp \
      src/method/utils.cpp \
      src/method/error.cpp \
      src/method/handlePost.cpp \
      src/method/handleRequest.cpp \
      src/method/response.cpp \
      src/parsing/ConfigParser.cpp \
      src/parsing/ServerConfig.cpp \
      src/parsing/LocationConfig.cpp \
      src/parsing/CGIHandler.cpp \
      src/init/multiplexing.cpp \
      src/init/server.cpp

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

.PHONY: all clean fclean re