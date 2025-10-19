NAME = webserv

SRC_DIR = src
INC_DIR = include


SRCS = main.cpp utils.cpp utilsCC.cpp \
		LocationConfig.cpp \
		ServerConfig.cpp \
		HttpRequest.cpp \
		HttpResponse.cpp \
		initServer.cpp \
		ConfigParser.cpp \
		Signals.cpp

OBJS = $(SRCS:.cpp=.o)
INCLUDES = -I $(INC_DIR)

CC = c++
RM = rm -f
CPPFLAGS = -D_GLIBCXX_USE_CXX11_ABI=0 -std=c++98
CFLAGS = -Wall -Wextra -Werror  -g #-fsanitize=address -fsanitize=leak


all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJS) $(INCLUDES) -o $@

%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/*.hpp Makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< $(INCLUDES) -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
