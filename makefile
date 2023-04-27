# definizione del compilatore e dei flag di compilazione
# che vengono usate dalle regole implicite
CC=gcc
CFLAGS=-std=c11 -Wall -O -g
LDLIBS=-lm -lrt -pthread

# elenco degli eseguibili da creare
EXECS=archivio

.PHONY: clean

# primo target: gli eseguibili sono precondizioni del target
# quindi verranno tutti creati
all: $(EXECS) 

archivio: archivio.o xerrori.o funThreads.o

# regola per pulire i file oggetto e gli eseguibili
clean:
	rm -f $(EXECS) *.o valgrind-*.log server.log lettori.log

# target che crea l'archivio dei sorgenti
zip:
	zip $(EXECS).zip makefile *.c *.h *.py *.md

execute:
	git clone git@github.com:ImNico00/Progetto-Lab-2.git testdir
	cd testdir
	make
	./server.py 5 -r 2 -w 4 -v &
	./client2 file1 file2
	./client1 file3
	pkill -INT -f server.py
