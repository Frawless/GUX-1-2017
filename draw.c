/*
 * xdraw.c - Base for 1. project
 * Autor: Jakub Stejskal
 * Predmet: GUX 2017/2018
 */

/*
 * Standard XToolkit and OSF/Motif include files.
 */
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

/*
 * Public include files for widgets used in this file.
 */
#include <Xm/MainW.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>

#include <Xm/CascadeB.h>

#include <Xm/Protocols.h>
#include <X11/Xmu/Editres.h>
#include <Xm/MessageB.h>

/*
 * Common C library include files
 */
#include <stdio.h>
#include <stdlib.h>

/*
 * Shared variables
 */

#define LINES_ALLOC_STEP	10	/* memory allocation stepping */
XSegment *lines = NULL;		/* array of line descriptors */
int maxlines = 0;		/* space allocated for max lines */
int nlines = 0;			/* current number of lines */

GC drawGC = 0;			/* GC used for final drawing */
GC inputGC = 0;			/* GC used for drawing current position */

int x1, y1, x2, y2, xt, yt;		/* input points */
int button_pressed = 0;		/* input state */

GC gc;
XGCValues gcv;
Widget draw;
String colours[] = {"Black", "White", "Green", "Blue", "Red"};

XColor black;

long int fill_pixel = 1;
long int lineFG = 1;
long int lineBG = 1;
long int shapeFG = 1;
long int shapeBG = 1;

/* stores current colour of fill - black default */
Display *display;

/* xlib id of display */
Colormap cmap;

Pixel foreground, backroundColor;

// Enum types for buttons
enum color {BLACK = 0, WHITE, GREEN, BLUE, RED};
enum shape {POINT = 0, LINE, RECTANGLE, CIRCLE};
enum width {WZERO = 0, WTHREE, WEIGHT};
enum type {SOLID = 0, DASHED};
enum fill {TRANSPARENT = 0, FILLED};

int lineWidth[] = {0, 3, 8};
int lineType[] = {LineSolid, LineDoubleDash};

// Struc for config
struct Config {
   int shape;
   int width;
   int type;
   int lineForeground;
   int lineBackground;
   int shapeForeground;
   int shapeBackground;
   int filled;
} Config_def = {POINT, WTHREE, SOLID, BLACK, BLACK, BLACK, BLACK};

struct Config config;

// Set width
int setWidth(int w1, int w2)
{
    if (w1 < w2) {
        xt = w1;
        return w2 - w1;
    } else {
        xt = w2;
        return  w1 - w2;
    }
}

// Set width
int setHeight(int h1, int h2)
{
    if (h1 < h2) {
        yt = h1;
        return h2 - h1;
    } else {
        yt = h2;
        return h1 - h2;
    }
}


// main switch for shape draw
void ShapeChanger(Widget w, int width, int height, int point)
{
    switch (config.shape) {
        case POINT:
            if (point){
                if (config.width > 0){
                    // https://tronche.com/gui/x/xlib/graphics/filling-areas/XFillArc.html
                    XFillArc(XtDisplay(w), XtWindow(w), inputGC, x2, y2, lineWidth[config.width], lineWidth[config.width], 0, 360*64);
                }
                else
                    XDrawPoint(XtDisplay(w), XtWindow(w), inputGC, x2, y2);
            }
            break;
        case LINE:

            XDrawLine(XtDisplay(w), XtWindow(w), inputGC, x1, y1, x2, y2);
            break;
        case RECTANGLE:
            width = setWidth(x1, x2);
            height = setHeight(y1, y2);
            // Fill the shape
            if (config.filled){
                XFillRectangle(XtDisplay(w), XtWindow(w), inputGC, xt, yt, width, height);
            }
            // Draw the shape
            XDrawRectangle(XtDisplay(w), XtWindow(w), inputGC, xt, yt, width, height);
            break;
        case CIRCLE:
            // Cursor will set middle of the object, drag by x/y axis will set width and height of object
            // count widht(setWidth) and height(setHeight)
            // Start for draw will be middle point - width/height
            // End will be 2x height/width
            width = setWidth(x1, x2);
            height = setHeight(y1, y2);
            // Fill the shape
            if (config.filled){
                XFillArc(XtDisplay(w), XtWindow(w), inputGC, x1 - width, y1 - height, 2 * width, 2 * height, 0, 360*64);
            }
            // Draw the shape
            XDrawArc(XtDisplay(w), XtWindow(w), inputGC, x1 - width, y1 - height, 2 * width, 2 * height, 0, 360*64);
            break;
    }
}


/*
 * "InputLine" event handler
 */
/* ARGSUSED */
void InputShapeEH(Widget w, XtPointer client_data, XEvent *event, Boolean *cont)
{
    Pixel	fg, bg;
    int width, height;

    if (button_pressed) {
      	if (!inputGC) {
      	    inputGC = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
      	    XSetFunction(XtDisplay(w), inputGC, GXxor);
      	    XSetPlaneMask(XtDisplay(w), inputGC, ~0);
      	    XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg, NULL);
      	    XSetForeground(XtDisplay(w), inputGC, fg ^ bg);
      	}

        XSetForeground(XtDisplay(w), inputGC, bg ^ lineFG);
        XSetBackground(XtDisplay(w), inputGC, bg ^ lineBG);

      	if (button_pressed > 1) {
      	    /* erase previous position */
            // XSetLineAttributes(XtDisplay(w), inputGC, lineWidth[config.width], LineOnOffDash, CapRound, JoinRound);
            ShapeChanger(w, width, height, 0);

      	} else {
      	    /* remember first MotionNotify */
      	    button_pressed = 2;
      	}

      	x2 = event->xmotion.x;
      	y2 = event->xmotion.y;

        // LineDoubleDash = 2, LineSolid = 0
        XSetLineAttributes(XtDisplay(w), inputGC, lineWidth[config.width], lineType[config.type], CapRound, JoinRound);

        ShapeChanger(w, width, height, 1);
    }
}

/*
 * "DrawLine" callback function
 */
void DrawShapeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg al[4];
    int ac;
    XGCValues v;
    XmDrawingAreaCallbackStruct *d = (XmDrawingAreaCallbackStruct*) call_data;

    switch (d->event->type) {
      	case ButtonPress:
      	    if (d->event->xbutton.button == Button1) {
            		button_pressed = 1;
            		x1 = d->event->xbutton.x;
            		y1 = d->event->xbutton.y;
      	    }
      	    break;

      	case ButtonRelease:
      	     if (d->event->xbutton.button == Button1) {
            // 		if (++nlines > maxlines) {
            // 		    maxlines += LINES_ALLOC_STEP;
            // 		    lines = (XSegment*) XtRealloc((char*)lines,
            // 		      (Cardinal)(sizeof(XSegment) * maxlines));
            // 		}
            //
            // 		lines[nlines - 1].x1 = x1;
            // 		lines[nlines - 1].y1 = y1;
            // 		lines[nlines - 1].x2 = d->event->xbutton.x;
            // 		lines[nlines - 1].y2 = d->event->xbutton.y;
            //
            		button_pressed = 0;
            //
            // 		if (!drawGC) {
            // 		    ac = 0;
            // 		    XtSetArg(al[ac], XmNforeground, &v.foreground); ac++;
            // 		    XtGetValues(w, al, ac);
            // 		    drawGC = XCreateGC(XtDisplay(w), XtWindow(w),
            // 			GCForeground, &v);
            // 		}
            // 		XDrawLine(XtDisplay(w), XtWindow(w), drawGC,
            // 		  x1, y1, d->event->xbutton.x, d->event->xbutton.y);
      	     }
      	    break;
    }
}

// Expose is function for re-draw
/*
 * "Expose" callback function
 */
/* ARGSUSED */
void ExposeCB(Widget w, XtPointer client_data, XtPointer call_data)
{

    if (nlines <= 0)
	return;
    if (!drawGC)
	drawGC = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
    XDrawSegments(XtDisplay(w), XtWindow(w), drawGC, lines, nlines);
}

/*
 * "Clear" button callback function
 */
/* ARGSUSED */
void ClearCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget wcd = (Widget) client_data;

    nlines = 0;
    XClearWindow(XtDisplay(wcd), XtWindow(wcd));
}

/*
 * "Quit" button callback function
 */
/* ARGSUSED */
void QuitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtManageChild(client_data);
}

/*
* Quit window handler
*/
void questionCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    switch ((uintptr_t)client_data)
    {
        case 0: /* ok */
            exit(0);
            break;
        case 1: /* cancel */
            break;
    }
}

void shapes_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.shape = (uintptr_t)client_data;

    fprintf(stderr, "Shape: %d\n",   config.shape); // Tady je ulozene cislo buttonu
}

void width_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.width = (uintptr_t)client_data;
    fprintf(stderr, "Width: %d\n",   config.width); // Tady je ulozene cislo buttonu
}

void type_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.type = (uintptr_t)client_data;
    fprintf(stderr, "Type: %d\n",   config.type); // Tady je ulozene cislo buttonu
}

void lf_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.lineForeground = (uintptr_t)client_data;
    fprintf(stderr, "lf_callback: %d\n",   config.lineForeground); // Tady je ulozene cislo buttonu
    fprintf(stderr, "COLOR: %s\n", colours[(uintptr_t)client_data]);

    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    lineFG = xcolour.pixel;
    fprintf(stderr, "saved\n");

    XtVaSetValues(w, XtNforeground, lineFG, NULL);
}

void lb_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.lineBackground= (uintptr_t)client_data;
    fprintf(stderr, "lb_callback: %d\n",   config.lineBackground); // Tady je ulozene cislo buttonu

    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    lineBG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, lineBG, NULL);
}

void sf_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.shapeForeground = (uintptr_t)client_data;
    fprintf(stderr, "sf_callback: %d\n",   config.shapeForeground); // Tady je ulozene cislo buttonu
    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    shapeFG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, shapeFG, NULL);
}

void sb_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.shapeBackground= (uintptr_t)client_data;
    fprintf(stderr, "sb_callback: %d\n",   config.shapeBackground); // Tady je ulozene cislo buttonu

    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    shapeBG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, shapeBG, NULL);
}

void fill_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.filled = (uintptr_t)client_data;
    fprintf(stderr, "fill_callback: %d\n",   config.filled); // Tady je ulozene cislo buttonu
}

void color_buttons(Widget w, Widget parent, XmString color, String name, void (callback)(Widget, XtPointer, XtPointer))
{
    XmString black, green, red, blue, white;
    // Shapes
    black = XmStringCreateLocalized("Black");
    white = XmStringCreateLocalized("White");
    green = XmStringCreateLocalized("Green");
    blue = XmStringCreateLocalized("Blue");
    red = XmStringCreateLocalized("Red");

    w = XmVaCreateSimpleOptionMenu(
      parent, name,
      color, XK_C,
      0,
      callback,
      XmVaPUSHBUTTON, black, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, white, XK_W, NULL, NULL,
      XmVaPUSHBUTTON, green, XK_G, NULL, NULL,
      XmVaPUSHBUTTON, blue, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, red, XK_R, NULL, NULL,
      NULL);

    XtManageChild(w);

    XmStringFree(black);
    XmStringFree(white);
    XmStringFree(green);
    XmStringFree(blue);
    XmStringFree(red);
}


int main(int argc, char **argv)
{
    XtAppContext app_context;
    Widget topLevel,  mainWin, frame, drawArea, menu,
                      drawMenu, colorMenu, shapesMenu,
                      quitBtn, clearBtn, lineSize, chooseShape, lineType,
                      lineForeground, lineBackground,
                      shapeForeground, shapeBackground,
                      question,
                      shapeFill;
    Atom wm_delete;
    XmString  color, black, green, red, blue, white,
              shape, spoint, sline, rectangle, scircle,
              widthLine, wzero, wthree, weight,
              solid, dashed,
              filled, transparent;

    // lineFG = black->pixel;
    // lineBG = black.pixel;
    // shapeFG = black.pixel;
    // shapeBG = black.pixel;


    char *fall[] = {
        "*question.dialogTitle: Quit question",
        "*question.messageString: Are you sure?",
        "*question.okLabelString: Yes",
        "*question.cancelLabelString: No",
        "*question.messageAlignment: XmALIGNMENT_CENTER",
        NULL
    };

    /*
     * Register the default language procedure
     */
    XtSetLanguageProc(NULL, (XtLanguageProc)NULL, NULL);

    topLevel = XtVaAppInitialize(
      &app_context,		 	/* Application context */
      "Draw",				/* Application class */
      NULL, 0,				/* command line option list */
      &argc, argv,			/* command line args */
      fall,
      XmNheight, (int)500,
      XmNwidth, (int)500,
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);

    mainWin = XtVaCreateManagedWidget(
      "mainWin",			/* widget name */
      xmMainWindowWidgetClass,		/* widget class */
      topLevel,				/* parent widget*/
      XmNcommandWindowLocation, XmCOMMAND_BELOW_WORKSPACE,
      NULL);				/* terminate varargs list */

    // Draw area
    frame = XtVaCreateManagedWidget(
      "frame",				/* widget name */
      xmFrameWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      NULL);				/* terminate varargs list */

    drawArea = XtVaCreateManagedWidget(
      "drawingArea",			/* widget name */
      xmDrawingAreaWidgetClass,		/* widget class */
      frame,				/* parent widget*/
      XmNwidth, 200,			/* set startup width */
      XmNheight, 100,			/* set startup height */
      NULL);				/* terminate varargs list */

    // display = XtDisplay(top);
    // screen = XtScreen(top);
    // gc = XCreateGC(display, RootWindowOfScreen(screen), 0, NULL);
    // XSetForeground(display, gc, fill_pixel);

/*
    XSelectInput(XtDisplay(drawArea), XtWindow(drawArea),
      KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
      | Button1MotionMask );
*/
// #############################################################################
    // App menu options
    menu = XtVaCreateManagedWidget(
      "menu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_COLUMN,	/* packing mode */
      NULL);				/* terminate varargs list */

    clearBtn = XtVaCreateManagedWidget(
      "Clear",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      menu,			/* parent widget*/
      NULL);				/* terminate varargs list */

    quitBtn = XtVaCreateManagedWidget(
      "Close",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      menu,			/* parent widget*/
      NULL);				/* terminate varargs list */


// #############################################################################
    // Draw menu options
    drawMenu = XtVaCreateManagedWidget(
      "drawMenu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_TIGHT,	/* packing mode */
  		// XmNscrolledWindowChildType, XmCOMMAND_WINDOW,
  		NULL);			/* terminate varargs list */

    // Draw menu options
    shapesMenu = XtVaCreateManagedWidget(
      "shapesMenu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      drawMenu,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_TIGHT,	/* packing mode */
  		// XmNscrolledWindowChildType, XmCOMMAND_WINDOW,
  		NULL);			/* terminate varargs list */

    // Draw menu options
    colorMenu = XtVaCreateManagedWidget(
      "colorMenu",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      drawMenu,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_TIGHT,	/* packing mode */
  		// XmNscrolledWindowChildType, XmCOMMAND_WINDOW,
  		NULL);			/* terminate varargs list */

    // Shapes
    shape = XmStringCreateLocalized("Shape:");
    spoint = XmStringCreateLocalized("Point");
    sline = XmStringCreateLocalized("Line");
    rectangle = XmStringCreateLocalized("Rectangle");
    scircle = XmStringCreateLocalized("Circle");

    chooseShape = XmVaCreateSimpleOptionMenu(
      shapesMenu, "chooseShape",
      shape, XK_S,
      0,
      shapes_callback,
      XmVaPUSHBUTTON, spoint, XK_P, NULL, NULL,
      XmVaPUSHBUTTON, sline, XK_L, NULL, NULL,
      XmVaPUSHBUTTON, rectangle, XK_S, NULL, NULL,
      XmVaPUSHBUTTON, scircle, XK_C, NULL, NULL,
      NULL);

    XtManageChild(chooseShape);

    XmStringFree(shape);
    XmStringFree(sline);
    XmStringFree(rectangle);
    XmStringFree(scircle);
    XmStringFree(spoint);

// #############################################################################
    widthLine = XmStringCreateLocalized("Width:");
    wzero = XmStringCreateLocalized(" 0 pt");
    wthree = XmStringCreateLocalized(" 3 pt");
    weight = XmStringCreateLocalized(" 8 pt");

    lineSize = XmVaCreateSimpleOptionMenu(
      shapesMenu, "lineSize",
      widthLine, XK_W,
      0,
      width_callback,
      XmVaPUSHBUTTON, wzero, XK_Z, NULL, NULL,
      XmVaPUSHBUTTON, wthree, XK_T, NULL, NULL,
      XmVaPUSHBUTTON, weight, XK_E, NULL, NULL,
      NULL);

    XtManageChild(lineSize);

    XmStringFree(widthLine);
    XmStringFree(wzero);
    XmStringFree(wthree);
    XmStringFree(weight);

    // Line type
    solid = XmStringCreateLocalized("solid");
    dashed = XmStringCreateLocalized("dashed");

    lineType = XmVaCreateSimpleOptionMenu(
      shapesMenu, "lineSize",
      NULL, XK_T,
      0,
      type_callback,
      XmVaPUSHBUTTON, solid, XK_S, NULL, NULL,
      XmVaPUSHBUTTON, dashed, XK_D, NULL, NULL,
      NULL);

    XtManageChild(lineType);

    XmStringFree(solid);
    XmStringFree(dashed);

    // Line width
    filled = XmStringCreateLocalized("filled");
    transparent = XmStringCreateLocalized("transparent");

    shapeFill = XmVaCreateSimpleOptionMenu(
      shapesMenu, "shapeFill",
      NULL, NULL,
      0,
      fill_callback,
      XmVaPUSHBUTTON, transparent, XK_T, NULL, NULL,
      XmVaPUSHBUTTON, filled, XK_F, NULL, NULL,
      NULL);

    XtManageChild(shapeFill);

    XmStringFree(filled);
    XmStringFree(transparent);

    cmap = DefaultColormapOfScreen(XtScreen(drawArea));
    display = XtDisplay(drawArea);

// #############################################################################
    color = XmStringCreateLocalized("LF:");

    // Shapes
    black = XmStringCreateLocalized("Black");
    white = XmStringCreateLocalized("White");
    green = XmStringCreateLocalized("Green");
    blue = XmStringCreateLocalized("Blue");
    red = XmStringCreateLocalized("Red");

    lineForeground = XmVaCreateSimpleOptionMenu(
      colorMenu, "lineForeground",
      color, XK_C,
      0,
      lf_callback,
      XmVaPUSHBUTTON, black, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, white, XK_W, NULL, NULL,
      XmVaPUSHBUTTON, green, XK_G, NULL, NULL,
      XmVaPUSHBUTTON, blue, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, red, XK_R, NULL, NULL,
      NULL);

    XtManageChild(lineForeground);

    color = XmStringCreateLocalized("LB:");

    lineBackground = XmVaCreateSimpleOptionMenu(
      colorMenu, "lineBackground",
      color, XK_C,
      0,
      lb_callback,
      XmVaPUSHBUTTON, black, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, white, XK_W, NULL, NULL,
      XmVaPUSHBUTTON, green, XK_G, NULL, NULL,
      XmVaPUSHBUTTON, blue, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, red, XK_R, NULL, NULL,
      NULL);

    XtManageChild(lineBackground);

    color = XmStringCreateLocalized("SF:");

    shapeForeground = XmVaCreateSimpleOptionMenu(
      colorMenu, "shapeForeground",
      color, XK_C,
      0,
      sf_callback,
      XmVaPUSHBUTTON, black, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, white, XK_W, NULL, NULL,
      XmVaPUSHBUTTON, green, XK_G, NULL, NULL,
      XmVaPUSHBUTTON, blue, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, red, XK_R, NULL, NULL,
      NULL);

    XtManageChild(shapeForeground);

    color = XmStringCreateLocalized("SB:");

    shapeBackground = XmVaCreateSimpleOptionMenu(
      colorMenu, "shapeBackground",
      color, XK_C,
      0,
      sb_callback,
      XmVaPUSHBUTTON, black, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, white, XK_W, NULL, NULL,
      XmVaPUSHBUTTON, green, XK_G, NULL, NULL,
      XmVaPUSHBUTTON, blue, XK_B, NULL, NULL,
      XmVaPUSHBUTTON, red, XK_R, NULL, NULL,
      NULL);

    XtManageChild(shapeBackground);

    XmStringFree(black);
    XmStringFree(white);
    XmStringFree(green);
    XmStringFree(blue);
    XmStringFree(red);
    XmStringFree(color);


// #############################################################################

    XmMainWindowSetAreas(mainWin, menu, drawMenu, NULL, NULL, frame);
    // Get backgorund color
    XtVaGetValues(drawArea, XmNbackground, &backroundColor, NULL);
    // XtVaSetValues(mainWin, XmNmenuBar, drawMenu, XmNworkWindow, frame, NULL);

    fprintf(stderr, "%ld\n", lineFG);

    // Color of TEXT
    XtVaSetValues(lineForeground, XtNforeground, lineFG, NULL);
    XtVaSetValues(lineBackground, XtNforeground, lineBG, NULL);
    XtVaSetValues(shapeForeground, XtNforeground, shapeFG, NULL);
    XtVaSetValues(shapeBackground, XtNforeground, shapeBG, NULL);

    XtAddCallback(lineForeground, XmNactivateCallback, lf_callback, lineForeground);
    XtAddCallback(lineBackground, XmNactivateCallback, lf_callback, lineBackground);
    XtAddCallback(shapeForeground, XmNactivateCallback, lf_callback, shapeForeground);
    XtAddCallback(shapeBackground, XmNactivateCallback, lf_callback, shapeBackground);


    XtAddCallback(drawArea, XmNinputCallback, DrawShapeCB, drawArea);
    XtAddEventHandler(drawArea, ButtonMotionMask, False, InputShapeEH, NULL);
    XtAddCallback(drawArea, XmNexposeCallback, ExposeCB, drawArea);

    // Added
    question = XmCreateQuestionDialog(topLevel, "question", NULL, 0);
    XtVaSetValues(question,
      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
      NULL);
    XtUnmanageChild(XmMessageBoxGetChild(question, XmDIALOG_HELP_BUTTON));
    XtAddCallback(question, XmNokCallback, questionCB, (XtPointer)0);
    XtAddCallback(question, XmNcancelCallback, questionCB, (XtPointer)1);


    // handler for quitBtn (raise question)
    XtAddCallback(quitBtn, XmNactivateCallback, QuitCB, question);
    // handler for clear button
    XtAddCallback(clearBtn, XmNactivateCallback, ClearCB, drawArea);

    XtRealizeWidget(topLevel);
    // Added
    wm_delete = XInternAtom(XtDisplay(topLevel), "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(topLevel, wm_delete, QuitCB, question);
    XmActivateWMProtocol(topLevel, wm_delete);
    XtAddEventHandler(topLevel, 0, True, _XEditResCheckMessages, NULL);

    XtAppMainLoop(app_context);

    return 0;
}

// TODO
// vykreslovat body, úsečky, obdélníky a elipsy, kde obdélníky a elipsy mohou být nevyplněné nebo vyplněné s obrysem;
// u elipsy se stiskem tlačítka označí střed elipsy a tažením myší se určí její x-ový a y-ový poloměr;
// Při tažení se objekt vykresluje (může být libovolný typ čáry, barva, šířka)!
// DONE: nastavit šířku čáry s alespoň 3 různými hodnotami včetně 0;
// DONE: šířka čáry určuje i velikost bodů, proto u bodů pro šířku 0 použijte funkci XDrawPoint() a u jiných šířek funkci XFillArc();
// DONE: (Maximální šířka musi byt alespoň 8bodů - např. 0-3-8 !)
// nastavit barvu kreslené čáry (aspoň 4, ne jen black/white!); stačí barva popředí a pozadí; (použije se při šrafování!)
// nastavit barvu pro vyplňování (aspoň 4, ne jen black/white!); opět stačí barva popředí a pozadí; (použilo by se pro vyplňování vzorkem)
// DONE: zvolit mezi plnou nebo čárkovanou čarou typu LineDoubleDash (ne OnOffDash!) - nemusí se vztahovat na čáru tloušťky 0 (někde čárkovaní u tloušťky 0 nefungovalo);
// DONE smazat nakreslený obrázek tlačítkem nebo z aplikačního menu;
// DONE: ukončit aplikaci tlačítkem nebo z aplikačního menu;
// DONE: při opouštění aplikace musí být zobrazen dialog, který se ptá, zda skutečně skončit (aplikaci lze ukončit i zvolením Close v menu nabízeném WM!).
