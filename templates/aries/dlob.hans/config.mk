# cutomized config for D-Link DIR-XXX
ifeq (buildno,$(wildcard buildno))
BUILDNO:=$(shell cat buildno)
PREDEFINE_RELIMAGE:=$(shell echo $(ELBOX_MODEL_NAME)_v$(ELBOX_FIRMWARE_VERSION)_$(BUILDNO))
endif
LANGPACK_ZIP:=sealpac.tgz

seal_langpack:
	@echo -e "\033[32m"; \
	echo "==========================================================================================="; \
	echo "You are going to seal the f/w with language pack.";                                           \
	echo "Please follow the steps:";                                                                    \
	echo " 1. input the existent Language Pack file.                 (eg: images/dir###_v1.00_8co8.tc.slp)";\
	echo " 2. input the country code.                                (eg: ru)";    							\
	echo " 3. input the existent F/W file.                           (eg: images/dir###_v1.01_916d.bin)";   \
	echo " 4. input the prefix file name of the output image.        (eg: dir###_v1.01_916d.ru)";           \
	echo " 5. input the compress format(gZIP|bZIP2), default is gzip.(eg: b)";					    \
	echo "    (Then you will get the sealed f/w \"images/dir###_v1.01_916d.tc.bin\".)";               \
	echo "==========================================================================================="; \
	echo -e -n "1. Please input the language pack : \033[0m";   \
	read lang;  \
	if [ ! -f $$lang ]; then \
		echo -e "\033[32mThe slp file [$$lang] is not existence!\033[0m"; exit 9;\
	else    \
		if [ "$$lang" == "" ];  then echo -e "\033[32mThe language pack is empty!\033[0m";          exit 9; fi; \
		echo -e -n "\033[32m2. Please input the country code in ISO name : \033[0m"; read lancode;	\
		if [ "$$lancode" == "" ];    then echo -e "\033[32mThe country code is empty!\033[0m";		exit 9; fi; \
		echo -e -n "\033[32m3. Please input the f/w : \033[0m"; read fw;    \
		if [ ! -f $$fw ];       then echo -e "\033[32mThe f/w [$$fw] is not existence!\033[0m";   exit 9; fi; \
		if [ "$$fw" == "" ];    then echo -e "\033[32mThe f/w is empty!\033[0m";                    exit 9; fi; \
		echo -e -n "\033[32m4. The prefix of the output f/w name you want: \033[0m";    read outfw; \
		if [ "$$outfw" == "" ]; then echo -e "\033[32mThe output f/w name is empty!\033[0m";        exit 9; fi; \
		echo -e -n "\033[32m5. Select the compress format(g:gzip|b:bzip2|l:lzma): \033[0m";    read zipmode; \
		make V=$(V) FW=$$fw LANGPACK_FILE=$$lang OUTPUT_FW=$$outfw LANCODE=$$lancode ZIPMODE=$$zipmode seal;    \
	fi

seal:
	@echo -e "\033[32m$(MYNAME) prepare seama file of language pack [$(LANGPACK_FILE)]...\033[0m"
	$(Q)[ -d sealpac ] || mkdir -p sealpac
	$(Q)cp $(LANGPACK_FILE) sealpac/sealpac.slp
	if [ "$(ZIPMODE)" == "l" ]; then				\
		tar -cf $(LANGPACK_ZIP) sealpac --lzma;	\
	elif [ "$(ZIPMODE)" == "b" ]; then				\
		tar -jcf $(LANGPACK_ZIP) sealpac;		\
	else											\
		tar -zcf $(LANGPACK_ZIP) sealpac;		\
	fi
	$(Q)$(SEAMA) -i $(LANGPACK_ZIP) -m type=sealpac -m dev=$(LANGDEV) -m langcode=$(LANCODE) -m file=$(LANGPACK_FILE)
	@echo -e "\033[32m$(MYNAME) prepare seama file of f/w image [$(FW)]...\033[0m"
	$(Q)$(SEAMA) -x raw.img -i $(FW) -m type=firmware
	$(Q)$(SEAMA) -i raw.img -m dev=$(FWDEV) -m type=firmware
	@echo -e "\033[32m$(MYNAME) seal f/w and language pack...\033[0m"
	$(Q)$(SEAMA) -s web.img -i raw.img.seama -i $(LANGPACK_ZIP).seama -m signature=$(ELBOX_SIGNATURE)
	$(Q)$(SEAMA) -d web.img
	$(Q)rm -rf sealpac; rm $(LANGPACK_ZIP) $(LANGPACK_ZIP).seama raw.img raw.img.seama
	$(Q)./tools/release.sh web.img $(OUTPUT_FW).bin

extract: ./tools/seama/seama
	@echo -e -n "\033[32mPlease input META you want to extract (ex: type=firmware / type=sealpac) : \033[0m";   \
	read meta;  \
	if [ "$$meta" == "" ]; then echo -e "\033[32mThe META is empty!\033[0m";exit 9; fi; \
	echo -e -n "\033[32mPlease input the f/w image : \033[0m";  \
	read fw;    \
	if [ ! -f $$fw ]; then \
		echo -e "\033[32mThe image file [$$fw] is not existence!\033[0m"; \
		echo -e "STOP extracting f/w!\033[0m"; \
	else    \
		if [ "$$fw" == "" ];    then echo -e "\033[32mThe image file  is empty!\033[0m"; exit 9; fi;    \
		echo -e -n "\033[32mThe output file name you want : \033[0m";   read outfile;   \
		if [ "$$outfile" == "" ];   then echo -e "\033[32mThe output file is empty!\033[0m"; exit 9; fi;    \
		$(SEAMA) -x $$outfile -i $$fw -m $$meta;    \
		if [ "$$meta" == "type=sealpac" ]; then \
			tar xvf $$outfile; mv sealpac/sealpac.slp $$outfile; rm -rf sealpac;    \
		fi; \
		ls -al $$outfile;   \
	fi


.PHONY: seal_langpack seal extract
