## Process this file with automake to produce Makefile.in


lib_LTLIBRARIES =  \
	libdialogs.la

libdialogs_la_SOURCES =  \
	databasetype.cpp \
	databasetype.h \
	dialogs.cpp \
	odbcconfigure.cpp \
	odbcconfigure.h \
	selecttables.cpp \
	selecttables.h \
	createindex.cpp \
	createindex.h \
	properties.cpp \
	properties.h \
	foreignkey.cpp \
	foreignkey.h \
	getobjectname.cpp \
	getobjectname.h \
	jointype.cpp \
	jointype.h \
	addcolumnsdialog.cpp \
	addcolumnsdialog.h \
	odbccredentials.cpp \
	odbccredentials.h \
	bitmappanel.cpp \
	bitmappanel.h \
	newquery.cpp \
	newquery.h \
	quickselect.cpp \
	quickselect.h \
	retrievalarguments.cpp \
	retrievalarguments.h \
	typecombobox.cpp \
	typecombobox.h \
	gotoline.cpp \
	gotoline.h \
	attachdb.cpp \
	attachdb.h \
	detachdb.cpp \
	detachdb.h \
	datasource.cpp \
	datasource.h

libdialogs_la_CXXFLAGS = -I../../dbinterface \
	-I../../libfieldswindow \
	-I../../libshapeframework \
	-I../../libpropertypages \
	-I../../libpropertieshandlers \
	`pkg-config \
	--cflags \
	gtk+-3.0` \
	`wx-config \
	--cxxflags` 

libdialogs_la_LDFLAGS = -L../libsqlite \
	-L../libshapeframework \
	-L../dbinterface \
	-L../libfieldswindow \
	-L../libodbc \
	-L../libpostgres \
	-L../libpropertypages \
	-ldbinterface \
	-lsqlite_lib \
	-lodbc_lib \
	-lshapeframework \
	-lfieldswindow \
	-lpropertypages \
	-lpostgres \
	`wx-config \
	--libs`
	
	 
