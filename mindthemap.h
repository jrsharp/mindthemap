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
	DRAGGING = 2,
	CANVAS_DRAG = 3
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

/* Global variables */
extern int mode;
extern Node *root;
extern Node *current;
extern struct Image *screen;
extern struct Font *font;
extern struct Display *display;
extern Rectangle maprect;
extern Point viewport;  /* Current viewport offset for panning */
extern Point pan_start;  /* Starting point for panning */
extern int panning;  /* Flag to indicate if we're panning the viewport */

/* Rio-inspired colors */
extern Image *back;    /* Background - pale yellow */
extern Image *high;    /* Highlight - pale blue */
extern Image *bord;    /* Border - dark blue */
extern Image *text;    /* Text - black */
extern Image *pale;    /* Pale - light blue */

/* Grid settings for snap-to-grid */
#define GRID_SIZE 15  /* Size of grid cells - reduced from 30 for finer control */

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