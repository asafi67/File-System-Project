




// Structure for directory entry
typedef struct {
    char name[256];
    unsigned int size;
    unsigned int loc;
} DE;

int initDir(int minEntries, DE* parent);