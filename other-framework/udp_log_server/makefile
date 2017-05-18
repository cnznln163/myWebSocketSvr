
all:udp_log_server

OBJECTS=udp_log_server.o ../timer_set.o


%.o:%.c
	gcc -g -Wall -Wextra -O2 -c $< -o $@

udp_log_server:$(OBJECTS)
	gcc -g -Wall -Wextra -O2 -o udp_log_server $(OBJECTS)

clean:
	rm -f udp_log_server $(OBJECTS)

