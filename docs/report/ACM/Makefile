texengine=pdflatex
main.pdf : *.tex *.bib
	${texengine}  --shell-escape main

all: *.tex
	${texengine}  --shell-escape main
	bibtex main
	${texengine} --shell-escape main
	${texengine} --shell-escape main

full: clean all

clean:
	${RM} -rf  *.aux *.log *.blg *.ent  tmp/* *.auxlock *.bbl *.out *.md5
	${RM} -rf main.pdf

pull:
	git stash; git pull --rebase; git stash pop; make

push:
	git commit -am "Pushed using make push"; git pull --rebase; git push origin master
