Note:
	The patch file just for issustrate which codes i modified, WOULD NOT apply at building time,
	if you changed something, you should udpate this file (make mk_patch),
	and do patch action by yourself (not always),
	then commit both(patch file and patched source code) to version control server.

	ex:
		list all files:
		$ ls -al
		-rw-rw-r--  1 user user  2116 Aug 28 13:54 Makefile
		drwxrwxr-x  9 user user  4096 Aug 27 19:56 ecmh-2005.02.09
		-rw-rw-r--  1 user user 43627 Aug 27 19:56 ecmh_2005.02.09.tar.bz2
		drwxrwxr-x  3 user user  4096 Aug 28 14:11 patch

		chang some code, directy modify ecmh-2005.02.09/[you want modifiy files]
		$ vim ./ecmh-2005.02.09/src/[some file].c

		after you make sure your modified code is ok, what you need to do ?:
		1. make new patch file
			$ make mk_patch
			$ ls -al
			-rw-rw-r--  1 user user  2116 Aug 28 13:54 Makefile
			drwxrwxr-x  9 user user  4096 Aug 27 19:56 ecmh-2005.02.09
			-rw-rw-r--  1 user user 43627 Aug 27 19:56 ecmh_2005.02.09.tar.bz2
			-rw-rw-r--  1 user user 22049 Aug 28 14:17 new.patch               <--------- new patch file
			drwxrwxr-x  3 user user  4096 Aug 28 14:11 patch
			$ cp ./new.patch ./patch/001.alpha.patch
			$ rm ./new.patch
			
		2. change 'ALPHA_VERSION' as your modify date at Makefile 
		3. clean object files, binary files
			$ make clean
			
		4. DONE, just commit it to version control server
			
