typedef struct
{
    short left;
    short right;
    short bottom;
    short top;
} window_s;

typedef struct
{
    window_s	window;
    window_s	active_window;
    short	    frame;
    short       storage_type;
    short       num_chan;
    short       num_matte;
    short       num_aux;
    unsigned short       revision;
    char        gamma[16];
    char        red_pri[24];
    char        green_pri[24];
    char        blue_pri[24];
    char        white_pri[24];
    long        job_num;
    char        name[128];
    char        desc[128];
    char        program[64];
    char        machine[32];
    char        user[32];
    char        date[20];
    char        aspect[24];
    char        aspect_ratio[8];
    char        chan[32];
    short       field;
    char        time[12];
    char        filter[32];
    short       chan_bits;
    short       matte_type;
    short       matte_bits;
    short       aux_type;
    short       aux_bits;
    char        aux[32];
    char        space[36];
    long        next;
} rlahdr;
