## Process this file with automake to produce Makefile.in


lib_LTLIBRARIES =  \
	libpropertypages.la

libpropertypages_la_SOURCES =  \
	tablegeneral.cpp \
	tablegeneral.h \
	fontpropertypagebase.cpp \
	fontpropertypagebase.h \
	syntaxproppage.cpp \
	syntaxproppage.h \
	wherehavingpage.cpp \
	wherehavingpage.h \
	fieldgeneral.cpp \
	fieldgeneral.h \
	fieldheader.cpp \
	fieldheader.h \
	propertypagebase.h \
	designlabelgeneral.cpp \
	designlabelgeneral.h \
	sortgroupbypage.cpp \
	sortgroupbypage.h \
	bandgeneral.cpp \
	bandgeneral.h \
	pointerproperty.cpp \
	pointerproperty.h \
	tableprimarykey.cpp \
	tableprimarykey.h \
	designgeneral.cpp \
	designgeneral.h \
	colorcombobox.cpp \
	colorcombobox.h \
	printspec.cpp \
	printspec.h \
	qt/fontpropertypageQt.cpp \
	qt/fontpropertypageQt.h \
	gtk/fontpropertypageGTK.cpp \
	gtk/fontpropertypageGTK.h

libpropertypages_la_CXXFLAGS = -I../../dbinterface \
	-I../../libshapeframework \
	-I../../libfieldswindow \
	`pkg-config \
	--cflags \
	gtk+-3.0` \
	`wx-config \
	--cxxflags` 

libpropertypages_la_LDFLAGS = `pkg-config \
	--libs \
	gtk+-3.0` \
	`wx-config \
	--libs` 
