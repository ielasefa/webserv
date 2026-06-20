
NAME    = webserv
CXX     = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS    = webserv.cpp \
          LocationConfig.cpp \
          ServerConfig.cpp \
          ConfigParser.cpp \
          CGIHandler.cpp

OBJS    = $(addprefix obj/, $(SRCS:.cpp=.o))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@echo "✓ $(NAME) built successfully"

obj/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj/

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
