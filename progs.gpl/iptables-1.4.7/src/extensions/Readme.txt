If you want to add new TARGET in extension, there are 2 ways.

1. By checking kernel .config file

   in GNUMakefile.in
	ifeq ($(CONFIG_IP_NF_TARGET_HIJACK), y)

		pf4_build_mod += HIJACK
	
	endif


2. By checking kernel support

   add .TARGET-test 
   Pls check hijack.sampletest file


