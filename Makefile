NAME = ircserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g

# Source files
SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/Server.cpp \
       $(SRC_DIR)/Client.cpp \
       $(SRC_DIR)/Channel.cpp \
	   $(SRC_DIR)/Utils.cpp \
	   $(SRC_DIR)/debug.cpp

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Ejecutar el servidor con parámetros de prueba
run: $(NAME)
	@./$(NAME) 6667 123456

# Prevent conflicts with files named like our targets
.PHONY: all clean fclean re run
