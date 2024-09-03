THERE_DATA := $(shell date "+%Y-%m-%d")
TARGETNAME := IMachine_HB
TARGETTYPE := APP

VERSION_STRING = "V1.01"
DATE_STRING := `date "+20%y-%m-%d %k:%M"`
$(shell echo "#define VER_AUTO 1" > include/Auto_version.h)
$(shell echo "#define VERSION \"$(VERSION_STRING)\"" >> include/Auto_version.h)
$(shell echo "#define COMPILEDATE \"$(DATE_STRING)\"" >> include/Auto_version.h)

to_lowercase = $(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

CONFIG ?= DEBUG

#$(shell bash cs_ctags.sh >> /dev/null)

CONFIGURATION_FLAGS_FILE := $(call to_lowercase,$(CONFIG)).mak

include $(CONFIGURATION_FLAGS_FILE)
CONFIGURATION_LINKER_SCRIPT := $(LINKER_SCRIPT)

include $(ADDITIONAL_MAKE_FILES)

ifneq ($(CONFIGURATION_LINKER_SCRIPT),)
LINKER_SCRIPT := $(CONFIGURATION_LINKER_SCRIPT)
endif

ifneq ($(LINKER_SCRIPT),)
LDFLAGS += -T$(LINKER_SCRIPT)
endif

ifeq ($(BINARYDIR),)
error:
	$(error Invalid configuration, please check your inputs)
endif

DIR_SRC = src
SOURCEFILES := $(wildcard ${DIR_SRC}/*.c) $(wildcard ${DIR_SRC}/*.cpp)
EXTERNAL_LIBS :=
EXTERNAL_LIBS_COPIED := $(foreach lib, $(EXTERNAL_LIBS),$(BINARYDIR)/$(notdir $(lib)))

CFLAGS += $(COMMONFLAGS)
CXXFLAGS += $(COMMONFLAGS)
ASFLAGS += $(COMMONFLAGS)
LDFLAGS += $(COMMONFLAGS)

CFLAGS += $(addprefix -I,$(INCLUDE_DIRS))
CXXFLAGS += $(addprefix -I,$(INCLUDE_DIRS))

CFLAGS += $(addprefix -D,$(PREPROCESSOR_MACROS))
CXXFLAGS += $(addprefix -D,$(PREPROCESSOR_MACROS))
ASFLAGS += $(addprefix -D,$(PREPROCESSOR_MACROS))

CXXFLAGS += $(addprefix -framework ,$(MACOS_FRAMEWORKS))
CFLAGS += $(addprefix -framework ,$(MACOS_FRAMEWORKS))
LDFLAGS += $(addprefix -framework ,$(MACOS_FRAMEWORKS))

LDFLAGS += $(addprefix -L,$(LIBRARY_DIRS))

LIBRARY_LDFLAGS = $(addprefix -l,$(LIBRARY_NAMES))
LIBRARY_SHARE :=

ifeq ($(IS_LINUX_PROJECT),1)
	RPATH_PREFIX := -Wl,--rpath='$$ORIGIN/../
	LIBRARY_LDFLAGS += $(EXTERNAL_LIBS)
	LIBRARY_LDFLAGS += -Wl,--rpath='$$ORIGIN/ArtcLib'
	LIBRARY_LDFLAGS += $(addsuffix ',$(addprefix $(RPATH_PREFIX),$(dir $(EXTERNAL_LIBS))))

	ifeq ($(TARGETTYPE),SHARED)
		LIBRARY_LDFLAGS += -Wl,-soname,$(TARGETNAME)
		LIBRARY_SHARE += -fPIC
		TARGETNAME := lib$(TARGETNAME).so
	endif

else
	LIBRARY_LDFLAGS += $(EXTERNAL_LIBS)
endif

LIBRARY_LDFLAGS += $(ADDITIONAL_LINKER_INPUTS)

all_make_files := Makefile $(CONFIGURATION_FLAGS_FILE) $(ADDITIONAL_MAKE_FILES)

ifeq ($(STARTUPFILES),)
	all_source_files := $(SOURCEFILES)
else
	all_source_files := $(STARTUPFILES) $(filter-out $(STARTUPFILES),$(SOURCEFILES))
endif

source_obj1 := $(all_source_files:.cpp=.o)
source_obj2 := $(source_obj1:.c=.o)
source_obj3 := $(source_obj2:.s=.o)
source_objs := $(source_obj3:.S=.o)

all_objs := $(addprefix $(BINARYDIR)/, $(notdir $(source_objs)))

ifeq ($(GENERATE_BIN_FILE),1)
all: $(BINARYDIR)/$(basename $(TARGETNAME)).bin

$(BINARYDIR)/$(basename $(TARGETNAME)).bin: $(BINARYDIR)/$(TARGETNAME)
	$(OBJCOPY) -O binary $< $@

else
all: $(BINARYDIR)/$(TARGETNAME)
	$(shell cp $(BINARYDIR)/$(TARGETNAME) $(BINARYDIR)/$(TARGETNAME)_$(THERE_DATA))
	$(shell cp $(BINARYDIR)/$(TARGETNAME) /data/$(TARGETNAME))
endif

ifeq ($(TARGETTYPE),APP)
$(BINARYDIR)/$(TARGETNAME): $(all_objs) $(EXTERNAL_LIBS)
	$(LD) -o $@ $(LDFLAGS) $(START_GROUP) $(all_objs) $(LIBRARY_LDFLAGS) $(END_GROUP)
endif

ifeq ($(TARGETTYPE),SHARED)
$(BINARYDIR)/$(TARGETNAME): $(all_objs) $(EXTERNAL_LIBS)
	$(LD) -shared -o $@ $(LDFLAGS) $(START_GROUP) $(all_objs) $(LIBRARY_LDFLAGS) $(END_GROUP)
endif

ifeq ($(TARGETTYPE),STATIC)
$(BINARYDIR)/$(TARGETNAME): $(all_objs)
	$(AR) -r $@ $^
endif

-include $(all_objs:.o=.dep)

clean:
ifeq ($(USE_DEL_TO_CLEAN),1)
	del /S /Q $(BINARYDIR)
else
	rm -rf $(BINARYDIR) cscope* tags ArtcLog
	$(shell rm -rf /data/$(TARGETNAME)*)
endif

$(BINARYDIR):
	mkdir $(BINARYDIR)

$(BINARYDIR)/%.o : src/%.cpp $(all_make_files) |$(BINARYDIR)
	$(CXX) $(CXXFLAGS) $(LIBRARY_SHARE) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : src/%.c $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(LIBRARY_SHARE) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.S $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) $(LIBRARY_SHARE) -c $< -o $@ -MD -MF $(@:.o=.dep)

$(BINARYDIR)/%.o : %.s $(all_make_files) |$(BINARYDIR)
	$(CC) $(CFLAGS) $(ASFLAGS) $(LIBRARY_SHARE) -c $< -o $@ -MD -MF $(@:.o=.dep)
