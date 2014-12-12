#include <string.h>

#include "nstring.h"
#include "pbm.h"

static char bit_table[2][3] = {
{1, 4, 0x10},
{2, 8, 0x40}
};

static int vmap_width;
static int vmap_height;

static int xres;
static int yres;

static char *vmap;


static void
init_map()
{
    int x, y;


    for(x = 0; x < vmap_width; ++x)
    {
        for(y = 0; y < vmap_height; ++y)
        {
            vmap[y*(vmap_width) + x] = 0x20;
        }
    }
}



static void
set_vmap(x, y)
    int x, y;
{
    int ix, iy;

    ix = x/2;
    iy = y/3;

    vmap[iy*(vmap_width) + ix] |= bit_table[x % 2][y % 3];
}



static void
fill_map(pbmfp)
    FILE *pbmfp;
{
    bit **pbm_image;
    int cols;
    int rows;
    int x;
    int y;

    pbm_image = pbm_readpbm(pbmfp, &cols, &rows);
    for(y = 0; y < rows && y < yres; ++y)
    {
        for(x = 0; x < cols && x < xres; ++x)
        {
            if(pbm_image[y][x] == PBM_WHITE)
            {
                set_vmap(x, y);
            }
        }
    }
}


static void
print_map()
{
    int x, y;
    int last_byte;

#ifdef BUFFERED
    char *iobuf;
    iobuf = (char *)malloc(BUFSIZ);
    if(iobuf == NULL)
    {
        pm_message( "Can't allocate space for I/O buffer.  "
                    "Using unbuffered I/O...\n" );
        setbuf(stdout, NULL);
    }
    else
    {
        setbuf(stdout, iobuf);
    }
#endif

    fputs("\033[H\033[J", stdout);	/* clear screen */
    fputs("\033[?3h", stdout);	/* 132 column mode */
    fputs("\033)}\016", stdout);	/* mosaic mode */

    for(y = 0; y < vmap_height; ++y)
    {
        for(last_byte = vmap_width - 1;
            last_byte >= 0
                && vmap[y * vmap_width + last_byte] == 0x20;
            --last_byte)
            ;

        for(x = 0; x <= last_byte; ++x)
        {
            fputc(vmap[y*(vmap_width) + x], stdout);
        }
        fputc('\n', stdout);
    }

    fputs("\033(B\017", stdout);
}



int
main(int argc, char * argv[]) {
    int argn;
    const char *pbmfile;
    FILE *pbmfp;
    const char *usage="[pbmfile]";

    pbm_init( &argc, argv );
    for(argn = 1;
        argn < argc && argv[argn][0] == '-' && strlen(argv[argn]) > 1;
        ++argn)
    {
        pm_usage(usage);
    }

    if(argn >= argc)
    {
        pbmfile = "-";
    }
    else if(argc - argn != 1)
    {
        pm_usage(usage);
    }
    else
    {
        pbmfile = argv[argn];
    }

    if(STREQ(pbmfile, "-"))
    {
        pbmfp = stdin;
    }
    else
    {
        pbmfp = pm_openr( argv[argn] );
    }

    vmap_width = 132;
    vmap_height = 23;

    xres = vmap_width * 2;
    yres = vmap_height * 3;

    vmap = malloc(vmap_width * vmap_height * sizeof(char));
    if(vmap == NULL)
	{
        pm_error( "Cannot allocate memory" );
	}

    init_map();
    fill_map(pbmfp);
    print_map();
    /* If the program failed, it previously aborted with nonzero completion
       code, via various function calls.
    */
    return 0; 
}



