struct decode {
  struct decode *branch[2];
  int leaf;
}; 

extern struct decode * free_decode;
extern struct decode first_decode[2048];
extern struct decode * second_decode;

void 
init_decoder(void);

void 
crw_init_tables(unsigned int const table);

const int * 
make_decoder_int (const int * const source, 
                  int         const level);

unsigned char * 
make_decoder(const unsigned char * const source, 
             int                   const level);
