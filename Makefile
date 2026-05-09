
NAME    = webserv
CPP     = c++
CPPFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS    = main.cpp \
          webserv.cpp \
          config/LocationConfig.cpp \
          config/ServerConfig.cpp \
          config/ConfigParser.cpp \
          cgi/CGIHandler.cpp

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
