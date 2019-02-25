all: shell.c
	gcc -g shell.c -o shell
	#pdflatex doc.tex

pdf: doc.tex
	pdflatex doc.tex

clean: 
	rm shell
	rm *.pdf *.log *.aux