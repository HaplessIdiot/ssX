all: randr.dvi randr.ps randr.pdf randr/index.html

randr.bbl: randr.tex randr.bib
	latex randr
	bibtex randr

randr.dvi: randr.tex randr.bbl multi.eps shadow.eps
	latex randr
	latex randr

randr.ps: randr.dvi
	dvips -o randr.ps randr

randr.pdf: randr.ps
	ps2pdf randr.ps randr.pdf

randr:
	mkdir randr

randr/index.html: randr.tex randr.bbl
	sh makehtml

clean:
	rm -f randr.aux randr.bbl randr.blg randr.log randr.pdf randr.ps randr.dvi
