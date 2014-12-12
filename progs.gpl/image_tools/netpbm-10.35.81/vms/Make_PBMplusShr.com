$!
$! Make shareable image out of PBMPLUS libraries.  This command procedure
$! takes no arguments, but must be placed in the top-level PBMPLUS directory.
$!
$! It uses the following input library files:
$!
$!	[.PBM]LIBPBM.OLB,[.PGM]LIBPGM.OLB,[.PPM]LIBPPM.OLB,[.PNM]LIBPNM.OLB
$!
$! This procedure generates the following files if missing or out-of-date:
$!
$!	TRANSVEC.OBJ	Object file containing transfer vector for PBMPLUSSHR.
$!	PBMPLUSSHR.EXE	Shareable image file for PBM libraries.
$!	PBMPLUSSHR.OPT	Linker options file for linking utility program against
$!			the PBMPLUSSHR.EXE shareable image.
$!
$ instruct = 0
$ proc = f$environment("PROCEDURE")
$ proc_cdt = f$cvtime(f$file(proc,"CDT"))
$ if f$search("TRANSVEC.OBJ") .EQS. "" THEN GOTO NEW_TRANSVEC
$ if f$cvtime(f$file("TRANSVEC.OBJ","CDT")) .GTS. PROC_CDT THEN GOTO TRANSVEC_DONE
$ NEW_TRANSVEC:
$ instruct = 1
$ Write SYS$Output "Making new transvec.obj..."
$ Macro /NoList /Object = TRANSVEC.OBJ Sys$Input
; PMBPLUS_TRANSFER_VECTOR
; This routine defines a transfer vector for use in creating shareable image
;
; define macro to make transfer vector entry for a given routine.  Entry mask 
; is obtained from routine we are transfering to.  Jump to word past entry 
; since these are VAX procedures (written in FORTRAN).
;
	.MACRO TRANSFER_ENTRY routine
;
	.TRANSFER routine
	.MASK	  routine
	JMP	  routine + 2
;
	.ENDM TRANSFER_ENTRY
;
	.TITLE PBMPLUS_TRANSFER_VECTOR
	.IDENT /01/
	.PSECT PBMPLUS_XVEC PIC,USR,CON,REL,LCL,SHR,EXE,RD,NOWRT,NOVEC
;
;	Simply go through iap procedures and declare transfer vector
;	entry points for them.  New procedure must be added to the END
;	of this list.
;
TRANSFER_VECTOR:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Library LIBPBM
;Module ARGPROC
	TRANSFER_ENTRY BACKGROUND_PROCESS 
	TRANSFER_ENTRY GETOPT
	TRANSFER_ENTRY GETREDIRECTION
	TRANSFER_ENTRY SET_OUTFILE_BINARY

;Module LIBPBM1
	TRANSFER_ENTRY PBM_INIT
	TRANSFER_ENTRY PM_ALLOCARRAY
	TRANSFER_ENTRY PM_ALLOCROW
	TRANSFER_ENTRY PM_BITSTOMAXVAL
	TRANSFER_ENTRY PM_CLOSE
	TRANSFER_ENTRY PM_ERROR                         
	TRANSFER_ENTRY PM_FREEARRAY                     
	TRANSFER_ENTRY PM_FREEROW
	TRANSFER_ENTRY PM_INIT                          
	TRANSFER_ENTRY PM_KEYMATCH                      
	TRANSFER_ENTRY PM_MAXVALTOBITS                  
	TRANSFER_ENTRY PM_MESSAGE
	TRANSFER_ENTRY PM_OPENR                         
	TRANSFER_ENTRY PM_OPENW                         
	TRANSFER_ENTRY PM_PERROR                        
	TRANSFER_ENTRY PM_READBIGLONG
	TRANSFER_ENTRY PM_READBIGSHORT                  
	TRANSFER_ENTRY PM_READLITTLELONG                
	TRANSFER_ENTRY PM_READLITTLESHORT               
	TRANSFER_ENTRY PM_USAGE
	TRANSFER_ENTRY PM_WRITEBIGLONG                  
	TRANSFER_ENTRY PM_WRITEBIGSHORT                 
	TRANSFER_ENTRY PM_WRITELITTLELONG               
	TRANSFER_ENTRY PM_WRITELITTLESHORT
	TRANSFER_ENTRY PM_READ_UNKNOWN_SIZE

;Module LIBPBM2
	TRANSFER_ENTRY PBM_READMAGICNUMBER              
	TRANSFER_ENTRY PBM_READPBM                      
	TRANSFER_ENTRY PBM_READPBMINIT                  
	TRANSFER_ENTRY PBM_READPBMINITREST
	TRANSFER_ENTRY PBM_READPBMROW

;Module LIBPBM3
	TRANSFER_ENTRY PBM_WRITEPBM                     
	TRANSFER_ENTRY PBM_WRITEPBMINIT                 
	TRANSFER_ENTRY PBM_WRITEPBMROW

;Module LIBPBM4
	TRANSFER_ENTRY PBM_GETC                         
	TRANSFER_ENTRY PBM_GETINT                       
	TRANSFER_ENTRY PBM_GETRAWBYTE

;Module LIBPBM5
	TRANSFER_ENTRY PBM_DEFAULTFONT                  
	TRANSFER_ENTRY PBM_DISSECTFONT                  
	TRANSFER_ENTRY PBM_DUMPFONT
	TRANSFER_ENTRY PBM_LOADFONT
	TRANSFER_ENTRY PBM_LOADBDFFONT
	TRANSFER_ENTRY MK_ARGVN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Library LIBPGM
;Module LIBPGM1
	TRANSFER_ENTRY PGM_INIT                         
	TRANSFER_ENTRY PGM_READPGM                      
	TRANSFER_ENTRY PGM_READPGMINIT                  
	TRANSFER_ENTRY PGM_READPGMINITREST
	TRANSFER_ENTRY PGM_READPGMROW

;Module LIBPGM2
	TRANSFER_ENTRY PGM_WRITEPGM                     
	TRANSFER_ENTRY PGM_WRITEPGMINIT                 
	TRANSFER_ENTRY PGM_WRITEPGMROW

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Library LIBPPM
;
;Module LIBPPM1
	TRANSFER_ENTRY PPM_INIT                         
	TRANSFER_ENTRY PPM_READPPM                      
	TRANSFER_ENTRY PPM_READPPMINIT                  
	TRANSFER_ENTRY PPM_READPPMINITREST
	TRANSFER_ENTRY PPM_READPPMROW

;Module LIBPPM2
	TRANSFER_ENTRY PPM_WRITEPPM                     
	TRANSFER_ENTRY PPM_WRITEPPMINIT                 
	TRANSFER_ENTRY PPM_WRITEPPMROW

;Module LIBPPM3
	TRANSFER_ENTRY PPM_ADDTOCOLORHASH               
	TRANSFER_ENTRY PPM_ADDTOCOLORHIST               
	TRANSFER_ENTRY PPM_ALLOCCOLORHASH               
	TRANSFER_ENTRY PPM_COLORHASHTOCOLORHIST
	TRANSFER_ENTRY PPM_COLORHISTTOCOLORHASH         
	TRANSFER_ENTRY PPM_COMPUTECOLORHASH             
	TRANSFER_ENTRY PPM_COMPUTECOLORHIST             
	TRANSFER_ENTRY PPM_FREECOLORHASH
	TRANSFER_ENTRY PPM_FREECOLORHIST                
	TRANSFER_ENTRY PPM_LOOKUPCOLOR

;Module LIBPPM4
	TRANSFER_ENTRY PPM_COLORNAME                    
	TRANSFER_ENTRY PPM_PARSECOLOR

;Module LIBPPM5
	TRANSFER_ENTRY PPMD_CIRCLE                      
	TRANSFER_ENTRY PPMD_FILL                        
	TRANSFER_ENTRY PPMD_FILLEDRECTANGLE             
	TRANSFER_ENTRY PPMD_FILL_DRAWPROC
	TRANSFER_ENTRY PPMD_FILL_INIT                   
	TRANSFER_ENTRY PPMD_LINE                        
	TRANSFER_ENTRY PPMD_POINT_DRAWPROC              
	TRANSFER_ENTRY PPMD_POLYSPLINE
	TRANSFER_ENTRY PPMD_SETLINECLIP                 
	TRANSFER_ENTRY PPMD_SETLINETYPE                 
	TRANSFER_ENTRY PPMD_SPLINE3

;Module BITIO
	TRANSFER_ENTRY PM_BITINIT
	TRANSFER_ENTRY PM_BITFINI
	TRANSFER_ENTRY PM_BITREAD
	TRANSFER_ENTRY PM_BITWRITE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Library LIBPNM
;Module LIBPNM1
	TRANSFER_ENTRY PNM_INIT                         
	TRANSFER_ENTRY PNM_READPNM                      
	TRANSFER_ENTRY PNM_READPNMINIT                  
	TRANSFER_ENTRY PNM_READPNMROW

;Module LIBPNM2
	TRANSFER_ENTRY PNM_WRITEPNM                     
	TRANSFER_ENTRY PNM_WRITEPNMINIT                 
	TRANSFER_ENTRY PNM_WRITEPNMROW

;Module LIBPNM3
	TRANSFER_ENTRY PNM_BACKGROUNDXEL                
	TRANSFER_ENTRY PNM_BACKGROUNDXELROW             
	TRANSFER_ENTRY PNM_BLACKXEL                     
	TRANSFER_ENTRY PNM_INVERTXEL
	TRANSFER_ENTRY PNM_PROMOTEFORMAT                
	TRANSFER_ENTRY PNM_PROMOTEFORMATROW             
	TRANSFER_ENTRY PNM_WHITEXEL

;Module LIBPNM4
	TRANSFER_ENTRY MEM_CREATE                       
	TRANSFER_ENTRY MEM_FREE                         
	TRANSFER_ENTRY PR_DUMP                          
	TRANSFER_ENTRY PR_LOAD_COLORMAP
	TRANSFER_ENTRY PR_LOAD_HEADER                   
	TRANSFER_ENTRY PR_LOAD_IMAGE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;	allocate extra space to allow for code modifications without changing 
;	the size of the shared image.
;
	.BLKB 2048-<.-TRANSFER_VECTOR>	; Reserve 4 pages.
;
	.END

$ TRANSVEC_DONE:
$!
$!   Create new options file if needed.
$!
$ if f$search("PBMPLUSSHR.OPT") .EQS. "" THEN GOTO NEW_OPTFILE
$ if f$cvtime(f$file("PBMPLUSSHR.OPT","CDT")) .GTS. PROC_CDT THEN GOTO OPTFILE_DONE
$ NEW_OPTFILE:
$ instruct = 1
$ write sys$output "Making new pbmplusshr.opt..."
$ CREATE PBMPLUSSHR.OPT
PBMplusShr /Share
Sys$Share:VAXCRTL /Share
PSECT_ATTR = ARGPROC_VERSION,GBL,NOSHR
PSECT_ATTR = OPTARG,GBL,NOSHR
PSECT_ATTR = OPTERR,GBL,NOSHR
PSECT_ATTR = OPTIND,GBL,NOSHR
PSECT_ATTR = OPTOPT,GBL,NOSHR
PSECT_ATTR = PGM_PBMMAXVAL,GBL,NOSHR
PSECT_ATTR = PNM_PBMMAXVAL,GBL,NOSHR
PSECT_ATTR = PPM_PBMMAXVAL,GBL,NOSHR
$!
$ OPTFILE_DONE:
$!
$ if f$search("PBMPLUSSHR.EXE") .EQS. "" THEN GOTO NEW_SHAREABLE
$ EXE_CDT = f$cvtime(f$file_attributes("PBMPLUSSHR.EXE","CDT"))
$ if EXE_CDT .LTS. PROC_CDT THEN GOTO NEW_SHAREABLE
$ if f$cvtime(f$file("[.PBM]LIBPBM.OLB","RDT")) .GTS. EXE_CDT THEN GOTO NEW_SHAREABLE
$ if f$cvtime(f$file("[.PGM]LIBPGM.OLB","RDT")) .GTS. EXE_CDT THEN GOTO NEW_SHAREABLE
$ if f$cvtime(f$file("[.PPM]LIBPPM.OLB","RDT")) .GTS. EXE_CDT THEN GOTO NEW_SHAREABLE
$ if f$cvtime(f$file("[.PNM]LIBPNM.OLB","RDT")) .GTS. EXE_CDT THEN GOTO NEW_SHAREABLE
$ GOTO SHAREABLE_DONE
$ NEW_SHAREABLE:
$ instruct = 1
$ write sys$output "Making new pbmplusshr.exe..."
$ Link /Map = PBMPLUSHSR.MAP /Share = SYS$DISK:[]PBMPLUSSHR.EXE Sys$Input/Option
COLLECT=FIRST,PBMPLUS_XVEC
COLLECT=GLOBALS1,PGM_PBMMAXVAL,PNM_PBMMAXVAL,PPM_PBMMAXVAL
COLLECT=GLOBALS2,ARGPROC_VERSION,OPTARG,OPTERR,OPTIND,OPTOPT

TRANSVEC.OBJ
[.PBM]LIBPBM/LIB,[.PGM]LIBPGM/LIB,[.PPM]LIBPPM/LIB
[.PNM]LIBPNM/LIB,SYS$SHARE:VAXCRTL/SHARE


UNSUPPORTED = 1			! force demand zero pages
GSMATCH=LEQUAL,2,1		! Major ID = 2, minor ID = 2

PSECT_ATTR = ARGPROC_VERSION,NOSHR
PSECT_ATTR = OPTARG,NOSHR
PSECT_ATTR = OPTERR,NOSHR
PSECT_ATTR = OPTIND,NOSHR
PSECT_ATTR = OPTOPT,NOSHR
PSECT_ATTR = PGM_PBMMAXVAL,NOSHR
PSECT_ATTR = PNM_PBMMAXVAL,NOSHR
PSECT_ATTR = PPM_PBMMAXVAL,NOSHR
!PSECT_ATTR = ,LCL,NOSHR
$!
$ SHAREABLE_DONE:
$ if .NOT. instruct then write sys$output "All PBMPLUSSHR files up to date."
$ if .NOT. instruct then exit $status
$ create sys$output

	Define the logical name PBMPLUSSHR as "disk:[dir]PBMPLUSSHR", where
	disk and [dir] are the disk and directory containing the
	shareable image PBMPLUSSHR.EXE and linker options file PBMPLUSSHR.OPT.

	You can then link an executable against the image with the command

	    LINK program.OBJ,PBMplusShr/Option

$ exit $status
