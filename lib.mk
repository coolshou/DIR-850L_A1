# function: create_install_paths
# $1: paths
# usage: $(call create_install_paths,path_a path_b ...)
define create_install_paths
	@for path in $1; do \
		[ -d $$path ] || mkdir -p $$path; \
	done
endef

# function: install_scripts
# $1: source path
# $2: destination path
# $3: filenames
# usage: $(call install_scripts,path_a,path_b,script_1 script_2 ...)
define install_scripts
	@for file in $3; do \
		cp --remove-destination $1/$$file $2; \
		chmod +x $2/$$file; \
	done
endef

# function: install_files
# $1: source path
# $2: destination path
# $3: filenames
# usage: $(call install_files,path_a,path_b,file_1 file_2 ...)
define install_files
	@for file in $3; do \
		cp --remove-destination $1/$$file $2; \
	done
endef

# function: install_apps
# $1: source path
# $2: destination path
# $3: filenames
# usage: $(call install_apps,path_a,path_b,app_1 app_2 ...)
define install_apps
	@for file in $3; do \
		$(STRIP) $1/$$file; \
		cp --remove-destination $1/$$file $2; \
	done
endef

# function: install_dirs
# $1: source path
# $2: destination path
# $3: directory names
# usage: $(call install_dirs,path_a,path_b,dir_1 dir_2 ...)
define install_dirs
	@for dir in $3; do \
		[ -d $2/$$dir ] || mkdir -p $2/$$dir; \
		cp -r --remove-destination $1/$$dir $2/.; \
	done
endef

# function: color_print
# $1: message
# $2: color 
# usage: $(call color_print,message,color)
define color_print
	$(if $(filter $2,green GREEN),@echo -e "\033[32m$1\033[0m",\
	$(if $(filter $2,red RED),@echo -e "\033[31m$1\033[0m",\
	@echo -e "$1"))
endef

# fucntion: do_make_for_each
# $1: extra arguments (-DDEBUG ...)
# $2: make target (ex: all, clean, install ...)
# $3: directories
# usage: $(call do_make_for_each,arguments,targe,dir_1 dir_2 ...)
define do_make_for_each
	@for dir in $3; do \
		make $1 -C $$dir $2; \
	done
endef

