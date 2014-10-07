// Unicode support by Jim Park -- 08/20/2007

#include "makensisw.h"
#include "update.h"

#include "jnetlib/httpget.h"

static BOOL update_initialized = FALSE;

static JNL_AsyncDNS *g_dns = NULL;

void InitializeUpdate() {
  if (update_initialized)
    return;

  update_initialized = TRUE;
  JNL::open_socketlib();
  g_dns = new JNL_AsyncDNS();
}

void FinalizeUpdate() {
  if (!update_initialized)
    return;

  delete g_dns;
  JNL::close_socketlib();
}

int getProxyInfo(char *out) {
  DWORD v=0;
  HKEY hKey;
  if (RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",0,KEY_READ,&hKey) == ERROR_SUCCESS) {
    DWORD l = 4;
    DWORD t;
    if (RegQueryValueExA(hKey,"ProxyEnable",NULL,&t,(unsigned char *)&v,&l) == ERROR_SUCCESS && t == REG_DWORD) {
      l=8192;
      if (RegQueryValueExA(hKey,"ProxyServer",NULL,&t,(unsigned char *)out,&l ) != ERROR_SUCCESS || t != REG_SZ) { 
        v=0; 
        *out=0; 
      }
    }
    else v=0;
    out[8192-1]=0;
    RegCloseKey(hKey);
  }
  return v;
}

DWORD CALLBACK UpdateThread(LPVOID v) {
  #define RSZ 30
  int len;
  char response[RSZ], *r;
  char url[300];
  BOOL error = FALSE;
  static char pbuf[8192];
  char *p=NULL;
  *response = 0;

  if (getProxyInfo(pbuf))
  {
    p=strstr(pbuf,"http=");
    if (!p) p=pbuf;
    else {
      p+=5;
    }
    char *tp=strstr(p,";");
    if (tp) *tp=0;
    char *p2=strstr(p,"=");
    if (p2) p=0; // we found the wrong proxy
  }

  InitializeUpdate();

  JNL_HTTPGet *get = new JNL_HTTPGet(g_dns,8192,(p&&p[0])?p:NULL);
  lstrcpyA(url,NSIS_UPDATE);
  lstrcatA(url,g_sdata.brandingv);

  lstrcpyA(response,"");
  get->addheader("User-Agent: MakeNSISw (jnetlib)");
  get->addheader("Accept:*/*");
  get->connect(url);
  while (1) {
    int st=get->run();
    if (st<0) { error = TRUE; break; }//error
    if (get->get_status()==2) {
      while( (len=get->bytes_available()) ) {
        char b[RSZ];
        if (len>RSZ) len=RSZ;
        if (lstrlenA(response)+len>RSZ) break;
        len=get->get_bytes(b,len);
        b[len]=0;
        lstrcatA(response,b);
      }
    }
    if (st==1) break; //closed
  }
  r = response;
  while (r&&*r) {
    if (*r=='\n') { *r = 0; break; }
    r++;
  }
  if (error) {
    char buf[1000];
    wsprintfA(buf, "There was a problem checking for an update.  Please try again later.\n\nError: %s",get->geterrorstr());
    MessageBoxA(g_sdata.hwnd,buf,"NSIS Update",MB_OK|MB_ICONINFORMATION);
  }
  else if (*response=='1'&&lstrlenA(response)>2) {
    char buf[200];
    wsprintfA(buf, "NSIS %s is now available.  Would you like to download it now?",response+2);
    if (MessageBoxA(g_sdata.hwnd,buf,"NSIS Update",MB_YESNO|MB_ICONINFORMATION)==IDYES) {
      ShellExecuteA(g_sdata.hwnd,"open",NSIS_DL_URL,NULL,NULL,SW_SHOWNORMAL);
    }
  }
  else if (*response=='2'&&lstrlenA(response)>2) {
    char buf[200];
    wsprintfA(buf,"NSIS %s is now available.  Would you like to download this preview release now?",response+2);
    if (MessageBoxA(g_sdata.hwnd,buf,"NSIS Update",MB_YESNO|MB_ICONINFORMATION)==IDYES) {
      ShellExecuteA(g_sdata.hwnd,"open",NSIS_DL_URL,NULL,NULL,SW_SHOWNORMAL);
    }
  }
  else MessageBoxA(g_sdata.hwnd,"There is no update available for NSIS at this time.","NSIS Update",MB_OK|MB_ICONINFORMATION);
  delete get;
  EnableMenuItem(g_sdata.menu,IDM_NSISUPDATE,MF_ENABLED);
  return 0;
}

void Update() {
  if (strstr(g_sdata.brandingv,"cvs"))
  {
    MessageBoxA(g_sdata.hwnd,"Cannot check for new version of nightly builds.  To update, download a new nightly build.","NSIS Update",MB_OK|MB_ICONSTOP);
    return;
  }

  EnableMenuItem(g_sdata.menu,IDM_NSISUPDATE,MF_GRAYED);
  DWORD tid;
  CloseHandle(CreateThread(NULL,0,UpdateThread,NULL,0,&tid));
}
