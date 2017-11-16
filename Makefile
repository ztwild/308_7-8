all: bsdump_template.c
	gcc -o bsdump bsdump_template.c
	./bsdump image
byte: bytedump.c
	gcc -o bytedump bytedump.c
	./bytedump image 11
fat: fat12ls.c
	gcc -o fat12ls fat12ls.c
	./fat12ls image
