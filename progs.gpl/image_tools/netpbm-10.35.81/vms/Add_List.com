$ VERIFY = F$Verify (0)
$!
$!     ADD_LIST.COM command procedure
$!         Usage:
$!             ADD_LIST library file_spec [logical_name_table]
$!
$!     Last Modified: 18-JAN-1991 Rick Dyson
$!
$!     Escape routes
$ On Control_Y Then GoTo FINISH
$ On Error     Then GoTo FINISH
$ On Warning   Then GoTo FINISH
$ On Severe    Then GoTo FINISH
$!
$!     We're out'a here if the calling parameter is null
$ P2 = F$Edit (P2, "TRIM, UPCASE")
$ If P2 .eqs. "" Then GoTo FINISH
$!
$!     Check logical name table argument and default if necessary.
$!
$ TABLE = F$Edit (P3, "UNCOMMENT, UPCASE, TRIM")
$ If (TABLE .eqs. "PROCESS")
$    Then
$    Else If (TABLE .eqs. "GROUP")
$            Then
$            Else If (TABLE .eqs. "JOB")
$                    Then
$                    Else If (TABLE .eqs. "SYSTEM")
$                            Then
$                            Else
$                                TABLE = "Process"
$                         EndIf
$                 EndIf
$         EndIf
$ EndIf
$!
$! Check the first value in the library list
$ LIB = P1
$ X = F$TrnLnm (LIB, "LNM$''TABLE'")
$ If X .eqs. "" Then GoTo INSERT
$ If X .eqs. P2 Then GoTo FINISH
$!
$! Find the first free logical to assign the library file to
$ BASE = P1 + "_"
$ N = 1
$NEXTLIB:
$   LIB := 'BASE''N'
$   X = F$TrnLnm (LIB, "LNM$''TABLE'")
$   If X .eqs. "" Then GoTo INSERT
$   If X .eqs. P2 Then GoTo FINISH
$   N = N + 1
$   GoTo NEXTLIB
$!
$! Add the library file to the library file list
$INSERT:
$   Define /'TABLE' 'LIB' 'P2'
$FINISH:
$   VERIFY = F$Verify (VERIFY)
$   Exit
