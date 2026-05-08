NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iinclude

SRC = main.cpp src/routing.cpp src/file_serving.cpp src/autoindex.cpp src/utils.cpp src/error.cpp \
		src/handlePost.cpp src/handleRequest.cpp src/response.cpp

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

re: fclean all

.PHONY: all clean fclean re
