#include "mindthemap.h"

enum {
	MARGIN = 60,      /* Generous margin for better spacing */
	HSPACE = 180,     /* Increased horizontal space between nodes */
	VSPACE = 120,     /* Increased vertical space between nodes */
	MINW = 180,       /* Minimum node width */
	NODEH = 60,       /* Node height */
	PADDING = 24,     /* Text padding inside nodes */
	CORNER = 12,      /* Corner sprite size */
	CONN = 9          /* Connection point sprite size */
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
char *argv0;

/* Initialize colors and sprites */
void
initcolors(void)
{
	int i, j;
	
	back = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xFFFFFFFF);
	high = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xEAFFFFFF);
	bord = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x000055FF);
	text = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0x000000FF);
	pale = allocimage(display, Rect(0,0,1,1), screen->chan, 1, 0xAAFFFFFF);
	
	if(back == nil || high == nil || bord == nil || text == nil || pale == nil)
		sysfatal("allocimage failed");
	
	/* Create corner sprites for each color combination */
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			/* Create mask image for the corner sprite */
			corner_sprites[i][j] = allocimage(display, Rect(0,0,12,12), GREY1, 1, DTransparent);
			if(corner_sprites[i][j] == nil)
				sysfatal("allocimage failed for corner sprite");
		}
	}
	
	/* Load normal node corners (white bg, blue border) */
	for(i = 0; i < 4; i++) {
		if(loadimage(corner_sprites[SPRITE_NORMAL][i], 
			Rect(0,0,12,12), 
			corner_normal[i], 
			2*12) < 0)  /* 2 bytes per row for 12 pixels */
			sysfatal("loadimage failed for normal corner");
	}
	
	/* Load selected node corners (blue bg, white border) */
	for(i = 0; i < 4; i++) {
		if(loadimage(corner_sprites[SPRITE_SELECTED][i], 
			Rect(0,0,12,12), 
			corner_selected[i], 
			2*12) < 0)  /* 2 bytes per row for 12 pixels */
			sysfatal("loadimage failed for selected corner");
	}
	
	/* Create root node corners (copy from normal but use high color) */
	for(i = 0; i < 4; i++) {
		if(loadimage(corner_sprites[SPRITE_ROOT][i], 
			Rect(0,0,12,12), 
			corner_normal[i], 
			2*12) < 0)  /* 2 bytes per row for 12 pixels */
			sysfatal("loadimage failed for root corner");
	}
	
	/* Create alternate node corners (copy from normal but use pale color) */
	for(i = 0; i < 4; i++) {
		if(loadimage(corner_sprites[SPRITE_ALT][i], 
			Rect(0,0,12,12), 
			corner_normal[i], 
			2*12) < 0)  /* 2 bytes per row for 12 pixels */
			sysfatal("loadimage failed for alt corner");
	}
	
	/* Create connection dots */
	for(i = 0; i < 4; i++) {
		conn_dots[i] = allocimage(display, Rect(0,0,6,6), GREY1, 1, DTransparent);
		if(conn_dots[i] == nil)
			sysfatal("allocimage failed for connection dot");
		if(loadimage(conn_dots[i], Rect(0,0,6,6), conn_sprite, 2*6) < 0)  /* 2 bytes per row for 6 pixels */
			sysfatal("loadimage failed for connection dot");
	}
}

/* Calculate node width based on text */
int
nodewidth(char *text)
{
	Point p;
	p = stringsize(font, text);
	return p.x + (2 * PADDING);  /* Text width plus padding on both sides */
}

/* Draw a rounded rectangle using corner sprites */
void
roundedrect(Image *dst, Rectangle r, Image *src, Point sp, int style)
{
	Rectangle corner;
	Rectangle body;
	
	/* Draw main rectangle body (excluding corners) */
	body = r;
	body.min.x += 12;  /* Adjust for corner width */
	body.max.x -= 12;
	draw(dst, body, src, nil, sp);
	
	/* Draw top and bottom bars (between corners) */
	draw(dst, Rect(r.min.x+12, r.min.y, r.max.x-12, r.min.y+12), src, nil, sp);
	draw(dst, Rect(r.min.x+12, r.max.y-12, r.max.x-12, r.max.y), src, nil, sp);
	
	/* Draw left and right bars (between corners) */
	draw(dst, Rect(r.min.x, r.min.y+12, r.min.x+12, r.max.y-12), src, nil, sp);
	draw(dst, Rect(r.max.x-12, r.min.y+12, r.max.x, r.max.y-12), src, nil, sp);
	
	/* Draw corners using appropriate sprites as masks */
	/* Top-left */
	corner = Rect(r.min.x, r.min.y, r.min.x+12, r.min.y+12);
	draw(dst, corner, src, corner_sprites[style][CORNER_TL], ZP);
	
	/* Top-right */
	corner = Rect(r.max.x-12, r.min.y, r.max.x, r.min.y+12);
	draw(dst, corner, src, corner_sprites[style][CORNER_TR], ZP);
	
	/* Bottom-left */
	corner = Rect(r.min.x, r.max.y-12, r.min.x+12, r.max.y);
	draw(dst, corner, src, corner_sprites[style][CORNER_BL], ZP);
	
	/* Bottom-right */
	corner = Rect(r.max.x-12, r.max.y-12, r.max.x, r.max.y);
	draw(dst, corner, src, corner_sprites[style][CORNER_BR], ZP);
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
		xpos = maprect.min.x + MARGIN;
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
			node->pos.y = maprect.min.y + MARGIN + level_height[depth];
			level_height[depth] += NODEH + VSPACE;
		} else {
			/* Root node centered vertically */
			node->pos.y = (maprect.max.y - maprect.min.y) / 2;
		}
		
		/* Set bounds rectangle with padding */
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
	
	/* After all children are laid out, center the tree if needed */
	if(depth == 0 && max_width < maprect.max.x - maprect.min.x) {
		int offset = (maprect.max.x - maprect.min.x - max_width) / 2;
		/* Only center nodes that aren't manually positioned */
		center_tree_auto(root, offset);
	}
}

/* Center only automatically positioned nodes */
void
center_tree_auto(Node *node, int offset)
{
	int i;
	
	if(node == nil)
		return;
	
	if(!node->manual_pos) {
		node->pos.x += offset;
		node->bounds.min.x += offset;
		node->bounds.max.x += offset;
	}
	
	for(i = 0; i < node->nchildren; i++)
		center_tree_auto(node->children[i], offset);
}

void
drawmap(void)
{
	Rectangle winr;
	int d;
	
	if(screen == nil || display == nil)
		return;
	
	/* Get the actual window rectangle */
	winr = screen->r;
	
	/* Calculate drawing area based on window size */
	d = (Dx(winr) > Dy(winr)) ? Dy(winr) - 20 : Dx(winr) - 20;
	
	/* Clear entire window first */
	draw(screen, winr, back, nil, ZP);
	
	if(root != nil) {
		/* Set maprect to window bounds minus margins */
		maprect = insetrect(winr, MARGIN);
		
		/* Constrain drawing area */
		if(Dx(maprect) > d) {
			int excess = Dx(maprect) - d;
			maprect.min.x += excess/2;
			maprect.max.x -= excess/2;
		}
		if(Dy(maprect) > d) {
			int excess = Dy(maprect) - d;
			maprect.min.y += excess/2;
			maprect.max.y -= excess/2;
		}
		
		/* Leave room for status line */
		maprect.max.y -= font->height + 5;
		
		/* Recalculate layout before drawing */
		layoutmap(root, 0);
		
		/* Draw nodes */
		drawnode(root);
		
		/* Draw status line at bottom of window */
		Rectangle statusr = Rect(winr.min.x, winr.max.y - font->height - 5,
			winr.max.x, winr.max.y - 5);
		draw(screen, statusr, bord, nil, ZP);
		
		/* Draw mode on left side */
		string(screen, Pt(statusr.min.x + 5, statusr.min.y), back, ZP, font,
			mode == NORMAL ? "NORMAL" : "INSERT");
		
		/* Draw version on right side */
		char *version = "mindthemap 0.1";
		string(screen, Pt(statusr.max.x - stringwidth(font, version) - 5, statusr.min.y),
			back, ZP, font, version);
	}
	
	flushimage(display, 1);
}

/* Draw connection between nodes with connection point sprites */
void
drawconnection(Point from, Point to, int thickness, Image *color)
{
	Point mid;
	Rectangle dot;
	int style = (color == bord) ? SPRITE_NORMAL : SPRITE_SELECTED;
	
	/* Only draw if both points are within bounds */
	if(from.x < maprect.min.x || from.x >= maprect.max.x ||
	   from.y < maprect.min.y || from.y >= maprect.max.y ||
	   to.x < maprect.min.x || to.x >= maprect.max.x ||
	   to.y < maprect.min.y || to.y >= maprect.max.y)
		return;
	
	/* Draw connection points using appropriate sprite as mask */
	dot = Rect(from.x-3, from.y-3, from.x+3, from.y+3);
	draw(screen, dot, color, conn_dots[style], ZP);
	
	dot = Rect(to.x-3, to.y-3, to.x+3, to.y+3);
	draw(screen, dot, color, conn_dots[style], ZP);
	
	/* Calculate midpoint with increased vertical offset for gentler curve */
	mid = Pt((from.x + to.x)/2, from.y + (to.y - from.y)/3);
	
	/* Only draw lines if midpoint is also within bounds */
	if(mid.x >= maprect.min.x && mid.x < maprect.max.x &&
	   mid.y >= maprect.min.y && mid.y < maprect.max.y) {
		/* Draw connection lines */
		line(screen, from, mid, 0, 0, thickness, color, ZP);
		line(screen, mid, to, 0, 0, thickness, color, ZP);
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
	int style;
	
	if(node == nil)
		return;
	
	/* Calculate node depth */
	for(p = node->parent; p != nil; p = p->parent)
		depth++;
	
	r = node->bounds;
	
	/* Skip if completely outside maprect bounds */
	if(r.max.x <= maprect.min.x || r.min.x >= maprect.max.x ||
	   r.max.y <= maprect.min.y || r.min.y >= maprect.max.y)
		return;
	
	/* Select colors and style based on node state and depth */
	if(node == current) {
		bg = bord;
		fg = back;
		style = SPRITE_SELECTED;
	} else if(node == root) {
		bg = high;
		fg = text;
		style = SPRITE_ROOT;
	} else {
		bg = depth % 2 ? pale : back;
		fg = text;
		style = depth % 2 ? SPRITE_ALT : SPRITE_NORMAL;
	}
	
	/* Draw node with appropriate corner sprites */
	roundedrect(screen, r, bg, ZP, style);
	
	/* Draw node text if there's room */
	if(r.max.x - r.min.x > 2*PADDING) {
		txtp.x = r.min.x + PADDING;
		txtp.y = r.min.y + (NODEH - font->height) / 2;
		string(screen, txtp, fg, ZP, font, node->text);
	}
	
	/* Draw connecting lines to children first (behind nodes) */
	for(i = 0; i < node->nchildren; i++) {
		Point from, to;
		
		from.x = node->bounds.min.x + (node->bounds.max.x - node->bounds.min.x) / 2;
		from.y = node->bounds.max.y;
		
		to.x = node->children[i]->bounds.min.x + 
			(node->children[i]->bounds.max.x - node->children[i]->bounds.min.x) / 2;
		to.y = node->children[i]->bounds.min.y;
		
		drawconnection(from, to, node == root ? 2 : 1, bord);
	}
	
	/* Draw children after parent */
	for(i = 0; i < node->nchildren; i++) {
		drawnode(node->children[i]);
	}
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
	if(mode == NORMAL) {
		switch(key) {
		case 'i':  /* Enter insert mode */
			switchmode(INSERT);
			break;
		case '\t':  /* Add child to current node */
			addchild(current);
			drawmap();
			break;
		case '\n':  /* Add sibling to current node */
			addsibling(current);
			drawmap();
			break;
		case 'd':  /* Delete current node */
			if(current != root) {
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
			navigate(key);
			drawmap();
			break;
		case 'q':  /* Quit command */
		case 'r':  /* Read file */
		case 'w':  /* Write file */
		case '<':  /* Read from command */
		case '>':  /* Write to command */
			buf[0] = key;
			buf[1] = 0;
			if (eenter("Cmd", buf, sizeof(buf), &ev->mouse) > 0)
				handlecmd(buf);
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
	
	/* Check children first (reverse order for top-to-bottom hit testing) */
	for(i = node->nchildren - 1; i >= 0; i--) {
		found = findnode(node->children[i], p);
		if(found != nil)
			return found;
	}
	
	/* Check if point is within this node's bounds */
	if(ptinrect(p, node->bounds))
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
	Rectangle bounds;
	
	if(node == nil)  /* Only check for nil node */
		return;
	
	/* Calculate new position based on mouse and drag offset */
	newpos = addpt(mouse, node->drag_offset);
	
	/* Snap to grid */
	newpos = snaptoGrid(newpos);
	
	/* Calculate new bounds */
	width = nodewidth(node->text);
	if(width < MINW)
		width = MINW;
	
	bounds = (Rectangle){
		Pt(newpos.x, newpos.y),
		Pt(newpos.x + width, newpos.y + NODEH)
	};
	
	/* Keep node within maprect bounds */
	if(bounds.min.x < maprect.min.x)
		newpos.x = maprect.min.x;
	if(bounds.min.y < maprect.min.y)
		newpos.y = maprect.min.y;
	if(bounds.max.x > maprect.max.x)
		newpos.x = maprect.max.x - width;
	if(bounds.max.y > maprect.max.y)
		newpos.y = maprect.max.y - NODEH;
	
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
				if(mode != DRAGGING) {
					Node *hit = findnode(root, ev.mouse.xy);
					if(hit != nil) {  /* Remove root node check */
						current = hit;
						mode = DRAGGING;
						hit->drag_offset = subpt(hit->pos, ev.mouse.xy);
						drawmap();
					}
				} else {
					updatedrag(current, ev.mouse.xy);
					drawmap();
				}
			} else if(mode == DRAGGING) {
				mode = NORMAL;
				drawmap();
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