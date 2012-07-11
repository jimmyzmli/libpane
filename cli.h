#ifndef CLI_H
#define CLI_H

#include<stdbool.h>

//Index into the property list
#define PANEL_BORDER_WIDTH 0x0
#define PANEL_BORDER_COLOR 0x1
#define PANEL_MGR_STORE 0x5

typedef struct _PANEL{
  void* hBuf; //Handle to whatever buffer is used
  int attr[20]; //List of attributes
  int x, y;
  int width, height;
  int hScroll, vScroll;
} PANEL;

//Basic functions
bool panel_init(PANEL *p, int x, int y, int w, int h);
bool panel_write(PANEL *p, const char* data);
bool panel_writeattr(PANEL *p, const char* attrStr, int len);
bool panel_getcursorpos(PANEL *p, int *x, int *y);
bool panel_setcursorpos(PANEL *p,int x, int y);
bool panel_display(PANEL *p);
bool panel_move(PANEL *p, int x, int y, int w, int h);
bool panel_scroll(PANEL *p, int v, int h);
bool panel_autoscroll(PANEL *p);
bool panel_hintbufsize(PANEL *p, int x, int y);
bool panel_bufmaxsize(PANEL *p);
bool panel_focus(PANEL *p);
bool panel_unfocus(PANEL *p);
bool panel_clear(PANEL *p);
bool panel_erase(PANEL *p,int len);
bool panel_destroy(PANEL *p);

//Input functions
int panel_getkey(PANEL *p, bool wait);
int panel_getline(PANEL *p, char* buf, const char* cbreaks);

//Panel managers
#define PANEL_CONSOLE_FOCUS 0x00
bool panel_console(PANEL *p, char b,bool (*cb)(PANEL*,char*));
bool panel_console_evt(PANEL *p,int type);
#define PANEL_MENU_UP 0x00
#define PANEL_MENU_DOWN 0x01
#define PANEL_MENU_SELECT 0x03
bool panel_menu(PANEL *p, char** items, int len, int def, bool (*cb)(PANEL*,int));
bool panel_menu_evt(PANEL *p,int type);
#define PANEL_GRID_UP 0x00
#define PANEL_GRID_DOWN 0x01
#define PANEL_GRID_LEFT 0x02
#define PANEL_GRID_RIGHT 0x03
#define PANEL_GRID_SELECT 0x04
bool panel_grid(PANEL *p, char* (*item)(int,int), int* maxw, int w, int h, bool (*cb)(PANEL*,int,int));
bool panel_grid_evt(PANEL *p,int type);

#endif
