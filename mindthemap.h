#ifndef MINDTHEMAP_H
#define MINDTHEMAP_H

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <keyboard.h>
#include <thread.h>

/* Maximum number of children per node */
#define MAXCHILDREN 32

/* Maximum length of node text */
#define MAXTEXT 256

/* Application modes */
enum {
	NORMAL = 0,
	INSERT = 1,
	DRAGGING = 2  /* New mode for node dragging */
};

/* Corner types for sprite selection */
enum {
	CORNER_TL = 0,  /* Top-left */
	CORNER_TR = 1,  /* Top-right */
	CORNER_BL = 2,  /* Bottom-left */
	CORNER_BR = 3   /* Bottom-right */
};

/* Color combinations for sprite selection */
enum {
	SPRITE_NORMAL = 0,    /* Normal node (white bg, blue border) */
	SPRITE_SELECTED = 1,  /* Selected node (blue bg, white border) */
	SPRITE_ROOT = 2,      /* Root node (light blue bg, blue border) */
	SPRITE_ALT = 3        /* Alternate node (pale blue bg, blue border) */
};

/* Node structure */
typedef struct Node {
	char text[MAXTEXT];
	Point pos;
	Rectangle bounds;
	struct Node *parent;
	struct Node *children[MAXCHILDREN];
	int nchildren;
	int manual_pos;  /* Flag to indicate manual positioning */
	Point drag_offset;  /* Offset from mouse position during drag */
	int selected;    /* Flag to indicate node selection state */
} Node;

/* Pre-compiled corner sprites - 12x12 pixels, 1px line border */
/* Format: Each sprite is 12 rows of 2 bytes each (16 bits per row, with 12 bits used)
   0 = transparent (background color)
   1 = border color */

/* Normal node corners (white background, blue border) */
static uchar corner_normal[4][24] = {  /* 2 bytes per row * 12 rows */
	/* Top-left */
	{
		0x0F,0x80,  /* ....XXXXX... */
		0x1F,0xC0,  /* ...XXXXXX... */
		0x3F,0xC0,  /* ..XXXXXXX... */
		0x7F,0xC0,  /* .XXXXXXX.... */
		0x7F,0xC0,  /* .XXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0   /* XXXXXXXX.... */
	},
	/* Top-right (mirrored) */
	{
		0xF1,0x00,  /* XXXX.....X.. */
		0xF9,0x80,  /* XXXXX....X.. */
		0xFD,0xC0,  /* XXXXXX...X.. */
		0xFD,0xC0,  /* XXXXXX...X.. */
		0xFD,0xC0,  /* XXXXXX...X.. */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0   /* XXXXXXXX.... */
	},
	/* Bottom-left */
	{
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0x7F,0xC0,  /* .XXXXXXX.... */
		0x7F,0xC0,  /* .XXXXXXX.... */
		0x3F,0xC0,  /* ..XXXXXXX... */
		0x1F,0xC0,  /* ...XXXXXX... */
		0x0F,0x80   /* ....XXXXX... */
	},
	/* Bottom-right */
	{
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFD,0xC0,  /* XXXXXX...X.. */
		0xFD,0xC0,  /* XXXXXX...X.. */
		0xFD,0xC0,  /* XXXXXX...X.. */
		0xF9,0x80,  /* XXXXX....X.. */
		0xF1,0x00   /* XXXX.....X.. */
	}
};

/* Selected node corners (blue background, white border) */
static uchar corner_selected[4][24] = {  /* 2 bytes per row * 12 rows */
	/* Top-left */
	{
		0x00,0x00,  /* ............ */
		0x07,0x00,  /* .....XXX.... */
		0x0F,0x80,  /* ....XXXXX... */
		0x1F,0xC0,  /* ...XXXXXX... */
		0x3F,0xC0,  /* ..XXXXXXX... */
		0x7F,0xC0,  /* .XXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0   /* XXXXXXXX.... */
	},
	/* Top-right (mirrored) */
	{
		0x00,0x00,  /* ............ */
		0x70,0x00,  /* .XXX........ */
		0xF8,0x00,  /* XXXXX....... */
		0xFC,0x00,  /* XXXXXX...... */
		0xFE,0x00,  /* XXXXXXX..... */
		0xFF,0x00,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0   /* XXXXXXXX.... */
	},
	/* Bottom-left */
	{
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0x7F,0xC0,  /* .XXXXXXX.... */
		0x3F,0xC0,  /* ..XXXXXXX... */
		0x1F,0xC0,  /* ...XXXXXX... */
		0x0F,0x80,  /* ....XXXXX... */
		0x07,0x00,  /* .....XXX.... */
		0x00,0x00   /* ............ */
	},
	/* Bottom-right */
	{
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0xC0,  /* XXXXXXXX.... */
		0xFF,0x00,  /* XXXXXXXX.... */
		0xFE,0x00,  /* XXXXXXX..... */
		0xFC,0x00,  /* XXXXXX...... */
		0xF8,0x00,  /* XXXXX....... */
		0x70,0x00,  /* .XXX........ */
		0x00,0x00   /* ............ */
	}
};

/* Connection point sprite - 6x6 pixels */
static uchar conn_sprite[12] = {  /* 2 bytes per row * 6 rows */
	0x3C,0x00,  /* ..XXXX.... */
	0x7E,0x00,  /* .XXXXXX... */
	0xFF,0x00,  /* XXXXXXXX.. */
	0xFF,0x00,  /* XXXXXXXX.. */
	0x7E,0x00,  /* .XXXXXX... */
	0x3C,0x00   /* ..XXXX.... */
};

/* Global variables */
extern int mode;
extern Node *root;
extern Node *current;
extern struct Image *screen;
extern struct Font *font;
extern struct Display *display;
extern Rectangle maprect;

/* Rio-inspired colors */
extern Image *back;    /* Background - pale yellow */
extern Image *high;    /* Highlight - pale blue */
extern Image *bord;    /* Border - dark blue */
extern Image *text;    /* Text - black */
extern Image *pale;    /* Pale - light blue */
extern Image *corner_sprites[4][4];  /* Corner sprites for each color combination */
extern Image *conn_dots[4];          /* Connection dots for each color */

/* Grid settings for snap-to-grid */
#define GRID_SIZE 30  /* Size of grid cells */

/* Function prototypes */
void setupdraw(void);
void initmap(void);
void initcolors(void);
int nodewidth(char *text);
void roundedrect(Image *dst, Rectangle r, Image *src, Point sp, int style);
void drawconnection(Point from, Point to, int thickness, Image *color);
Node* createnode(char *text, Node *parent);
void addchild(Node *parent);
void deletenode(Node *node);
void layoutmap(Node *node, int depth);
void drawmap(void);
void drawnode(Node *node);
void navigate(Rune key);
void switchmode(int newmode);
void handlekey(Rune key, Event *ev);
void resdraw(void);
void eresized(int new);
void usage(void);
void center_tree_auto(Node *node, int offset);
Node* findnode(Node *node, Point p);
Point snaptoGrid(Point p);
void updatedrag(Node *node, Point mouse);

/* File operations */
void savenode(int fd, Node *node);
Node* loadnode(int fd, Node *parent);
void savemap(char *filename);
void loadmap(char *filename);
void handlecmd(char *cmd);
int pipeline(char *fmt, ...);

#endif 