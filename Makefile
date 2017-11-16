all: bsdump_template.c
	gcc -o bsdump bsdump_template.c
	./bsdump image
byte: bytedump.c
	gcc -o bytedump bytedump.c
	./bytedump image 11
