main: *.tex *.bib
	pdflatex main
	bibtex main
	makeindex -s nomencl.ist -o main.nls main.nlo
	pdflatex main
	pdflatex main
