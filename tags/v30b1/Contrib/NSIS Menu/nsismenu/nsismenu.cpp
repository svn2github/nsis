/////////////////////////////////////////////////////////////////////////////
// NSIS MENU
//
// Reviewed for Unicode support by Jim Park -- 08/23/2007
// Basically, compiling wxWidgets as Unicode should do it.
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/event.h>
#include <wx/filefn.h>
#include <wx/image.h>
#include <wx/html/htmlwin.h>
#include <wx/html/htmlproc.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
 // override base class virtuals
 // ----------------------------

 // this one is called on application startup and is a good place for the app
 // initialization (doing it here and not in the ctor allows to have an error
 // return: if OnInit() returns false, the application terminates)
   virtual bool OnInit();
};

// Define a new frame type: this is going to be our main frame
class MyFrame : public wxFrame
{
public:
 // ctor(s)
    MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
 // event handler(s)
    void OnLink(wxHtmlLinkEvent& event);
    void OnClose(wxCloseEvent& event);

private:
     wxHtmlWindow *m_Html;

 // any class wishing to process wxWindows events must use this macro
 DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#define HTMLW 598 // release.py generates a 598x45 header image
#define HTMLH 365

// IDs for the controls and the menu commands
   enum
   {
    // controls start here (the numbers are, of course, arbitrary)
   HtmlControl = 1000
   };

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

// the event tables connect the wxWindows events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
   BEGIN_EVENT_TABLE(MyFrame, wxFrame)
     EVT_HTML_LINK_CLICKED(HtmlControl, MyFrame::OnLink)
   END_EVENT_TABLE()
   
   // Create a new application object: this macro will allow wxWindows to create
   // the application object during program execution (it's better than using a
   // static object for many reasons) and also declares the accessor function
   // wxGetApp() which will return the reference of the right type (i.e. MyApp and
   // not wxApp)
   IMPLEMENT_APP(MyApp)
   
   // ============================================================================
   // implementation
   // ============================================================================
   
   // ----------------------------------------------------------------------------
   // the application class
   // ----------------------------------------------------------------------------
   // `Main program' equivalent: the program execution "starts" here
   bool MyApp::OnInit()
   {
     wxInitAllImageHandlers();

     // Create the main application window
     MyFrame *frame = new MyFrame(_("NSIS Menu"),
         wxPoint(50, 50), wxSize(HTMLW + wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_X), HTMLH + wxSystemSettings::GetMetric(wxSYS_FRAMESIZE_Y)));
   
     // Show it and tell the application that it's our main window

     frame->SetClientSize(HTMLW, HTMLH);
     frame->Show(TRUE);
     SetTopWindow(frame);
   
     // success: wxApp::OnRun() will be called which will enter the main message
     // loop and the application will run. If we returned FALSE here, the
     // application would exit immediately.
     return TRUE;
   }

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------


// frame constructor
   MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
   : wxFrame((wxFrame *)NULL, -1, title, pos, size, wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION,
             wxT("nsis_menu"))
   {  
      m_Html = new wxHtmlWindow(this, HtmlControl);
      m_Html->SetRelatedFrame(this, wxT("%s")); // Dialog caption comes from the html title element or filename
      m_Html->SetBorders(0);
      m_Html->EnableScrolling(false, false);
      m_Html->SetSize(HTMLW, HTMLH);
      
      // Set font size
      wxSize DialogSize(1000, 1000);
      DialogSize = this->ConvertDialogToPixels(DialogSize);
      int fonts[7] = {0, 0, 14000 / (DialogSize.GetWidth()), 19000 / (DialogSize.GetWidth()), 0, 0, 0};
      m_Html->SetFonts(wxString(), wxString(), fonts);
      
      wxString exePath = wxStandardPaths::Get().GetExecutablePath();
      wxString path = ::wxPathOnly(exePath);
      m_Html->LoadPage(path + wxT("\\Menu\\index.html"));
      
      this->Centre(wxBOTH);
#ifndef __WXGTK__
      this->SetIcon(wxICON(nsisicon));
#endif
   }

// event handler

void MyFrame::OnLink(wxHtmlLinkEvent& event)
{
  const wxMouseEvent *e = event.GetLinkInfo().GetEvent();
  if (e == NULL || e->LeftUp())
  {
    const wxString href = event.GetLinkInfo().GetHref();
    if (href.Left(3).IsSameAs((const wxChar*) wxT("EX:"), false))
    {
      wxString url = href.Mid(3);
      if (url.Left(7).IsSameAs((const wxChar*) wxT("http://"), false) || url.Left(6).IsSameAs((const wxChar*) wxT("irc://"), false))
      {
        ::wxLaunchDefaultBrowser(url);
      }
      else
      {
        wxString exePath = wxStandardPaths::Get().GetExecutablePath();
        wxString path = ::wxPathOnly(exePath);
        path.Append(wxFileName::GetPathSeparators()[0]);
        path.Append(url);
        ::wxLaunchDefaultBrowser(path);
      }
    }
    else
    {
      event.Skip();
    }
  }
}

