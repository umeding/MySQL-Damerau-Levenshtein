# Makefile
# Copyright (c) 2008 Meding Software Technik -- All Rights Reserved.
#

CC=gcc

SRCS=dameraulevenshtein.cpp
OBJS=${SRCS:.cpp=.o}
SO=libdameraulevenshtein.so
CPPFLAGS=-I/usr/include/mysql -O -fPIC
LDOPTS=-shared -L/usr/lib/mysql -lmysqlclient -lstdc++

${SO}:	${OBJS}
	${CC} -o ${SO} ${OBJS} ${LDOPTS}

clean:;	rm -f ${SO} ${OBJS} *~
