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

int x1, y1, x2, y2;		/* input points */
int button_pressed = 0;		/* input state */

/*
 * "InputLine" event handler
 */
/* ARGSUSED */
void InputLineEH(Widget w, XtPointer client_data, XEvent *event, Boolean *cont)
{
    Pixel	fg, bg;

    if (button_pressed) {
	if (!inputGC) {
	    inputGC = XCreateGC(XtDisplay(w), XtWindow(w), 0, NULL);
	    XSetFunction(XtDisplay(w), inputGC, GXxor);
	    XSetPlaneMask(XtDisplay(w), inputGC, ~0);
	    XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg, NULL);
	    XSetForeground(XtDisplay(w), inputGC, fg ^ bg);
	}

	if (button_pressed > 1) {
	    /* erase previous position */
	    XDrawLine(XtDisplay(w), XtWindow(w), inputGC, x1, y1, x2, y2);
	} else {
	    /* remember first MotionNotify */
	    button_pressed = 2;
	}

	x2 = event->xmotion.x;
	y2 = event->xmotion.y;

	XDrawLine(XtDisplay(w), XtWindow(w), inputGC, x1, y1, x2, y2);
    }
}

/*
 * "DrawLine" callback function
 */
void DrawLineCB(Widget w, XtPointer client_data, XtPointer call_data)
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
		if (++nlines > maxlines) {
		    maxlines += LINES_ALLOC_STEP;
		    lines = (XSegment*) XtRealloc((char*)lines,
		      (Cardinal)(sizeof(XSegment) * maxlines));
		}

		lines[nlines - 1].x1 = x1;
		lines[nlines - 1].y1 = y1;
		lines[nlines - 1].x2 = d->event->xbutton.x;
		lines[nlines - 1].y2 = d->event->xbutton.y;

		button_pressed = 0;

		if (!drawGC) {
		    ac = 0;
		    XtSetArg(al[ac], XmNforeground, &v.foreground); ac++;
		    XtGetValues(w, al, ac);
		    drawGC = XCreateGC(XtDisplay(w), XtWindow(w),
			GCForeground, &v);
		}
		XDrawLine(XtDisplay(w), XtWindow(w), drawGC,
		  x1, y1, d->event->xbutton.x, d->event->xbutton.y);
	    }
	    break;
    }
}

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
    switch ((int)client_data)
    {
        case 0: /* ok */
            exit(0);
            break;
        case 1: /* cancel */
            break;
    }
}


int main(int argc, char **argv)
{
    XtAppContext app_context;
    Widget topLevel, mainWin, frame, drawArea, rowColumn, quitBtn, clearBtn, question;
    Atom wm_delete;


    char *fall[] = {
        "*question.dialogTitle: Mala otazka",
        "*question.messageString: Konec aplikace?",
        "*question.okLabelString: Ano",
        "*question.cancelLabelString: Ne",
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

/*
    XSelectInput(XtDisplay(drawArea), XtWindow(drawArea),
      KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask
      | Button1MotionMask );
*/

    rowColumn = XtVaCreateManagedWidget(
      "rowColumn",			/* widget name */
      xmRowColumnWidgetClass,		/* widget class */
      mainWin,				/* parent widget */
      XmNentryAlignment, XmALIGNMENT_CENTER,	/* alignment */
      XmNorientation, XmHORIZONTAL,	/* orientation */
      XmNpacking, XmPACK_COLUMN,	/* packing mode */
      NULL);				/* terminate varargs list */

    clearBtn = XtVaCreateManagedWidget(
      "Clear",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      rowColumn,			/* parent widget*/
      NULL);				/* terminate varargs list */

    quitBtn = XtVaCreateManagedWidget(
      "Close",				/* widget name */
      xmPushButtonWidgetClass,		/* widget class */
      rowColumn,			/* parent widget*/
      NULL);				/* terminate varargs list */

    XmMainWindowSetAreas(mainWin, NULL, rowColumn, NULL, NULL, frame);

    XtAddCallback(drawArea, XmNinputCallback, DrawLineCB, drawArea);
    XtAddEventHandler(drawArea, ButtonMotionMask, False, InputLineEH, NULL);
    XtAddCallback(drawArea, XmNexposeCallback, ExposeCB, drawArea);

    XtAddCallback(clearBtn, XmNactivateCallback, ClearCB, drawArea);

    // Added
    question = XmCreateQuestionDialog(topLevel, "question", NULL, 0);
    XtVaSetValues(question,
      XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
      NULL);
    XtUnmanageChild(XmMessageBoxGetChild(question, XmDIALOG_HELP_BUTTON));
    XtAddCallback(question, XmNokCallback, questionCB, (XtPointer)0);
    XtAddCallback(question, XmNcancelCallback, questionCB, (XtPointer)1);

    // Added
    wm_delete = XInternAtom(XtDisplay(topLevel), "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(topLevel, wm_delete, QuitCB, NULL);
    XmActivateWMProtocol(topLevel, wm_delete);
    XtAddEventHandler(topLevel, 0, True, _XEditResCheckMessages, NULL);

    // handler for quitBtn (raise question)
    XtAddCallback(quitBtn, XmNactivateCallback, QuitCB, question);
    // handler for clear button
    XtAddCallback(clearBtn, XmNactivateCallback, ClearCB, drawArea);

    XtRealizeWidget(topLevel);

    XtAppMainLoop(app_context);

    return 0;
}

// TODO
// vykreslovat body, úsečky, obdélníky a elipsy, kde obdélníky a elipsy mohou být nevyplněné nebo vyplněné s obrysem;
// u elipsy se stiskem tlačítka označí střed elipsy a tažením myší se určí její x-ový a y-ový poloměr;
// Při tažení se objekt vykresluje (může být libovolný typ čáry, barva, šířka)!
// nastavit šířku čáry s alespoň 3 různými hodnotami včetně 0;
// šířka čáry určuje i velikost bodů, proto u bodů pro šířku 0 použijte funkci XDrawPoint() a u jiných šířek funkci XFillArc();
// (Maximální šířka musi byt alespoň 8bodů - např. 0-3-8 !)
// nastavit barvu kreslené čáry (aspoň 4, ne jen black/white!); stačí barva popředí a pozadí; (použije se při šrafování!)
// nastavit barvu pro vyplňování (aspoň 4, ne jen black/white!); opět stačí barva popředí a pozadí; (použilo by se pro vyplňování vzorkem)
// zvolit mezi plnou nebo čárkovanou čarou typu LineDoubleDash (ne OnOffDash!) - nemusí se vztahovat na čáru tloušťky 0 (někde čárkovaní u tloušťky 0 nefungovalo);
// smazat nakreslený obrázek tlačítkem nebo z aplikačního menu;
// DONE: ukončit aplikaci tlačítkem nebo z aplikačního menu;
// DONE: při opouštění aplikace musí být zobrazen dialog, který se ptá, zda skutečně skončit (aplikaci lze ukončit i zvolením Close v menu nabízeném WM!).