# build an executable named chat from chat.c
all: chat.c 
		gcc api/api.c chat.c -o chat -lpthread

clean: 
	rm chat