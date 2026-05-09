
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

test: test_config_parser test_integration
	@echo "Running all tests..."

test_config_parser: test_config_parser.cpp ConfigParser.cpp ServerConfig.cpp LocationConfig.cpp
	$(CXX) $(CPPFLAGS) -o test_config_parser test_config_parser.cpp ConfigParser.cpp ServerConfig.cpp LocationConfig.cpp
	@echo "Running ConfigParser tests..."
	@./test_config_parser

test_integration: test_integration.cpp ConfigParser.cpp ServerConfig.cpp LocationConfig.cpp CGIHandler.cpp webserv.cpp
	$(CXX) $(CPPFLAGS) -o test_integration test_integration.cpp ConfigParser.cpp ServerConfig.cpp LocationConfig.cpp CGIHandler.cpp webserv.cpp
	@echo "Running Integration tests..."
	@./test_integration

test_clean:
	rm -f test_config_parser test_integration

.PHONY: all clean fclean re test test_config_parser test_integration test_clean
