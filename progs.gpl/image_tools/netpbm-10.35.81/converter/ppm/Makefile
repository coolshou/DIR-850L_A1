ifeq ($(SRCDIR)x,x)
  SRCDIR = $(CURDIR)/../..
  BUILDDIR = $(SRCDIR)
endif
SUBDIR = converter/ppm
VPATH=.:$(SRCDIR)/$(SUBDIR)

include $(BUILDDIR)/Makefile.config

SUBDIRS = hpcdtoppm ppmtompeg

PORTBINARIES =	411toppm eyuvtoppm gouldtoppm ilbmtoppm imgtoppm \
		leaftoppm mtvtoppm neotoppm \
		pcxtoppm pc1toppm pi1toppm picttoppm pjtoppm \
		ppmtoacad ppmtoarbtxt \
		ppmtobmp ppmtoeyuv ppmtogif ppmtoicr ppmtoilbm \
		ppmtoleaf ppmtolj ppmtomitsu ppmtoneo \
		ppmtopcx ppmtopi1 ppmtopict ppmtopj \
		ppmtopjxl ppmtoppm ppmtopuzz ppmtorgb3 ppmtosixel ppmtoterm \
		ppmtowinicon ppmtoxpm ppmtoyuv ppmtoyuvsplit \
		qrttoppm rawtoppm rgb3toppm spctoppm \
		sputoppm tgatoppm winicontoppm ximtoppm xpmtoppm xvminitoppm \
		yuvtoppm yuvsplittoppm

MATHBINARIES = sldtoppm 

# We don't build vidtoppm by default, because it requires special libraries
# and there is no known requirement for vidtoppm.


MERGEBINARIES = $(PORTBINARIES) $(MATHBINARIES)
NOMERGEBINARIES =

BINARIES = $(MERGEBINARIES) $(NOMERGEBINARIES) 

DATAFILES = pcxstd.ppm

OBJECTS = $(BINARIES:%=%.o)

MERGE_OBJECTS = $(BINARIES:%=%.o2)

.PHONY: all
all: $(BINARIES) $(SUBDIRS:%=%/all)

include $(SRCDIR)/Makefile.common

ppmtobmp.o ppmtobmp.o2: bmp.h

tgatoppm.o tgatoppm.o2: tga.h
