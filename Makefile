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
      src/init/server.cpp \
      src/init/request_utils.cpp \
      src/init/epoll_utils.cpp

OBJ_DIR = obj

OBJ = $(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re