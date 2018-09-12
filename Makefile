#name:Eric Rogers
#email:ejr140230@utdallas.edu
#course number:CS3376.001
# Compiler Specific stuff

CXX = g++
CXXFLAGS = -Wall -O
CPPFLAGS = -I /home/012/e/ej/ejr140230/include
LDFLAGS = -L /home/012/e/ej/ejr140230/lib 
LDLIBS = -lcdk -lcurses -lboost_system -lboost_thread-mt -pthread

#
# Project Specific stuff
PROJECTNAME = HW6
# used for the backup target
EXECFILE = hw6			# the neme of the executable
OBJS = hw6.o	# a list of all .o files needed

# Implicit Rule
%.o:%.cc
	$(CXX) $(CPPFLAGS) -M -MF $*.P $<
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@


# This is the first target so it is the default
all: $(EXECFILE)


# Link .o files to create an executable
$(EXECFILE): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS)


clean:
	make backup
	rm -f $(OBJS) *.P *~ \#* $(EXECFILE)

#
# Note: tar errors will not be displayed.  Check your backups.
#
backup:
	@mkdir -p ~/backups; chmod 700 ~/backups
	@$(eval CURDIRNAME := $(shell basename `pwd`))
	@$(eval MKBKUPNAME := ~/backups/$(PROJECTNAME)-$(shell date +'%Y.%m.%d-%H:%M:%S').tar.gz)
	@echo
	@echo Writing Backup file to: $(MKBKUPNAME)
	@echo
	@-tar zcf $(MKBKUPNAME) ../$(CURDIRNAME) 2> /dev/null
	@chmod 600 $(MKBKUPNAME)
	@echo
	@echo Done!

tarball:
	make backup
	make clean
	tar cfzv ../tarballs/ejr140230_HW6.tar.gz --exclude=cdk-5.0-20161210 --exclude=cdk.tar.gz ../$(PROJECTNAME)

# Include all the header file dependencies
# created by the preprocessor.  The list
# is initially empty after a make clean.
#
# The first full build will create the .P
# files. After the .P files exist and will
# be included.
-include $(OBJS:%.o=%.P)
