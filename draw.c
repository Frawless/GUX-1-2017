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

#define STRUCT_ALLOC_STEP	2	/* memory allocation stepping */
XSegment *lines = NULL;		/* array of line descriptors */
int maxShapes = 0;		/* space allocated for max lines */
int savedShapes = 0;			/* current number of lines */

GC drawGC = 0;			/* GC used for final drawing */
GC inputGC = 0;			/* GC used for drawing current position */

int x1, y1, x2, y2, xt, yt;		/* input points */
int button_pressed = 0;		/* input state */

GC gc;
XGCValues gcv;
Widget draw;
String colours[] = {"Black", "White", "Green", "Blue", "Red"};

/* stores current colour of fill - black default */
Display *display;

/* xlib id of display */
Colormap cmap;

// Enum types for buttons
enum color {BLACK = 0, WHITE, GREEN, BLUE, RED};
enum shape {POINT = 0, LINE, RECTANGLE, CIRCLE};
enum width {WZERO = 0, WTHREE, WEIGHT};
enum type {SOLID = 0, DASHED};
enum fill {TRANSPARENT = 0, FILLED};

int lineWidth[] = {0, 3, 8};
int lineType[] = {LineSolid, LineDoubleDash};

// Struct for config of each shape
struct Config {
   int shape;
   int width;
   int type;
   long int lineFG;
   long int lineBG;
   long int shapeFG;
   long int shapeBG;
   int filled;
} Config_def = {POINT, WTHREE, SOLID, BLACK, BLACK, BLACK, BLACK, TRANSPARENT};

// Struct for save shapes for re-draw
struct ExposedData {
    int x1, x2, y1, y2;
    int filled, type, width, shape;
    long int fg, bg;
    GC gc;
};

// Global variable for expose data
struct ExposedData *drawData = NULL;

// Global variable for config shape
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

// Function for save shape into struct
void saveScreen(Widget w)
{
    if(++savedShapes > maxShapes){
        maxShapes += STRUCT_ALLOC_STEP;
        drawData = (struct ExposedData*)realloc(drawData, sizeof(struct ExposedData) * maxShapes);
        if (drawData == NULL){
            fprintf(stderr, "Can't alloc enough memory! Termination in 3...\n");
            exit(1);
        }
    }

    drawData[savedShapes-1].x1 = x1;
    drawData[savedShapes-1].x2 = x2;
    drawData[savedShapes-1].y1 = y1;
    drawData[savedShapes-1].y2 = y2;
    drawData[savedShapes-1].filled = config.filled;
    drawData[savedShapes-1].type = config.type;
    drawData[savedShapes-1].width = config.width;
    drawData[savedShapes-1].shape = config.shape;
    drawData[savedShapes-1].fg = config.shapeFG;
    drawData[savedShapes-1].bg = config.shapeBG;

    drawData[savedShapes-1].gc = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
    XSetForeground(XtDisplay(w), drawData[savedShapes-1].gc, config.lineFG);
    XSetBackground(XtDisplay(w), drawData[savedShapes-1].gc, config.lineBG);
    XCopyGC(XtDisplay(w), inputGC, GCLineWidth | GCLineStyle, drawData[savedShapes-1].gc);
}


// Main switch for shape draw
void ShapeChanger(Widget w, int width, int height, int point, Pixel bg)
{
    switch (config.shape) {
        case POINT:
            if (point){
                saveScreen(w);
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
                XSetForeground(XtDisplay(w), inputGC, bg ^ config.shapeFG);
                XFillRectangle(XtDisplay(w), XtWindow(w), inputGC, xt, yt, width, height);
                XSetForeground(XtDisplay(w), inputGC, bg ^ config.lineFG);
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
                XSetForeground(XtDisplay(w), inputGC, bg ^ config.shapeFG);
                XFillArc(XtDisplay(w), XtWindow(w), inputGC, x1 - width, y1 - height, 2 * width, 2 * height, 0, 360*64);
                XSetForeground(XtDisplay(w), inputGC, bg ^ config.lineFG);
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
    width = height = 0;

    if (button_pressed) {
      	if (!inputGC) {
      	    inputGC = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
      	    XSetFunction(XtDisplay(w), inputGC, GXxor);
      	    XSetPlaneMask(XtDisplay(w), inputGC, ~0);
      	}
        XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg, NULL);
        XSetForeground(XtDisplay(w), inputGC, bg ^ config.lineFG);
        XSetBackground(XtDisplay(w), inputGC, bg ^ config.lineBG);
        // LineDoubleDash = 2, LineSolid = 0
        XSetLineAttributes(XtDisplay(w), inputGC, lineWidth[config.width], lineType[config.type], CapRound, JoinRound);
      	if (button_pressed > 1) {
      	    /* erase previous position */
            ShapeChanger(w, width, height, 0, bg);

      	} else {
      	    /* remember first MotionNotify */
      	    button_pressed = 2;
      	}

      	x2 = event->xmotion.x;
      	y2 = event->xmotion.y;

        ShapeChanger(w, width, height, 1, bg);
    }
}

/*
 * "DrawShape" callback function
 */
void DrawShapeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
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
                saveScreen(w);
                button_pressed = 0;
                XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 0, 0, True);
            }
            break;
    }
}

/*
 * "Expose" callback function
 * Expose is function for re-draw
 */
/* ARGSUSED */
void ExposeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    XGCValues v;
    int width, height, k;
    width = height = 0;

    if (savedShapes <= 0)
        return;

    for(k = 0; k < savedShapes; k++)
    {
        XGetGCValues(XtDisplay(w), drawData[k].gc, GCForeground|GCBackground, &v);

        switch (drawData[k].shape) {
            case POINT:
              if (drawData[k].width > 0){
                  // https://tronche.com/gui/x/xlib/graphics/filling-areas/XFillArc.html
                  XFillArc(XtDisplay(w), XtWindow(w), drawData[k].gc, drawData[k].x2, drawData[k].y2, lineWidth[drawData[k].width], lineWidth[drawData[k].width], 0, 360*64);
              }
              else
                  XDrawPoint(XtDisplay(w), XtWindow(w), drawData[k].gc, drawData[k].x2, drawData[k].y2);
                break;
            case LINE:
                XDrawLine(XtDisplay(w), XtWindow(w), drawData[k].gc, drawData[k].x1, drawData[k].y1, drawData[k].x2, drawData[k].y2);
                break;
            case RECTANGLE:
                width = setWidth(drawData[k].x1, drawData[k].x2);
                height = setHeight(drawData[k].y1, drawData[k].y2);
                // Fill the shape
                if (drawData[k].filled){
                    XSetForeground(XtDisplay(w), drawData[k].gc, drawData[k].fg);
                    XFillRectangle(XtDisplay(w), XtWindow(w), drawData[k].gc, xt, yt, width, height);
                    XSetForeground(XtDisplay(w), drawData[k].gc, v.foreground);
                }
                // Draw the shape
                XDrawRectangle(XtDisplay(w), XtWindow(w), drawData[k].gc, xt, yt, width, height);
                break;
            case CIRCLE:
                width = setWidth(drawData[k].x1, drawData[k].x2);
                height = setHeight(drawData[k].y1, drawData[k].y2);
                // Fill the shape
                if (drawData[k].filled){
                    XSetForeground(XtDisplay(w), drawData[k].gc, drawData[k].fg);
                    XFillArc(XtDisplay(w), XtWindow(w), drawData[k].gc, drawData[k].x1 - width, drawData[k].y1 - height, 2 * width, 2 * height, 0, 360*64);
                    XSetForeground(XtDisplay(w), drawData[k].gc, v.foreground);
                }
                // Draw the shape
                XDrawArc(XtDisplay(w), XtWindow(w), drawData[k].gc, drawData[k].x1 - width, drawData[k].y1 - height, 2 * width, 2 * height, 0, 360*64);
                break;
        }
    }
}

/*
 * "Clear" button callback function
 */
/* ARGSUSED */
void ClearCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget wcd = (Widget) client_data;
    savedShapes = 0;
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
    int x;
    switch ((uintptr_t)client_data)
    {
        case 0: /* ok */
            // CLearing memory
            for(x = 0; x < savedShapes; x++)
                XFreeGC(XtDisplay(w), drawData[x].gc);

            XFreeGC(XtDisplay(w), inputGC);
            exit(0);
            break;
        case 1: /* cancel */
            break;
    }
}

// Buttons callbacks
void shapes_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.shape = (uintptr_t)client_data;
}

void width_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.width = (uintptr_t)client_data;
}

void type_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.type = (uintptr_t)client_data;
}

void lf_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    config.lineFG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, config.lineFG, NULL);
}

void lb_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    config.lineBG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, config.lineBG, NULL);
}

void sf_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    config.shapeFG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, config.shapeFG, NULL);
}

void sb_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XColor xcolour, spare;
    if (XAllocNamedColor(display, cmap, colours[(uintptr_t)client_data], &xcolour, &spare) == 0)
        return;			/* remember new colour */
    config.shapeBG = xcolour.pixel;

    XtVaSetValues(w, XtNforeground, config.shapeBG, NULL);
}

void fill_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    config.filled = (uintptr_t)client_data;
}

// main
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

    XColor init;

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

// #####################################LINE PROPERTIES BUTTON########################################
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
      NULL, XK_T,
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

    XAllocNamedColor(display, cmap, "Black", &init, &init);

    config.lineFG = init.pixel;
    config.lineBG = init.pixel;
    config.shapeFG = init.pixel;
    config.shapeBG = init.pixel;

// #######################COLOR BUTTONS#####################################
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
    XtVaSetValues(lineForeground, XtNforeground, config.lineFG, NULL);
    XtVaSetValues(lineBackground, XtNforeground, config.lineBG, NULL);
    XtVaSetValues(shapeForeground, XtNforeground, config.shapeFG, NULL);
    XtVaSetValues(shapeBackground, XtNforeground, config.shapeBG, NULL);

    XmMainWindowSetAreas(mainWin, menu, drawMenu, NULL, NULL, frame);

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
// DONE: vykreslovat body, úsečky, obdélníky a elipsy, kde obdélníky a elipsy mohou být nevyplněné nebo vyplněné s obrysem;
// DONE: u elipsy se stiskem tlačítka označí střed elipsy a tažením myší se určí její x-ový a y-ový poloměr;
// DONE: Při tažení se objekt vykresluje (může být libovolný typ čáry, barva, šířka)!
// DONE: nastavit šířku čáry s alespoň 3 různými hodnotami včetně 0;
// DONE: šířka čáry určuje i velikost bodů, proto u bodů pro šířku 0 použijte funkci XDrawPoint() a u jiných šířek funkci XFillArc();
// DONE: (Maximální šířka musi byt alespoň 8bodů - např. 0-3-8 !)
// DONE: nastavit barvu kreslené čáry (aspoň 4, ne jen black/white!); stačí barva popředí a pozadí; (použije se při šrafování!)
// DONE: nastavit barvu pro vyplňování (aspoň 4, ne jen black/white!); opět stačí barva popředí a pozadí; (použilo by se pro vyplňování vzorkem)
// DONE: zvolit mezi plnou nebo čárkovanou čarou typu LineDoubleDash (ne OnOffDash!) - nemusí se vztahovat na čáru tloušťky 0 (někde čárkovaní u tloušťky 0 nefungovalo);
// DONE smazat nakreslený obrázek tlačítkem nebo z aplikačního menu;
// DONE: ukončit aplikaci tlačítkem nebo z aplikačního menu;
// DONE: při opouštění aplikace musí být zobrazen dialog, který se ptá, zda skutečně skončit (aplikaci lze ukončit i zvolením Close v menu nabízeném WM!).
