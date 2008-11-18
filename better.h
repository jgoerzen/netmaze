typedef enum {LINKS, GERADE, RECHTS} richtung;

/* Diese Struct ersetzt einen Haufen static-variablen fuer BB */
typedef struct { int ret;
                richtung zustand;
                int counter;
                long oldx;
                long oldy;
                int ausweichen;
                int jagd;
                int freund;
                char freundname[255];
                int tod;
                int exfreund;
		int alt_opfer;
} robodat_t;

extern robodat_t robodat;
