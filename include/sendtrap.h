
struct specificTrap{
    unsigned short trapNo;
    unsigned char* trapOID;
};

void sendtrap(unsigned char* cmd);
