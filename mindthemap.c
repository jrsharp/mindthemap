#include "mindthemap.h"

enum {
	MARGIN = 30,      /* Reduced margin for better use of space */
	HSPACE = 80,      /* Reduced horizontal space between nodes */
	VSPACE = 50,      /* Reduced vertical space between nodes */
	MINW = 100,       /* Reduced minimum node width */
	NODEH = 30,       /* Reduced node height */
	PADDING = 10,     /* Reduced text padding inside nodes */
	CORNER = 8,       /* Corner sprite size (unchanged) */
	CONN = 4          /* Connection point sprite size (unchanged) */
};

/* Rio-inspired colors */
Image *back;    /* Background - pale yellow */
Image *high;    /* Highlight - pale blue */
Image *bord;    /* Border - dark blue */
Image *text;    /* Text - black */
Image *pale;    /* Pale - light blue */
Image *corner_sprites[4][4];  /* Corner sprites for each color combination */
Image *conn_dots[4];          /* Connection dots for each color */

/* Global variables */
int mode = NORMAL;
Node *root;
Node *current;
struct Image *screen;
struct Font *font;
struct Display *display;
Rectangle maprect;
Point viewport = {0, 0};  /* Current viewport offset for panning */
Point pan_start = {0, 0};  /* Starting point for panning */
int panning = 0;  /* Flag to indicate if we're panning the viewport */
char *argv0;

/* Initialize colors */
void
initcolors(void)
{
	back = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xFFFFF0FF);  /* Light cream background */
	high = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xB0E0FFFF);  /* Sky blue for root nodes */
	bord = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x000066FF);  /* Navy blue for borders */
	text = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x000000FF);  /* Black for text */
	pale = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xE0E8F0FF);  /* Light steel blue for regular nodes */
	
	if(back == nil || high == nil || bord == nil || text == nil || pale == nil)
		sysfatal("allocimage failed");
}

/* Calculate node width based on text */
int
nodewidth(char *text)
{
	Point p;
	p = stringsize(font, text);
	return p.x + (2 * PADDING);  /* Text width plus padding on both sides */
}

/* Draw a rounded rectangle using bezier curves */
void
roundedrect(Image *dst, Rectangle r, Image *src, Point sp, int style)
{
	int radius = 8;  /* Corner radius */
	Point p[12];     /* Points for the rounded corners */
	
	/* Draw the main rectangle body */
	Rectangle body = r;
	body.min.x += radius;
	body.max.x -= radius;
	draw(dst, body, src, nil, sp);
	
	/* Draw top and bottom bars */
	draw(dst, Rect(r.min.x+radius, r.min.y, r.max.x-radius, r.min.y+radius), src, nil, sp);
	draw(dst, Rect(r.min.x+radius, r.max.y-radius, r.max.x-radius, r.max.y), src, nil, sp);
	
	/* Draw left and right bars */
	draw(dst, Rect(r.min.x, r.min.y+radius, r.min.x+radius, r.max.y-radius), src, nil, sp);
	draw(dst, Rect(r.max.x-radius, r.min.y+radius, r.max.x, r.max.y-radius), src, nil, sp);
	
	/* Use the appropriate border color based on the node's state */
	Image *border = bord;  /* Default to normal border color */
	
	/* Top-left corner */
	p[0] = Pt(r.min.x+radius, r.min.y);          /* Start */
	p[1] = Pt(r.min.x+radius/2, r.min.y);        /* Control point 1 */
	p[2] = Pt(r.min.x, r.min.y+radius/2);        /* Control point 2 */
	p[3] = Pt(r.min.x, r.min.y+radius);          /* End */
	
	/* Top-right corner */
	p[4] = Pt(r.max.x-radius, r.min.y);
	p[5] = Pt(r.max.x-radius/2, r.min.y);
	p[6] = Pt(r.max.x, r.min.y+radius/2);
	p[7] = Pt(r.max.x, r.min.y+radius);
	
	/* Bottom-right corner */
	p[8] = Pt(r.max.x, r.max.y-radius);
	p[9] = Pt(r.max.x, r.max.y-radius/2);
	p[10] = Pt(r.max.x-radius/2, r.max.y);
	p[11] = Pt(r.max.x-radius, r.max.y);
	
	/* Draw the bezier curves for each corner */
	bezier(dst, p[0], p[1], p[2], p[3], Enddisc, Enddisc, 1, border, ZP);
	bezier(dst, p[4], p[5], p[6], p[7], Enddisc, Enddisc, 1, border, ZP);
	bezier(dst, p[8], p[9], p[10], p[11], Enddisc, Enddisc, 1, border, ZP);
	
	/* Bottom-left corner */
	p[0] = Pt(r.min.x, r.max.y-radius);
	p[1] = Pt(r.min.x, r.max.y-radius/2);
	p[2] = Pt(r.min.x+radius/2, r.max.y);
	p[3] = Pt(r.min.x+radius, r.max.y);
	bezier(dst, p[0], p[1], p[2], p[3], Enddisc, Enddisc, 1, border, ZP);
	
	/* Draw straight lines for the borders */
	draw(dst, Rect(r.min.x+radius, r.min.y, r.max.x-radius, r.min.y+1), border, nil, ZP);  /* Top */
	draw(dst, Rect(r.min.x+radius, r.max.y-1, r.max.x-radius, r.max.y), border, nil, ZP);  /* Bottom */
	draw(dst, Rect(r.min.x, r.min.y+radius, r.min.x+1, r.max.y-radius), border, nil, ZP);  /* Left */
	draw(dst, Rect(r.max.x-1, r.min.y+radius, r.max.x, r.max.y-radius), border, nil, ZP);  /* Right */
}

void
setupdraw(void)
{
	if(display == nil)
		sysfatal("display not initialized");
	
	screen = display->image;
	if(screen == nil)
		sysfatal("screen not initialized");
	
	font = display->defaultfont;
	if(font == nil)
		sysfatal("font not initialized");
	
	/* Initialize colors */
	initcolors();
}

/* Initialize the mindmap with a root node */
void
initmap(void)
{
	root = createnode("Main Topic", nil);
	current = root;
	layoutmap(root, 0);
}

/* Create a new node */
Node*
createnode(char *text, Node *parent)
{
	Node *n;
	
	n = malloc(sizeof(Node));
	if(n == nil)
		sysfatal("malloc failed: %r");
	
	memset(n, 0, sizeof(Node));
	strncpy(n->text, text, MAXTEXT-1);
	n->text[MAXTEXT-1] = '\0';
	n->parent = parent;
	n->nchildren = 0;
	n->selected = 0;
	
	/* Add to parent's children if it has a parent */
	if(parent != nil && parent->nchildren < MAXCHILDREN)
		parent->children[parent->nchildren++] = n;
	
	return n;
}

/* Add a child to a node */
void
addchild(Node *parent)
{
	Node *child;
	
	if(parent->nchildren >= MAXCHILDREN)
		return;
	
	child = createnode("", parent);  /* Start with empty text */
	current = child;
	switchmode(INSERT);
	
	/* Re-calculate layout */
	layoutmap(root, 0);
}

/* Add a sibling to a node */
void
addsibling(Node *node)
{
	Node *sibling;
	
	if(node == root || node->parent == nil)  /* Can't add sibling to root */
		return;
	
	if(node->parent->nchildren >= MAXCHILDREN)
		return;
	
	sibling = createnode("", node->parent);  /* Start with empty text */
	current = sibling;
	switchmode(INSERT);
	
	/* Re-calculate layout */
	layoutmap(root, 0);
}

/* Delete a node and its children */
void
deletenode(Node *node)
{
	int i;
	
	if(node == nil || node == root)
		return;
	
	/* Recursively delete all children first */
	for(i = 0; i < node->nchildren; i++)
		deletenode(node->children[i]);
	
	/* Remove this node from parent's children */
	if(node->parent != nil) {
		for(i = 0; i < node->parent->nchildren; i++) {
			if(node->parent->children[i] == node) {
				/* Shift remaining children down */
				while(i < node->parent->nchildren - 1) {
					node->parent->children[i] = node->parent->children[i + 1];
					i++;
				}
				node->parent->nchildren--;
				break;
			}
		}
	}
	
	free(node);
}

/* Calculate positions for nodes */
void
layoutmap(Node *node, int depth)
{
	int i, width;
	static int xpos = 0;
	static int level_height[100] = {0};  /* Heights for each level */
	static int max_width = 0;  /* Track maximum width for centering */
	
	if(depth == 0) {
		/* Root initialization */
		memset(level_height, 0, sizeof(level_height));
		xpos = MARGIN;  /* Remove dependency on maprect */
		max_width = 0;
	}
	
	/* Calculate node width */
	width = nodewidth(node->text);
	if(width < MINW)
		width = MINW;
	
	/* Only position nodes that aren't manually placed */
	if(!node->manual_pos) {
		/* Position the node */
		node->pos.x = xpos + depth * (width + HSPACE);
		
		if(depth > 0) {
			/* Position vertically based on previous nodes at this level */
			node->pos.y = MARGIN + level_height[depth];
			level_height[depth] += NODEH + VSPACE;
		} else {
			/* Root node at a fixed position if not manually placed */
			node->pos.y = MARGIN;
		}
		
		/* Set bounds rectangle */
		node->bounds = (Rectangle){
			Pt(node->pos.x, node->pos.y),
			Pt(node->pos.x + width, node->pos.y + NODEH)
		};
	}
	
	/* Track maximum width */
	if(node->pos.x + width > max_width)
		max_width = node->pos.x + width;
	
	/* Layout all children */
	for(i = 0; i < node->nchildren; i++) {
		layoutmap(node->children[i], depth + 1);
	}
}

/* Draw all connection lines in the tree */
void
drawlines(Node *node)
{
	int i;
	
	if(node == nil)
		return;
	
	/* Draw connecting lines to children */
	for(i = 0; i < node->nchildren; i++) {
		Point from, to;
		
		from.x = node->bounds.min.x + (node->bounds.max.x - node->bounds.min.x) / 2;
		from.y = node->bounds.max.y;
		
		to.x = node->children[i]->bounds.min.x + 
			(node->children[i]->bounds.max.x - node->children[i]->bounds.min.x) / 2;
		to.y = node->children[i]->bounds.min.y;
		
		drawconnection(from, to, 1, bord);  /* Always use 1px lines */
	}
	
	/* Recursively draw lines for children */
	for(i = 0; i < node->nchildren; i++) {
		drawlines(node->children[i]);
	}
}

/* Draw a node and its children */
void
drawnode(Node *node)
{
	int i;
	Rectangle r;
	Point txtp;
	Image *bg, *fg;
	int depth = 0;
	Node *p;
	int style = 0;
	
	if(node == nil)
		return;
	
	/* Calculate node depth */
	for(p = node->parent; p != nil; p = p->parent)
		depth++;
	
	r = node->bounds;
	
	/* Apply viewport offset to bounds */
	r.min.x -= viewport.x;
	r.min.y -= viewport.y;
	r.max.x -= viewport.x;
	r.max.y -= viewport.y;
	
	/* Skip if completely outside window bounds */
	if(r.max.x <= screen->r.min.x || r.min.x >= screen->r.max.x ||
	   r.max.y <= screen->r.min.y || r.min.y >= screen->r.max.y)
		return;
	
	/* Select colors based on node state and depth */
	if(node == current) {
		bg = bord;
		fg = back;
	} else if(node == root) {
		bg = high;
		fg = text;
	} else {
		bg = depth % 2 ? pale : back;
		fg = text;
	}
	
	/* Draw node with bezier corners */
	roundedrect(screen, r, bg, ZP, style);
	
	/* Draw node text if there's room */
	if(r.max.x - r.min.x > 2*PADDING) {
		txtp.x = r.min.x + PADDING;
		txtp.y = r.min.y + (NODEH - font->height) / 2;
		string(screen, txtp, fg, ZP, font, node->text);
	}
	
	/* Draw children after parent */
	for(i = 0; i < node->nchildren; i++) {
		drawnode(node->children[i]);
	}
}

void
drawmap(void)
{
	Rectangle winr;
	char buf[128];
	
	if(screen == nil || display == nil)
		return;
	
	/* Get the actual window rectangle */
	winr = screen->r;
	
	/* Clear entire window first */
	draw(screen, winr, back, nil, ZP);
	
	if(root != nil) {
		/* Set maprect to window bounds */
		maprect = winr;
		
		/* Leave room for status line */
		maprect.max.y -= font->height + 5;
		
		/* Recalculate layout before drawing */
		layoutmap(root, 0);
		
		/* First draw all connection lines */
		drawlines(root);
		
		/* Then draw all nodes on top */
		drawnode(root);
		
		/* Draw status line at bottom of window */
		Rectangle statusr = Rect(winr.min.x, winr.max.y - font->height - 5,
			winr.max.x, winr.max.y - 5);
		draw(screen, statusr, bord, nil, ZP);
		
		/* Draw mode and canvas drag status on left side */
		char *modestr;
		switch(mode) {
			case NORMAL: modestr = "NORMAL"; break;
			case INSERT: modestr = "INSERT"; break;
			case DRAGGING: modestr = "DRAGGING"; break;
			case CANVAS_DRAG: modestr = panning ? "CANVAS DRAG (active)" : "CANVAS DRAG"; break;
			default: modestr = "UNKNOWN"; break;
		}
		string(screen, Pt(statusr.min.x + 5, statusr.min.y), back, ZP, font, modestr);
		
		/* Draw version and coordinates on right side */
		snprint(buf, sizeof(buf), "mindthemap 0.1 [%d,%d]", viewport.x, viewport.y);
		string(screen, Pt(statusr.max.x - stringwidth(font, buf) - 5, statusr.min.y),
			back, ZP, font, buf);
	}
	
	flushimage(display, 1);
}

/* Draw connection between nodes with bezier curves */
void
drawconnection(Point from, Point to, int thickness, Image *color)
{
	Point p[4];
	int dy = to.y - from.y;
	
	/* Apply viewport offset to points */
	from.x -= viewport.x;
	from.y -= viewport.y;
	to.x -= viewport.x;
	to.y -= viewport.y;
	
	/* Only draw if both points are within bounds */
	if(from.x < maprect.min.x || from.x >= maprect.max.x ||
	   from.y < maprect.min.y || from.y >= maprect.max.y ||
	   to.x < maprect.min.x || to.x >= maprect.max.x ||
	   to.y < maprect.min.y || to.y >= maprect.max.y)
		return;
	
	/* Calculate control points for a smooth curve */
	p[0] = from;
	p[1] = Pt(from.x, from.y + dy/3);  /* First control point at 1/3 distance */
	p[2] = Pt(to.x, from.y + 2*dy/3);  /* Second control point at 2/3 distance */
	p[3] = to;
	
	/* Draw the bezier curve */
	bezier(screen, p[0], p[1], p[2], p[3], Enddisc, Enddisc, thickness, color, ZP);
}

/* Handle vim-like navigation */
void
navigate(Rune key)
{
	int i, idx = -1;
	
	/* Find current node's index within parent */
	if(current->parent != nil) {
		for(i = 0; i < current->parent->nchildren; i++) {
			if(current->parent->children[i] == current) {
				idx = i;
				break;
			}
		}
	}
	
	switch(key) {
	case 'h':  /* Move to parent */
		if(current->parent != nil)
			current = current->parent;
		break;
	case 'j':  /* Move down to next sibling */
		if(current->parent != nil && idx >= 0 && idx < current->parent->nchildren - 1)
			current = current->parent->children[idx + 1];
		break;
	case 'k':  /* Move up to previous sibling */
		if(current->parent != nil && idx > 0)
			current = current->parent->children[idx - 1];
		break;
	case 'l':  /* Move to first child */
		if(current->nchildren > 0)
			current = current->children[0];
		break;
	}
}

/* Switch between modes */
void
switchmode(int newmode)
{
	mode = newmode;
	drawmap();  /* Redraw to update status line */
}

/* Handle keypresses */
void
handlekey(Rune key, Event *ev)
{
	char buf[1024];
	
	/* Handle keys based on mode */
	if(mode == NORMAL || mode == CANVAS_DRAG) {
		switch(key) {
		case Kesc:  /* Exit canvas drag mode */
			if(mode == CANVAS_DRAG) {
				panning = 0;
				switchmode(NORMAL);
			}
			break;
		case 'i':  /* Enter insert mode */
			if(mode == NORMAL)
				switchmode(INSERT);
			break;
		case '\t':  /* Add child to current node */
			if(mode == NORMAL) {
				addchild(current);
				drawmap();
			}
			break;
		case '\n':  /* Add sibling to current node */
			if(mode == NORMAL) {
				addsibling(current);
				drawmap();
			}
			break;
		case 'd':  /* Delete current node */
			if(mode == NORMAL && current != root) {
				Node *parent = current->parent;
				deletenode(current);
				current = parent;
				layoutmap(root, 0);
				drawmap();
			}
			break;
		case 'h':
		case 'j':
		case 'k':
		case 'l':
			if(mode == NORMAL) {
				navigate(key);
				drawmap();
			}
			break;
		case 'q':  /* Quit command */
		case 'r':  /* Read file */
		case 'w':  /* Write file */
		case '<':  /* Read from command */
		case '>':  /* Write to command */
			if(mode == NORMAL) {
				buf[0] = key;
				buf[1] = 0;
				if (eenter("Cmd", buf, sizeof(buf), &ev->mouse) > 0)
					handlecmd(buf);
			}
			break;
		case ' ':  /* Toggle canvas drag mode */
			if(mode == NORMAL)
				switchmode(CANVAS_DRAG);
			else if(mode == CANVAS_DRAG)
				switchmode(NORMAL);
			break;
		}
	} else if(mode == INSERT) {
		int len = strlen(current->text);
		
		if(key == '\n' || key == Kesc) {
			/* Exit insert mode */
			switchmode(NORMAL);
		} else if(key == '\b') {
			/* Backspace */
			if(len > 0) {
				current->text[len - 1] = '\0';
				drawmap();  /* Redraw after text change */
			}
		} else if(len < MAXTEXT - 1 && key >= ' ' && key < Runemax) {
			/* Add character */
			current->text[len] = key;
			current->text[len + 1] = '\0';
			drawmap();  /* Redraw after text change */
		}
	}
}

void
resdraw(void)
{
	if(getwindow(display, Refnone) < 0)
		sysfatal("getwindow: %r");
	drawmap();
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0)
		sysfatal("can't reattach to window");
	drawmap();
}

void
usage(void)
{
	fprint(2, "usage: %s [file]\n", argv0);
	exits("usage");
}

/* Find node under point */
Node*
findnode(Node *node, Point p)
{
	Node *found;
	int i;
	
	if(node == nil)
		return nil;
	
	/* Apply viewport offset to point for hit testing */
	Point test = addpt(p, viewport);
	
	/* Check children first (reverse order for top-to-bottom hit testing) */
	for(i = node->nchildren - 1; i >= 0; i--) {
		found = findnode(node->children[i], p);
		if(found != nil)
			return found;
	}
	
	/* Check if point is within this node's bounds */
	if(ptinrect(test, node->bounds))
		return node;
	
	return nil;
}

/* Snap point to grid */
Point
snaptoGrid(Point p)
{
	p.x = ((p.x + GRID_SIZE/2) / GRID_SIZE) * GRID_SIZE;
	p.y = ((p.y + GRID_SIZE/2) / GRID_SIZE) * GRID_SIZE;
	return p;
}

/* Update node position during drag */
void
updatedrag(Node *node, Point mouse)
{
	Point newpos;
	int width;
	
	if(node == nil)
		return;
	
	/* Calculate new position based on mouse and drag offset */
	newpos = addpt(addpt(mouse, viewport), node->drag_offset);
	
	/* Snap to grid */
	newpos = snaptoGrid(newpos);
	
	/* Calculate new bounds */
	width = nodewidth(node->text);
	if(width < MINW)
		width = MINW;
	
	/* Update node position and bounds */
	node->pos = newpos;
	node->bounds = (Rectangle){
		Pt(newpos.x, newpos.y),
		Pt(newpos.x + width, newpos.y + NODEH)
	};
	
	node->manual_pos = 1;  /* Mark as manually positioned */
}

/* Save node and its children to file */
void
savenode(int fd, Node *node)
{
	int i;
	char buf[MAXTEXT + 256];  /* text + metadata */
	
	if(node == nil)
		return;
	
	/* Format: "NODE text x y manual_pos nchildren\n" */
	snprint(buf, sizeof(buf), "NODE %s %d %d %d %d\n",
		node->text,
		node->pos.x,
		node->pos.y,
		node->manual_pos,
		node->nchildren);
	
	/* Write node data */
	if(write(fd, buf, strlen(buf)) < 0)
		sysfatal("write failed: %r");
	
	/* Recursively save children */
	for(i = 0; i < node->nchildren; i++)
		savenode(fd, node->children[i]);
}

/* Load node from file */
Node*
loadnode(int fd, Node *parent)
{
	char buf[MAXTEXT + 256];
	char *toks[8];
	Node *node;
	int ntok, x, y, manual, nchildren;
	int i, textlen, width;
	char *text, *p;
	
	/* Read a line */
	for(i = 0; i < sizeof(buf)-1; i++) {
		if(read(fd, buf+i, 1) != 1)
			return nil;
		if(buf[i] == '\n') {
			buf[i+1] = '\0';
			break;
		}
	}
	buf[sizeof(buf)-1] = '\0';
	
	/* Parse node data */
	if(strncmp(buf, "NODE ", 5) != 0)
		return nil;
	
	/* Find the last four space-separated numbers */
	p = buf + strlen(buf);
	for(i = 0; i < 4; i++) {
		while(p > buf && p[-1] != ' ')
			p--;
		if(p <= buf)
			return nil;
		p--;
	}
	p++;
	
	/* Parse the numbers */
	ntok = tokenize(p, toks, 4);
	if(ntok != 4)
		return nil;
	
	nchildren = atoi(toks[3]);
	manual = atoi(toks[2]);
	y = atoi(toks[1]);
	x = atoi(toks[0]);
	
	/* Extract text (everything between "NODE " and the numbers) */
	textlen = p - (buf + 5);
	if(textlen <= 0)
		return nil;
	
	text = buf + 5;
	text[textlen-1] = '\0';  /* Remove trailing space */
	
	/* Create node */
	node = createnode(text, parent);
	node->pos.x = x;
	node->pos.y = y;
	node->manual_pos = manual;
	
	/* Set bounds for manually positioned nodes */
	if(manual) {
		width = nodewidth(text);
		if(width < MINW)
			width = MINW;
		node->bounds = (Rectangle){
			Pt(x, y),
			Pt(x + width, y + NODEH)
		};
	}
	
	/* Load children */
	for(i = 0; i < nchildren; i++) {
		if(loadnode(fd, node) == nil)
			break;
	}
	
	return node;
}

/* Save mind map to file */
void
savemap(char *filename)
{
	int fd;
	
	if((fd = create(filename, OWRITE, 0666)) < 0)
		sysfatal("create failed: %r");
	
	savenode(fd, root);
	close(fd);
}

/* Load mind map from file */
void
loadmap(char *filename)
{
	int fd;
	Node *newroot;
	
	if((fd = open(filename, OREAD)) < 0)
		sysfatal("open failed: %r");
	
	newroot = loadnode(fd, nil);
	close(fd);
	
	if(newroot == nil)
		sysfatal("invalid file format");
	
	/* Replace existing tree */
	if(root != nil)
		deletenode(root);
	root = newroot;
	current = root;
	
	/* Update layout */
	layoutmap(root, 0);
	drawmap();
}

/* Handle file operations */
void
handlecmd(char *cmd)
{
	char *s;
	int fd;
	Node *newroot;
	
	s = cmd+1;
	while(*s == ' ' || *s == '\t')
		s++;
	
	switch(cmd[0]) {
	case 'q':  /* quit */
		exits(nil);
		break;
	case 'r':  /* read file */
		if(*s == 0)
			break;
		loadmap(s);
		break;
	case 'w':  /* write file */
		if(*s == 0)
			break;
		savemap(s);
		break;
	case '<':  /* read from command */
		if(*s == 0)
			break;
		if((fd = pipeline("%s", s)) < 0)
			sysfatal("pipeline failed: %r");
		newroot = loadnode(fd, nil);
		close(fd);
		if(newroot == nil)
			sysfatal("invalid file format");
		if(root != nil)
			deletenode(root);
		root = newroot;
		current = root;
		layoutmap(root, 0);
		drawmap();
		break;
	case '>':  /* write to command */
		if(*s == 0)
			break;
		if((fd = pipeline("%s", s)) < 0)
			sysfatal("pipeline failed: %r");
		savenode(fd, root);
		close(fd);
		break;
	}
}

/* Pipeline command execution (from paint.c) */
int
pipeline(char *fmt, ...)
{
	char buf[1024];
	va_list a;
	int p[2];
	
	va_start(a, fmt);
	vsnprint(buf, sizeof(buf), fmt, a);
	va_end(a);
	
	if(pipe(p) < 0)
		return -1;
	
	switch(rfork(RFPROC|RFMEM|RFFDG|RFNOTEG|RFREND)){
	case -1:
		close(p[0]);
		close(p[1]);
		return -1;
	case 0:
		close(p[1]);
		dup(p[0], 0);
		dup(p[0], 1);
		close(p[0]);
		execl("/bin/rc", "rc", "-c", buf, nil);
		exits("exec");
	}
	close(p[0]);
	return p[1];
}

void
main(int argc, char *argv[])
{
	Event ev;
	int e;

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();

	/* Initialize display first */
	if(initdraw(nil, nil, "mindthemap") < 0)
		sysfatal("initdraw failed: %r");
	
	/* Initialize event system */
	einit(Emouse|Ekeyboard);
	
	/* Set up display */
	setupdraw();
	
	/* Get initial window */
	if(getwindow(display, Refnone) < 0)
		sysfatal("getwindow failed: %r");
	
	/* Now create initial map */
	if(argc == 1) {
		loadmap(argv[0]);
	} else {
		initmap();
	}
	
	/* First draw after window is properly sized */
	drawmap();
	
	/* Main event loop */
	for(;;) {
		e = event(&ev);
		switch(e) {
		case Emouse:
			if(ev.mouse.buttons & 1) {  /* Left button */
				if(mode != DRAGGING && mode != CANVAS_DRAG) {
					Node *hit = findnode(root, ev.mouse.xy);
					if(hit != nil) {
						current = hit;
						mode = DRAGGING;
						/* Calculate drag offset in absolute coordinates */
						hit->drag_offset = subpt(hit->pos, addpt(ev.mouse.xy, viewport));
						drawmap();
					} else {
						/* Start canvas drag mode */
						switchmode(CANVAS_DRAG);
						panning = 1;
						pan_start = ev.mouse.xy;
						drawmap();
					}
				} else if(mode == DRAGGING) {
					updatedrag(current, ev.mouse.xy);
					drawmap();
				} else if(mode == CANVAS_DRAG) {
					/* Update viewport position */
					viewport.x += pan_start.x - ev.mouse.xy.x;
					viewport.y += pan_start.y - ev.mouse.xy.y;
					pan_start = ev.mouse.xy;
					drawmap();
				}
			} else {
				if(mode == DRAGGING) {
					mode = NORMAL;
					drawmap();
				} else if(mode == CANVAS_DRAG) {
					/* Exit canvas drag mode on mouse up if we were panning */
					if(panning) {
						panning = 0;
						switchmode(NORMAL);
						drawmap();
					}
				}
			}
			break;
		case Ekeyboard:
			handlekey(ev.kbdc, &ev);
			drawmap();
			break;
		}
	}
	
	exits(nil);  /* Normal exit */
} 