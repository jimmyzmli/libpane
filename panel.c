#include "cli.h"

static const char* menu_highlight = "\x1B[35m\x1B[41m";
static const char* menu_highlightrst = "\x1B[39m\x1B[49m";

bool panel_console_evt(PANEL *p, int evt){
  int k, b;
  char buf[1024];
  bool (*cb)(PANEL*,char*);
  cb = (void*)p->attr[PANEL_MGR_STORE];
  b = p->attr[PANEL_MGR_STORE+1];
  if( evt == PANEL_FOCUS ){
    panel_focus(p);
    panel_write(p,">>");
    panel_focus(p);
    while(1){
      k = panel_getline(p,buf,"\n~\x0D");
      if( buf[k-1] == b ) break;
      buf[k-1] = 0;
      panel_write(p,buf);
      panel_write(p,"\n");
      if( !cb(p,buf) ) break;
      panel_write(p,"\n\n>>");
      panel_focus(p); panel_focus(p);
    }
    panel_write(p,"\n");
  }
  return true;
}

bool panel_console(PANEL *p, char b, bool (*cb)(PANEL*,char*)){
  p->attr[PANEL_MGR_STORE] = (int)cb;
  p->attr[PANEL_MGR_STORE+1] = b;
}

bool panel_list_evt(PANEL *p, int evt){
  int top, len, x, y, emode, k;
  bool (*cb)(PANEL*,int);
  top = p->attr[PANEL_MGR_STORE]; len = p->attr[PANEL_MGR_STORE+1];
  cb = (void*)p->attr[PANEL_MGR_STORE+2];
  panel_getcursorpos(p,&x,&y);
  
  if( evt == PANEL_UP && y > top ){
    panel_writeattr(p,menu_highlightrst,20);
    panel_setcursorpos(p,x,y-1);
    panel_writeattr(p,menu_highlight,20);
  }else if( evt == PANEL_DOWN && y < top+len-1 ){
    panel_writeattr(p,menu_highlightrst,20);
    panel_setcursorpos(p,x,y+1);
    panel_writeattr(p,menu_highlight,20);
  }else if( evt == PANEL_SELECT ){
    if( cb != 0 )
      if(!cb(p,y-top)) return false;
  }
  panel_focus(p);
  return true;
}

bool panel_list(PANEL *p, char* (*items)(int), int def, bool (*cb)(PANEL*,int)){
  int i, len, k, x, y, top, bottom;
  char* txt;
  bool emode;
  panel_write(p,"\n");
  panel_getcursorpos(p,&x,&top);
  p->attr[PANEL_MGR_STORE] = top;
  p->attr[PANEL_MGR_STORE+2] = (int)cb;
  p->attr[PANEL_MGR_STORE+3] = (int)items;
  for(i=0;(txt=items(i))!=0;i++){
    panel_write(p,txt);
    panel_write(p,"\n");
  }
  p->attr[PANEL_MGR_STORE+1] = i;
  panel_setcursorpos(p,x,top+def);
  panel_writeattr(p,menu_highlight,20);
  panel_display(p);
  return true;
}

static int panel_grid_findcol(int* widths,int len,int s,int x){
  int i,t;
  t = s;
  for(i=0;i<len;i++){
    t += widths[i];
    if( x < t ) return i;
  }
  return len-1;
}

static int panel_grid_findcoloffset(int *widths,int len,int s){
  int j,t;
  t = s;
  for(j=0;j<len;j++) t += widths[j]+1;
  return t+1;
}

bool panel_grid_evt(PANEL *p, int evt){
  bool (*cb)(PANEL*,int,int);
  int topX, topY, x, y, w, h, *maxw, col,ofst;
  /* Deserialize and restore data */
  topX = p->attr[PANEL_MGR_STORE+3]; topY = p->attr[PANEL_MGR_STORE+4];
  maxw = (int*)p->attr[PANEL_MGR_STORE+2]; cb = (void*)p->attr[PANEL_MGR_STORE+1];
  w = p->attr[PANEL_MGR_STORE+5]; h = p->attr[PANEL_MGR_STORE+6];
  panel_getcursorpos(p,&x,&y);
  col = panel_grid_findcol(maxw,w,topX,x);
  ofst = panel_grid_findcoloffset(maxw,col,topX);
  if( evt == PANEL_UP ){
    panel_writeattr(p,menu_highlightrst,maxw[col]);
    panel_setcursorpos(p,ofst,y-2);
    panel_writeattr(p,menu_highlight,maxw[col]);
    panel_focus(p);
  }else if( evt == PANEL_DOWN ){
    panel_writeattr(p,menu_highlightrst,maxw[col]);
    panel_setcursorpos(p,ofst,y+2);
    panel_writeattr(p,menu_highlight,maxw[col]);
    panel_focus(p);
  }else if( evt == PANEL_LEFT && col > 0 ){
    ofst = panel_grid_findcoloffset(maxw,col-1,topX);
    panel_writeattr(p,menu_highlightrst,maxw[col]);
    panel_setcursorpos(p,ofst,y);
    panel_writeattr(p,menu_highlight,maxw[col-1]);
    panel_focus(p);
  }else if( evt == PANEL_RIGHT && col < w-1 && col >= 0 ){
    ofst = panel_grid_findcoloffset(maxw,col+1,topX);
    panel_writeattr(p,menu_highlightrst,maxw[col]);
    panel_setcursorpos(p,panel_grid_findcoloffset(maxw,col+1,topX),y);
    panel_writeattr(p,menu_highlight,maxw[col+1]);
    panel_focus(p);
  }else if( evt == PANEL_SELECT ){
    if( !cb(p,col,y) ) return false;
  }
  return true;
}

bool panel_grid(PANEL *p, char*(*item)(int,int), int* maxw, int w, int h, bool (*cb)(PANEL*,int,int)){
  int i, j, width, topX, topY;
  char linesep[1024], buf[1024], *t;
  p->attr[PANEL_MGR_STORE] = (int)item;
  p->attr[PANEL_MGR_STORE+1] = (int)cb;
  p->attr[PANEL_MGR_STORE+2] = (int)maxw;
  panel_getcursorpos(p,&topX,&topY);
  p->attr[PANEL_MGR_STORE+3] = topX; p->attr[PANEL_MGR_STORE+4] = topY;
  p->attr[PANEL_MGR_STORE+5] = w;
  p->attr[PANEL_MGR_STORE+6] = h;
  width = 0;
  for(i=0;i<w;i++) width += maxw[i];
  width += w+1;
  memset(linesep,'-',width); linesep[width] = 0;
  t = 0;
  for(i=0;i<h;i++){
    panel_write(p,linesep);
    panel_write(p,"\n");
    for(j=0;j<w;j++){
      t = item(j,i);
      if( t != 0 ){
	sprintf(buf,"|%-*.*s",maxw[j],maxw[j],t);
	panel_write(p,buf);
      }else{
	break;
      }
    }
    if( t==0 ) break;
    panel_write(p,"|\n");
  }
  panel_setcursorpos(p,topX+1,topY+1);
  panel_writeattr(p,menu_highlight,maxw[0]);
}
