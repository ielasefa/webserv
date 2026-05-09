SRC = sock.cpp
OBJ = $(SRC:.cpp=.o)

COMPILE = c++
FLAGS = -Wall -Werror -Wextra -std=c++98

NAME = webserv

all: $(NAME)

$(NAME): $(OBJ)
	$(COMPILE) $(FLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(COMPILE) $(FLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all