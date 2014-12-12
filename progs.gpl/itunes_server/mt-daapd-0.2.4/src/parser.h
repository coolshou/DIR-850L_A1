typedef union {
    unsigned int ival;
    char *cval;
    PL_NODE *plval;    
} YYSTYPE;
#define	OR	257
#define	AND	258
#define	ARTIST	259
#define	ALBUM	260
#define	GENRE	261
#define	PATH	262
#define	COMPOSER	263
#define	ORCHESTRA	264
#define	CONDUCTOR	265
#define	GROUPING	266
#define	TYPE	267
#define	COMMENT	268
#define	EQUALS	269
#define	LESS	270
#define	LESSEQUAL	271
#define	GREATER	272
#define	GREATEREQUAL	273
#define	IS	274
#define	INCLUDES	275
#define	NOT	276
#define	ID	277
#define	NUM	278
#define	DATE	279
#define	YEAR	280
#define	BPM	281
#define	BITRATE	282
#define	DATEADDED	283
#define	BEFORE	284
#define	AFTER	285
#define	AGO	286
#define	INTERVAL	287


extern YYSTYPE yylval;
