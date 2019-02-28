all:
	gcc -g -Wall msgq_client.c -o client
	gcc -g -Wall msgq_server.c -o server
	gcc -g -Wall msgq_group.c -o group

pdf: doc.tex
	pdflatex doc.tex

clean: 
	rm server client group