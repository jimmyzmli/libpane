//Requires: WinAPI32, libc
#include<windows.h>
#include<stdio.h>
#include<string.h>
#include<stdbool.h>
//This is the interface definition
#include "cli.h"

//Here are some constants
#define ERR_WARN 0x0
#define ERR_FATAL 0x1
#define ERR_DBG 0x2
#define RESIZE_X 0x1
#define RESIZE_Y 0x2

//Default color is White on black
#define DEFAULT_FG fgcolors[7]
#define DEFAULT_BG bgcolors[0]
#define DEFAULT_ATTR (DEFAULT_FG | DEFAULT_BG)

//Defines how we build the colors Black, Red, Green, Yellow, Blue, Magenta, Cyan, White
static const int fgcolors[] = { 0, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY, FOREGROUND_BLUE,FOREGROUND_RED|FOREGROUND_INTENSITY,
				FOREGROUND_BLUE|FOREGROUND_INTENSITY, FOREGROUND_BLUE|FOREGROUND_RED|FOREGROUND_GREEN },
  bgcolors[] = { 0, BACKGROUND_RED, BACKGROUND_GREEN, FOREGROUND_RED|FOREGROUND_GREEN,BACKGROUND_BLUE,BACKGROUND_RED|BACKGROUND_INTENSITY,
		 BACKGROUND_BLUE|BACKGROUND_INTENSITY, BACKGROUND_BLUE|BACKGROUND_RED|BACKGROUND_GREEN };
static HANDLE hOut = INVALID_HANDLE_VALUE;

//These functions are for the sake of convienience
void conerror(const char* msg, int lvl){
  static const char* lvlname[] = {"warning","error","debug message"};
  printf("Console %s (0x%x): %s\n", lvlname[lvl], GetLastError(), msg);
  if( lvl == ERR_FATAL ) exit(1);
}

SMALL_RECT smrect(int a,int b,int c,int d){
  SMALL_RECT t;
  t.Left = a; t.Top = b; t.Right = c; t.Bottom = d;
  return t; //Copy
}

COORD crd(int a, int b){
  COORD t;
  t.X = a; t.Y = b;
  return t; //Copy
}

bool buf_size(HANDLE,COORD*);
/*

 */
HANDLE buf_create(){
  HANDLE hBuf;
  if( (hBuf=CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL )) == INVALID_HANDLE_VALUE )
    conerror("Failed to create buffer", ERR_DBG);
  else{
    SetConsoleTextAttribute(hBuf,DEFAULT_ATTR);
  }
  return hBuf;
}

bool buf_cursor(HANDLE hBuf, COORD c){
  BOOL r;
  if( (r=SetConsoleCursorPosition(hBuf,c))==0 ) conerror("Failed to set cursor position",ERR_DBG);
  return r;
}

bool buf_cursorvisible(HANDLE hBuf, bool b){
  CONSOLE_CURSOR_INFO info;
  GetConsoleCursorInfo( hBuf, &info );
  info.bVisible = b;
  return SetConsoleCursorInfo( hBuf, &info );
}

bool buf_cursorpos(HANDLE hBuf, COORD* c){
  CONSOLE_SCREEN_BUFFER_INFO info;
  if( !GetConsoleScreenBufferInfo(hBuf, &info) ) return false;
  *c = info.dwCursorPosition;
  return true;
}


bool buf_attr_ansi_alter(WORD* attr,int n){
  if( (n >= 30 && n <= 37) || n == 39 ){
    *attr = (*attr&(~(FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY)));
    if( n == 39 ) *attr |= DEFAULT_FG; else *attr |= fgcolors[n-30];
  }else if( (n >= 40 && n <= 47) || n == 49 ){
    *attr = (*attr&(~(BACKGROUND_RED|BACKGROUND_BLUE|BACKGROUND_GREEN|BACKGROUND_INTENSITY)));
    if( n == 49 ) *attr |= DEFAULT_BG; else *attr |= bgcolors[n-40];
  }else if( n == 0 ){
    *attr = DEFAULT_ATTR;
  }else{
    return false;
  }
  return true;
}

/*

 */
int buf_write(HANDLE hBuf, const char* s ){
  int i,d1, d2, slen;
  char c1;
  DWORD done, written;
  WORD attr, fgcolor, bgcolor;
  COORD size, pos;
  slen = strlen(s); written = 0; fgcolor = DEFAULT_FG; bgcolor = DEFAULT_BG; attr = DEFAULT_ATTR;
  buf_size(hBuf,&size);
  for(i=0;s[i]!=0;i++){
    buf_cursorpos(hBuf,&pos);
    if( pos.X+1 >= size.X || pos.Y+1 >= size.Y ){
      if( !SetConsoleScreenBufferSize( hBuf, crd(size.X*2,size.Y*2) ) ){
	conerror("Buffer is out of space, ignoring writes",ERR_DBG);
	break;
      }else{
	buf_size(hBuf,&size);
      }
    }
    //ANSI vt100 escape sequences
    //Detect CSI
    if( s[i] == 0x1B && i+2<slen && s[i+1] == '[' ){
      d1 = -1; d2 = -1; c1 = -1;
      //Scan possible formats
      if( (done=sscanf(s+i+2,"%d;%d%c", &d1, &d2, &c1)) == 3 ||
	  (done=sscanf(s+i+2,"%d%c",&d1,&c1)) == 2 || (done=sscanf(s+i+2,"%c",&c1)) == 1 ){
	//Format recongized
	//Char attribute change
	if( d1 > 0 && d2==-1 && c1 == 'm' ){
	  buf_attr_ansi_alter(&attr,d1);
	  if( SetConsoleTextAttribute(hBuf,attr) == 0 ) conerror("Failed to set attribute", ERR_WARN);
	}
	i = strchr(s+i,c1) - s;
	continue;
      }
    }
    //Output current character
    if( WriteConsole( hBuf, s+i, 1, &done, NULL ) == 0 || done == 0 )
      conerror("Failed to write character", ERR_WARN);
    else written += done;
  }
  return written;
}

/*

 */
void buf_fill(HANDLE hBuf, CHAR c, WORD attr, SMALL_RECT r){
  int i;
  DWORD written;
  for(i=r.Top;i<=r.Bottom;i++){
    FillConsoleOutputCharacter(hBuf,c,r.Right-r.Left,crd(r.Left,i),&written);
    FillConsoleOutputAttribute(hBuf,attr,r.Right-r.Left,crd(r.Left,i),&written);
  }
}

/*

 */
void buf_clear(HANDLE hBuf, SMALL_RECT r){
  buf_fill(hBuf,' ',DEFAULT_ATTR,r);
}

CHAR_INFO* buf_scrape(HANDLE hBuf, SMALL_RECT r, CHAR_INFO* buf){
  COORD szBuf = crd(r.Right-r.Left,r.Bottom-r.Top), crdBuf = {0,0};
  if( buf == 0 ) buf = (CHAR_INFO*)malloc(sizeof(CHAR_INFO)*szBuf.X*szBuf.Y);
  if( ReadConsoleOutput( hBuf, buf, szBuf, crdBuf, &r ) == 0 ){
    conerror("Failed to read buffer output", ERR_DBG );
    return 0;
  }else{
    return buf;
  }
}

/*
  Listen to a specific event, discard all events until the certain event happens.
  If wait is set to false, then only the first record (if any) is peeked at (non-blocking).
*/
INPUT_RECORD buf_getevent(HANDLE hBuf, WORD t, bool wait){
  INPUT_RECORD in;
  DWORD done;
  memset(&in,0,sizeof(INPUT_RECORD));
  in.EventType = t - 1;
  if( wait ){
    while( ReadConsoleInput( hBuf, &in, 1, &done ) ) if( in.EventType == t ) break;
  }else{
    PeekConsoleInput( hBuf, &in, 1, &done );
    if( done > 0 ) ReadConsoleInput( hBuf, &in, 1, &done );
  }
  return in;
}

/*
  Copies a region (src) from one buffer hBuf to another region (dest) of another buffer hOut. Note whatever value in hOut will be overridden.
*/
int buf_copy(HANDLE hBuf, HANDLE hOut, SMALL_RECT src, SMALL_RECT dest){
  CHAR_INFO *pData;
  if( (pData=buf_scrape(hBuf,src,0)) != 0 ){
    buf_clear(hOut,dest);
    if( WriteConsoleOutput(hOut, pData, crd(src.Right-src.Left,src.Bottom-src.Top), crd(0,0), &dest) == 0 )
      conerror("Failed to copy ConsoleScreenBuffer regions", ERR_WARN);
  }
  free(pData);
  return 1;
}

bool buf_size(HANDLE hBuf, COORD* c){
  CONSOLE_SCREEN_BUFFER_INFO info;
  if( GetConsoleScreenBufferInfo( hBuf, &info ) ){
    c->X = info.dwSize.X;
    c->Y = info.dwSize.Y;
    return true;
  }else{
    conerror("Failed to get buffer info for panel for scrolling",ERR_DBG);
    return false;
  }
}

//Here begins panel implementation using the win32 api code
//Here are some INTERNAL functions
bool panel_globalinit(){
  if( hOut == INVALID_HANDLE_VALUE ){
    hOut = buf_create();
    if( hOut != INVALID_HANDLE_VALUE ) SetConsoleActiveScreenBuffer( hOut );
  }
  panel_unfocus(0);
  return hOut != INVALID_HANDLE_VALUE;
}

bool panel_adjustbufview(PANEL *p, COORD* c ){
  if( !buf_size(p->hBuf,c) ) return false;
  if( c->X > p->width+p->hScroll ) c->X = p->width+p->hScroll;
  if( c->Y > p->height+p->vScroll ) c->Y = p->height+p->vScroll;
  if( c->X > 0 ) c->X--; if(c->Y > 0) c->Y--;
  return true;
}

bool panel_adjustbounds(PANEL *p){
  COORD size;
  if( p!=0 && (p->x < 0 | p->y < 0 | p->width < 0 | p->height < 0 ) )
    if( !buf_size(p->hBuf,&size) ){
      conerror("Failed to get console information", ERR_DBG);
      return false;
    }else{
      while( p->x < 0 ) p->x += size.X + 1;
      while( p->y < 0 ) p->y += size.Y + 1;
      while( p->width < 0 ){ p->width += size.X + 1; if( p->width > 0 ) p->width -= p->x; }
      while( p->height < 0 ){ p->height += size.Y + 1; if( p->height > 0 ) p->height -= p->y; }
    }
  return true;
}

bool panel_adjustviewtocursor(PANEL *p){
  //Adjust the size of the buffer view rect starting at (p->hScroll,p->vScroll)
  //Changes view so it shows the cursor
  COORD cBufBottom, size;
  int x, y, pad;
  pad = p->attr[PANEL_BORDER_WIDTH];
  buf_size(p->hBuf,&size);
  panel_getcursorpos(p,&x,&y);
  panel_adjustbufview(p,&cBufBottom);
  if( x < p->hScroll ){ p->hScroll = x; }
  if( y < p->vScroll ){ p->vScroll = y; }
  if( x > cBufBottom.X ){ p->hScroll += (x - cBufBottom.X) + 1 + pad; }
  if( y > cBufBottom.Y ){ p->vScroll += (y - cBufBottom.Y) + 1 + pad; }
  return true;
}

//Begin implementation
bool panel_init(PANEL *p, int x, int y, int w, int h){
  if( panel_globalinit() ){
    memset(p,0,sizeof(PANEL));
    if( (p->hBuf = buf_create()) == INVALID_HANDLE_VALUE ){
      conerror("Failed to create buffer for panel",ERR_FATAL);
    }else{
      panel_move(p,x,y,w,h);
      buf_cursorvisible(p->hBuf,false);
      return true;
    }
  }
  return false;
}

bool panel_write(PANEL *p, const char* data){
  buf_write(p->hBuf,data);
}

bool panel_writeattr(PANEL *p, const char* attrStr, int len){
  int i, c, n, curX, curY;
  DWORD done;
  WORD *attr = malloc(sizeof(WORD)*len);
  memset(attr,0,sizeof(WORD)*len);
  panel_getcursorpos(p,&curX,&curY);
  ReadConsoleOutputAttribute( p->hBuf, attr, len, crd(curX,curY), &done );
  for(c=0;c<done;c++){
    for(i=0;attrStr[i]!=0;i++)
      if( attrStr[i] == 0x1B )
	if( sscanf(attrStr+i+1,"[%dm",&n) == 1 )
	  buf_attr_ansi_alter(&attr[c],n);
  }
  WriteConsoleOutputAttribute(p->hBuf,attr,done,crd(curX,curY),&done);
  free(attr);
  return true;
}

bool panel_getcursorpos(PANEL *p, int *x,int *y){
  COORD pos;
  if( !buf_cursorpos(p->hBuf,&pos) ) return false;
  *x = pos.X; *y = pos.Y;
  return true;
}

bool panel_setcursorpos(PANEL *p, int x, int y){
  COORD size,cBufBottom;
  buf_size(p->hBuf,&size);
  //Also change the scroll so the cursor shows
  //Cur buffer view rect is (p->hScroll,p->vScroll) with size
  if( x < 0 || x > size.X || y < 0 || y > size.Y ){
    conerror("Cursor set to outbound position", ERR_WARN);
    return false;
  }
  buf_cursor(p->hBuf,crd(x,y));
  panel_adjustviewtocursor(p);
  return true;
}


bool panel_display(PANEL *p){
  int i, pad;
  WORD attr;
  COORD cBufBottom;
  //Adjust height of buffer and other bounds
  panel_adjustbufview(p,&cBufBottom);
  panel_adjustbounds(p);
  //Clear space
  //buf_clear(hOut,smrect(p->x,p->y,p->x+p->width,p->y+p->height));
  //Draw border
  pad = p->attr[PANEL_BORDER_WIDTH];
  attr = (DEFAULT_ATTR^DEFAULT_BG) | bgcolors[p->attr[PANEL_BORDER_COLOR]];
  buf_fill( hOut, ' ', attr, smrect(p->x, p->y, p->x+p->width, p->y+pad-1) );
  for(i=pad;i<=(p->height-pad);i++){
    buf_fill( hOut, ' ', attr, smrect(p->x, p->y+i, p->x+pad, p->y+i) );
    buf_fill( hOut, ' ', attr, smrect(p->x+p->width-pad, p->y+i, p->x+p->width, p->y+i) );
  }
  buf_fill( hOut, ' ', attr, smrect( p->x, p->y+p->height-pad, p->x+p->width, p->y+p->height-1) );
  //Draw text
  return buf_copy(p->hBuf,hOut,
		  smrect(p->hScroll,p->vScroll,cBufBottom.X,cBufBottom.Y),
		  smrect(p->x+pad,p->y+pad,p->x+p->width-pad-1,p->y+p->height-pad-1));
}

bool panel_move(PANEL *p, int x, int y, int w, int h){
  COORD size;
  p->x = x; p->y = y; p->width = w; p->height = h;
  panel_adjustbounds(p);
  return true;
}

bool panel_autoscroll(PANEL *p){
  panel_adjustviewtocursor(p);
}

bool panel_scroll(PANEL *p, int v, int h){
  COORD size;
  p->hScroll += h; p->vScroll += v;
  if( buf_size(p->hBuf,&size) && p->width<=size.X && p->height<=size.Y ){
    if( p->hScroll > size.X-p->width ) p->hScroll = size.X-p->width;
    if( p->vScroll > size.Y-p->height ) p->vScroll = size.Y-p->height;
  }
  if( p->hScroll < 0 ) p->hScroll = 0;
  if( p->vScroll < 0 ) p->vScroll = 0;
  return panel_display(p);
}
bool panel_hintbufsize(PANEL *p, int x, int y);
bool panel_bufmaxsize(PANEL *p);
bool panel_focus(PANEL *p){
  //Give global cursor control to panel cursor
  int curX,curY,pad;
  COORD pos;
  pad = p->attr[PANEL_BORDER_WIDTH];
  panel_getcursorpos(p,&curX,&curY);
  panel_adjustviewtocursor(p);
  buf_cursorvisible(hOut,true);
  if( !buf_cursor( hOut, crd((p->x + pad)+(curX - p->hScroll), (p->y+pad)+(curY-p->vScroll)) ) ){
    conerror("Failed to set cursor position on handle", ERR_DBG);
    return false;
  }
  return panel_display(p);
}

bool panel_unfocus(PANEL *p){
  buf_cursor(hOut,crd(0,0));
  buf_cursorvisible(hOut,false);
}

bool panel_clear(PANEL *p){
  COORD size;
  buf_size(p->hBuf,&size);
  buf_clear(p->hBuf, smrect(0,0,size.X,size.Y) );
  return panel_setcursorpos(p,0,0);
}

bool panel_erase(PANEL *p,int len){
  COORD size;
  int x, y;
  panel_getcursorpos(p,&x,&y);
  buf_size(p->hBuf,&size);
  while( len < 0 ) len = size.X + len + 1;
  len -= x;
  buf_clear(p->hBuf, smrect(x,y,x+len,y));
}

bool panel_destroy(PANEL*p){
  //Nothing to do actually
  return true;
}

int panel_getkey(PANEL *p, bool wait){
  static int ekey = 0;
  int c;
  INPUT_RECORD e;
  HANDLE hIn;
  hIn = GetStdHandle(STD_INPUT_HANDLE);
  if( ekey != 0 ){ c = ekey; ekey = 0; return c; }
  do{
    e=buf_getevent(hIn, KEY_EVENT, wait);
    if( e.EventType == KEY_EVENT ){
      if( e.Event.KeyEvent.wRepeatCount == 1 && e.Event.KeyEvent.bKeyDown  ){
	c = e.Event.KeyEvent.uChar.AsciiChar;
	if( c == 0 ) ekey = e.Event.KeyEvent.wVirtualKeyCode;
	return c;
      }
    }
  }while(wait && hIn!=INVALID_HANDLE_VALUE);

  return -1;
}

int panel_getline(PANEL *p, char* buf, const char* bchrs){
  COORD cur, size;
  char cb[2];
  int c, i, pad, m;
  bool emode;
  cb[1] = 0; i = 0;
  pad = p->attr[PANEL_BORDER_WIDTH];
  buf_cursorpos(hOut,&cur);
  buf_size(hOut,&size);
  if( size.X > p->x+p->width ) size.X = p->x + p->width;
  if( size.Y > p->y+p->height) size.Y = p->y + p->height;
  emode = false;
  while( (c=panel_getkey(NULL,true)) != -1 ){
    //Handle events
    m = 1;
    if( c == 0 ){ emode = true; continue; }
    if( (emode && c==0 && strchr(bchrs,c) != 0) || strchr(bchrs,c) != 0 ){ buf[i++] = c; break; }
    if( c == 8 ) if( i > 0 ){m = -1; c=' '; }else{ continue;}
    if( emode ){
      //Special characters
    }else{
      if( m == 1 ) buf[i] = c;
      cb[0] = c;
      i += m;
      buf_write(hOut,cb);
      if( m == 1 ? (cur.X < p->x+p->width-pad-1) : (cur.X > p->x+pad) ){
	cur.X+=m;
      }else{
	if( m == 1 ) cur.X = p->x+pad; else cur.X = p->x+p->width-pad-1;
	cur.Y+=m;
	if( cur.Y >= p->y+size.Y-pad ) break;
      }
      buf_cursor(hOut,cur);
      if( m == -1 ){ buf_write(hOut,cb); buf_cursor(hOut,cur); }
    }
    emode = false;
  }
  buf[i] = 0;
  buf_cursorvisible(hOut,false); panel_display(p);
  return i;
}

