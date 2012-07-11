#include<stdio.h>
#include<string.h>
#include "cli.h"

PANEL con, menu, home, info, search, data, *focused;
char *options[] = {"Home","Search","View All","Info","Quit"},
  *fields[] = {"Product Code", "Product Name","Product Description","Stock","Shelf Price","Wholesale Price","Manufacturer Name","Search"};
char readme[10024];
FILEARY db;
static int selected[1024] = {0}, fwidths[7];

bool show(PANEL *p){
  panel_display(p);
  focused = p;
}

bool hide(PANEL *p){
  static PANEL* panes[3] = {&con,&menu,&home};
  int i;
  for(i=0;i<sizeof(panes)/sizeof(PANEL*);i++)
    if( panes[i] != 0 && panes[i] != p ) panel_display(panes[i]);
}

char* get_data(int col,int row){
  
}

bool edit_data(PANEL* p,int x, int y){
  
}

bool prompt_data(PANEL* p,int x, int y){
  
}

bool output_data(PANEL *p){
  panel_clear(p);
  panel_write(p,"Query Results:\n");
  panel_grid(p,get_data,fwidths,7,1024,edit_data);
  show(p);
}



void show_home(){
  //Read in README.txt file for help
  int size = 0;
  FILE* fh = fopen("README.txt","r");
  panel_clear(&home);
  if( fh != 0 ){
    do{ size += fread(readme,1,1024,fh); }while( ferror(fh) != 0 );
    fclose(fh);
    readme[size] = 0;
    panel_write(&home,readme);
  }else{
    panel_write(&home,"README File not found!\n");
  }
  panel_display(&home);
}

bool console_exec(PANEL* p,char* cmd){
  panel_write(p,"\x1B[31mParser Error (0x01):\x1B[39m Function not implemented");
  return true;
}

char* get_field(int i){
  if( i>=8 ) return 0;
  return fields[i];
}

char* get_option(int i){
  if( i>=5 ) return 0;
  return options[i];
}

bool field_search(PANEL* p,int i){
  int x, y;
  char buf[1024];
  if( i != 7 ){
    panel_getcursorpos(p,&x,&y);
    panel_setcursorpos(p,x+strlen(fields[i]),y);
    panel_erase(p,-1);
    panel_focus(p);
    panel_getline(p,buf,"\n\x0D");
    panel_write(p,buf);
  }else{
    //Do search
    output_data(&data);
    return false;
  }
  return true;
}

bool menu_select(PANEL* p,int i){
  if( i == 0 ){
    show(&home);
    show_home();
  }else if( i == 1 ){
    show(&search);
  }else if( i == 2 ){
    output_data(&data);
  }else if( i == 3 ){
    show(&info);
  }if( i == 4 ){
    return false;
  }
  return true;
}

int main(void){
  int c, emode;
  panel_init(&con,0,0,-1,20);
  panel_init(&menu,0,0,20,-1);
  panel_init(&home,20,0,-1,-1);
  panel_init(&search,20,0,-1,-1);
  panel_init(&data,20,0,-1,-1);
  panel_init(&info,20,0,-1,-1);
  con.attr[PANEL_BORDER_WIDTH] = 1; con.attr[PANEL_BORDER_COLOR] = 1;
  menu.attr[PANEL_BORDER_WIDTH] = 1; menu.attr[PANEL_BORDER_COLOR] = 2;
  home.attr[PANEL_BORDER_WIDTH] = 1; home.attr[PANEL_BORDER_COLOR] = 5;
  search.attr[PANEL_BORDER_WIDTH] = 1; search.attr[PANEL_BORDER_COLOR] = 6;
  data.attr[PANEL_BORDER_WIDTH] = 1; data.attr[PANEL_BORDER_COLOR] = 7;
  info.attr[PANEL_BORDER_WIDTH] = 1; info.attr[PANEL_BORDER_COLOR] = 7;

  /*
    Specialize all the panels
  */
  panel_write(&con,"This is the \x1B[34mPDB\x1B[39m interpreter\nIt allows very fine control over the system, use with care\n");
  panel_list(&menu,get_option,0,menu_select);
  panel_list(&search,get_field,0,field_search);
  panel_console(&con,'~',console_exec);

  show(&menu); show(&home);

  /*
    Start program
  */
  show_home();
  /*
    This is the event loop, it gets keys and sends global events
  */
  emode = false;
  while(1){
    c = panel_getkey(0,true);
    if( c == 0 ){ emode = true; continue; }
    if( c == '~' ){
      panel_console_evt(&con,PANEL_FOCUS);
      hide(&con);
    }else if( emode && c == 38 ){ /* Up */
      if( focused == &search ){
	panel_list_evt(&search, PANEL_UP);
      }else if( focused == &data ){
	panel_grid_evt(&data, PANEL_UP);
      }else{
	panel_list_evt(&menu,PANEL_UP);
      }
    }else if( emode && c == 40 ){ /* Down */
      if( focused == &search ){
	panel_list_evt(&search, PANEL_DOWN);
      }else if( focused == &data ){
	panel_grid_evt(&data,PANEL_DOWN);
      }else{
	panel_list_evt(&menu,PANEL_DOWN);
      }
    }else if( (emode && c == 37) || (!emode && c==27) ){ /* Left or ESC */
      if( c==37 && focused == &data ) panel_grid_evt(&data,PANEL_LEFT);
      else if( focused != &home ) show(&home);
    }else if( (emode && c == 39) || c == 0xD ){ /* Right or enter */
      if( focused == &search ){
	panel_list_evt(&search, PANEL_SELECT);
      }else if( focused == &data ){
	if( c == 39 ) panel_grid_evt(&data, PANEL_RIGHT);
	else panel_grid_evt(&data,PANEL_SELECT);
      }else{
	if( !panel_list_evt(&menu,PANEL_SELECT) ) break;
      }
    }else if( c == 'j' ){
      //Scroll panel Down
      panel_scroll(focused,1,0);
    }else if( c == 'k' ){
      //Scroll panel Up
      panel_scroll(focused,-1,0);
    }else if( c == 'J' ){
      //Scroll panel Left
      panel_scroll(focused,0,-1);
    }else if( c == 'K' ){
      //Scroll Panel Right
      panel_scroll(focused,0,1);
    }
    emode = false;
  }
  return 0;
}
