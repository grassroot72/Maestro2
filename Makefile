# $@ - target
# $^ - dependency
# $< - first prerequisite (usually the source file)

CC = gcc

#SANITIZER = -fsanitize=address
OPTIMISATION = -O3

CFLAGS = -Idsa -Icrypt -Itools -Iservices -I. \
         -I/usr/lib/mimalloc-2.0/include \
         -Wall -march=native \
         $(OPTIMISATION) \
         $(SANITIZER)

LDFLAGS = -lpq \
          -lpthread \
          -ldeflate \
          -lmimalloc \
          $(SANITIZER)

OBJS = dsa/sllist.o \
       dsa/dllist.o \
       dsa/rbtree.o \
       crypt/base64.o \
       crypt/md5.o \
       crypt/sha1.o \
       crypt/sha256.o \
       crypt/hmac_sha256.o \
       tools/util.o \
       tools/thpool.o \
       tools/io.o \
       epsock.o \
       pg_conn.o \
       http_header.o \
       http_msg.o \
       http_parser.o \
       http_cache.o \
       http_get.o \
       http_post.o \
       http_conn.o \
       http_cfg.o \
       services/jwt.o \
       services/auth.o \
       services/sqlobj.o \
       services/sqlops.o \
       maestro.o

EXES = maestro

all: ${EXES}

${EXES}: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	${CC} -o $@ -c $< $(CFLAGS)

clean:
	$(RM) *.o dsa/*.o crypt/*.o tools/*.o services/*.o $(EXES)
