$ VERIFY = F$Verify (0)
$ On Error Then GoTo EXIT
$ Write Sys$Output "SETting UP PBMplus (ver netpbm-VMS)..."
$! 
$!  Keep this proc in the top directory of the PBMPLUS tree. Execute it from 
$!  anywhere and it will set up command symbols for all executables in the
$!  PBMplus_Root:[EXE] directory.
$!  There is a problem if this directory is located in a "rooted"
$!  directory structure already.  It is not possible to define a "rooted"
$!  directory twice, i.e.:
$![1;5m BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD[m
$![1;5m BAD                                                                 BAD[m
$![1;5m BAD[m     Define /Trans=conceal Public Disk$:[Dir.]                   [1;5mBAD[m
$![1;5m BAD[m     Define /Trans=conceal PBMplus_Root Public:[PBMplus.]        [1;5mBAD[m
$![1;5m BAD                                                                 BAD[m
$![1;5m BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD BAD[m
$!  THIS WILL NOT WORK!  In this case, you will have to manually define
$!  PBMplus_Root instead of the autosensing feature below....
$! 
$ PBMPLUS_PATH = F$Element (0, "]", F$Environment ("PROCEDURE")) + ".]"
$ Define /Translation_Attributes = Concealed PBMplus_Root "''PBMPLUS_PATH'"
$ Define PBMplus_Dir PBMplus_Root:[000000]
$ Define PBMplusShr PBMplus_Dir:PBMplusShr
$ NAME = "PBMplus_Root:[Exe]*.EXE"
$LOOP:
$   PROG = F$Search (NAME)
$   If PROG .nes. ""
$       Then
$           PROG = PROG - F$Parse (PROG, , , "VERSION")
$           CMD = F$Parse (PROG, , , "NAME")
$           'CMD' :== $ 'PROG'
$       GoTo LOOP
$   EndIf
$   @ PBMplus_Dir:ADD_LIST Hlp$Library PBMplus_Dir:PBMPLUS.HLB
$EXIT:
$   VERIFY = F$Verify (VERIFY)
$   Exit
