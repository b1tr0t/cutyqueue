////////////////////////////////////////////////////////////////////
//
// CutyCapt - A Qt WebKit Web Page Rendering Capture Utility
//
// Copyright (C) 2003-2008 Bjoern Hoehrmann <bjoern@hoehrmann.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id: CutyCapt.cpp 2 2008-06-11 06:01:28Z hoehrmann $
//
////////////////////////////////////////////////////////////////////

#include <QApplication>
#include <QtWebKit>
#include <QtGui>
#include <QSvgGenerator>
#include <QPrinter>
#include <QTimer>
#include <QByteArray>
#include <QNetworkRequest>
#include "CutyCapt.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <libmemcached/memcached.h>



using namespace std;

#ifdef STATIC_PLUGINS
  Q_IMPORT_PLUGIN(qjpeg)
  Q_IMPORT_PLUGIN(qgif)
  Q_IMPORT_PLUGIN(qtiff)
  Q_IMPORT_PLUGIN(qsvg)
  Q_IMPORT_PLUGIN(qmng)
  Q_IMPORT_PLUGIN(qico)
#endif

static struct _CutyExtMap {
  CutyCapt::OutputFormat id;
  const char* extension;
  const char* identifier;
} const CutyExtMap[] = {
  { CutyCapt::SvgFormat,         ".svg",        "svg"   },
  { CutyCapt::PdfFormat,         ".pdf",        "pdf"   },
  { CutyCapt::PsFormat,          ".ps",         "ps"    },
  { CutyCapt::InnerTextFormat,   ".txt",        "itext" },
  { CutyCapt::HtmlFormat,        ".html",       "html"  },
  { CutyCapt::RenderTreeFormat,  ".rtree",      "rtree" },
  { CutyCapt::JpegFormat,        ".jpeg",       "jpeg"  },
  { CutyCapt::PngFormat,         ".png",        "png"   },
  { CutyCapt::MngFormat,         ".mng",        "mng"   },
  { CutyCapt::TiffFormat,        ".tiff",       "tiff"  },
  { CutyCapt::GifFormat,         ".gif",        "gif"   },
  { CutyCapt::BmpFormat,         ".bmp",        "bmp"   },
  { CutyCapt::PpmFormat,         ".ppm",        "ppm"   },
  { CutyCapt::XbmFormat,         ".xbm",        "xbm"   },
  { CutyCapt::XpmFormat,         ".xpm",        "xpm"   },
  { CutyCapt::OtherFormat,       "",            ""      }
};


QString
CutyPage::chooseFile(QWebFrame* /*frame*/, const QString& /*suggestedFile*/) {
  return QString::null;
}

bool
CutyPage::javaScriptConfirm(QWebFrame* /*frame*/, const QString& /*msg*/) {
  return true;
}

bool
CutyPage::javaScriptPrompt(QWebFrame* /*frame*/,
                           const QString& /*msg*/,
                           const QString& /*defaultValue*/,
                           QString* /*result*/) {
  return true;
}

void
CutyPage::javaScriptConsoleMessage(const QString& /*message*/,
                                   int /*lineNumber*/,
                                   const QString& /*sourceID*/) {
  // noop
}

void
CutyPage::javaScriptAlert(QWebFrame* /*frame*/, const QString& /*msg*/) {
  // noop
}



QString
CutyPage::userAgentForUrl(const QUrl& url) const {

  if (!mUserAgent.isNull())
    return mUserAgent;

  return QWebPage::userAgentForUrl(url);
}

void
CutyPage::setUserAgent(const QString& userAgent) {
  mUserAgent = userAgent;
}

void
CutyPage::setAttribute(QWebSettings::WebAttribute option,
                       const QString& value) {

  if (value == "on")
    settings()->setAttribute(option, true);
  else if (value == "off")
    settings()->setAttribute(option, false);
  else
    (void)0; // TODO: ...
}

// TODO: Consider merging some of main() and CutyCap

CutyCapt::CutyCapt(CutyPage* page,int delay, OutputFormat format) {
  mPage = page;
  mDelay = delay;
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
  mFormat = format;
}

void
CutyCapt::InitialLayoutCompleted() {
  mSawInitialLayout = true;

  if (mSawInitialLayout && mSawDocumentComplete)
    TryDelayedRender();
}

void
CutyCapt::DocumentComplete(bool /*ok*/) {
  mSawDocumentComplete = true;

  if (mSawInitialLayout && mSawDocumentComplete)
    TryDelayedRender();
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
}

void
CutyCapt::TryDelayedRender() {

  if (mDelay > 0) {
    // cout << "Scheduling delayed render for " << mDelay << " seconds" << endl;
    QTimer::singleShot(mDelay, this, SLOT(Delayed()));
    return;
  }

  saveSnapshot();
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
  QApplication::exit();
}

void
CutyCapt::Timeout() {
    // cout << "!! TIMEOUT CALLED";
  saveSnapshot();
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
  QApplication::exit();
}

void
CutyCapt::Delayed() {
  saveSnapshot();  
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
  QApplication::exit();
}

void
CutyCapt::saveSnapshot() {
  QWebFrame *mainFrame = mPage->mainFrame();
  QPainter painter;
  const char* format = NULL;

  for (int ix = 0; CutyExtMap[ix].id != OtherFormat; ++ix)
    if (CutyExtMap[ix].id == mFormat)
      format = CutyExtMap[ix].identifier; //, break;

  // TODO: sometimes contents/viewport can have size 0x0
  // in which case saving them will fail. This is likely
  // the result of the method being called too early. So
  // far I've been unable to find a workaround, except
  // using --delay with some substantial wait time. I've
  // tried to resize multiple time, make a fake render,
  // check for other events... This is primarily a problem
  // under my Ubuntu virtual machine.

  // mPage->setViewportSize( mainFrame->contentsSize() );

  // cout << "-- preparing to save url: '" << mainFrame->url().toString().toStdString() << "'" << endl;

  switch (mFormat) {
    case SvgFormat: {
      QSvgGenerator svg;
      svg.setFileName(mOutput);
      svg.setSize(mPage->viewportSize());
      painter.begin(&svg);
      mainFrame->render(&painter);
      painter.end();
      break;
    }
    case PdfFormat:
    case PsFormat: {
      QPrinter printer;
      printer.setPageSize(QPrinter::A4);
      printer.setOutputFileName(mOutput);
      // TODO: change quality here?
      mainFrame->print(&printer);
      break;
    }
    case RenderTreeFormat:
    case InnerTextFormat:
    case HtmlFormat: {
      QFile file(mOutput);
      file.open(QIODevice::WriteOnly | QIODevice::Text);
      QTextStream s(&file);
      s.setCodec("utf-8");
      s << (mFormat == RenderTreeFormat ? mainFrame->renderTreeDump() :
            mFormat == InnerTextFormat  ? mainFrame->toPlainText() :
            mFormat == HtmlFormat       ? mainFrame->toHtml() :
            "bug");
      break;
    }
    default: {
      QImage image(mPage->viewportSize(), QImage::Format_ARGB32);
      painter.begin(&image);
      mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
      mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);      
      mainFrame->render(&painter);
      painter.end();
      // TODO: add quality
      image = image.scaledToWidth(mScaledWidth, Qt::SmoothTransformation);
      image.save(mOutput, format);
    }
  };
  mSawInitialLayout = false;
  mSawDocumentComplete = false;
}

void
CaptHelp(void) {
  printf("%s",
    " -----------------------------------------------------------------------------\n"
    " Usage: CutyCapt --url=http://www.example.org/ --out=localfile.png            \n"
    " -----------------------------------------------------------------------------\n"
    "  --help                         Print this help page and exit                \n"
    "  --queue-address=<string>       default: localhost                           \n"
    "  --queue-port=<string>          default: 22201                               \n"
    "  --primary-queue-name=<string>          default: queue_fast                  \n"
    "  --secondary-queue-name=<string>        default: queue_slow                  \n"
    "  --max-wait=<ms>                Don't wait more than (default: 9000, inf: 0)\n"
    "  --max-runs=<int>               Run <int> times before exiting (dflt: 50)\n"
    "  --sleep-check=<int>ms          check for new items every <int>ms (dflt: 100)\n"
    "  --instance=<string>            Just names this instance (does nothing)\n"
    "  ============================================================================\n"
    "  --delay=<ms>                   After successful load, wait (default: 0)     \n"
    "  --user-styles=<url>            Location of user style sheet, if any         \n"
    "  --header=<name>:<value>        request header; repeatable; some can't be set\n"
    "  --method=<get|post|put>        Specifies the request method (default: get)  \n"
    "  --body-string=<string>         Unencoded request body (default: none)       \n"
    "  --body-base64=<base64>         Base64-encoded request body (default: none)  \n"
    "  --app-name=<name>              appName used in User-Agent; default is none  \n"
    "  --app-version=<version>        appVers used in User-Agent; default is none  \n"
    "  --user-agent=<string>          Override the User-Agent header Qt would set  \n"
    "  --javascript=<on|off>          JavaScript execution (default: on)           \n"
    "  --java=<on|off>                Java execution (default: unknown)            \n"
    "  --plugins=<on|off>             Plugin execution (default: unknown)          \n"
    "  --private-browsing=<on|off>    Private browsing (default: unknown)          \n"
    "  --auto-load-images=<on|off>    Automatic image loading (default: on)        \n"
    "  --js-can-open-windows=<on|off> Script can open windows? (default: unknown)  \n"
    "  --js-can-access-clipboard=<on|off> Script clipboard privs (default: unknown)\n"
    " -----------------------------------------------------------------------------\n"
    "  <f> is svg,ps,pdf,itext,html,rtree,png,jpeg,mng,tiff,gif,bmp,ppm,xbm,xpm    \n"
    " -----------------------------------------------------------------------------\n"
    " http://cutycapt.sf.net - (c) 2003-2008 Bjoern Hoehrmann - bjoern@hoehrmann.de\n"
    "");
}

void Tokenize(const string& str,
                      vector<string>& tokens,
                      const string& delimiters = " ")
{
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}



int
main(int argc, char *argv[]) {

  // cout << "-- Begin main\n";

  int argHelp = 0;
  int argDelay = 0;
  int argSilent = 0;
  int argMaxWait = 9000;
  int argVerbosity = 0;
  const char* argUrl = NULL;
  const char* argUserStyle = NULL;
  const char* argIconDbPath = NULL;
  
  const char* queueAddress = "localhost";
  int queuePort = 22201;
  const char* primaryQueueName = "queue_fast";
  const char* secondaryQueueName = "queue_slow";
  int maxRuns = 50;
  int sleepCheck = 100;

  CutyCapt::OutputFormat format = CutyCapt::PngFormat;

  QApplication app(argc, argv, true);
  CutyPage page;

  QNetworkAccessManager::Operation method =
    QNetworkAccessManager::GetOperation;
  QByteArray body;
  QNetworkRequest req;

  // Parse command line parameters
  for (int ax = 1; ax < argc; ++ax) {
    size_t nlen;

    const char* s = argv[ax];
    const char* value;

    // boolean options
    if (strcmp("--silent", s) == 0) {
      argSilent = 1;
      continue;

    } else if (strcmp("--help", s) == 0) {
      argHelp = 1;
      break;

    } else if (strcmp("--verbose", s) == 0) {
      argVerbosity++;
      continue;
    } 

    value = strchr(s, '=');

    if (value == NULL) {
      // TODO: error
      argHelp = 1;
      break;
    }

    nlen = value++ - s;

    // --name=value options
    if (strncmp("--url", s, nlen) == 0) {
      argUrl = value;

    } else if (strncmp("--delay", s, nlen) == 0) {
      // TODO: see above
      argDelay = (unsigned int)atoi(value);

    }
    
    /////
    else if (strncmp("--queue-address", s, nlen) == 0) {
      queueAddress=value;
    }
    
    else if (strncmp("--queue-port", s, nlen) == 0) {
      queuePort = (unsigned int)atoi(value);
    }
    
    else if (strncmp("--primary-queue-name", s, nlen) == 0) {
        primaryQueueName = value;
    }

    else if (strncmp("--secondary-queue-name", s, nlen) == 0) {
        secondaryQueueName = value;
    }
    
    else if (strncmp("--max-runs", s, nlen) == 0) {
      maxRuns = (unsigned int)atoi(value);
    }    

    else if (strncmp("--sleep-check", s, nlen) == 0) {
      sleepCheck = (unsigned int)atoi(value);
    }    
    
    else if (strncmp("--instance", s, nlen) == 0) {
      // do nothing
    }
    //////
    
     else if (strncmp("--max-wait", s, nlen) == 0) {
      // TODO: see above
      argMaxWait = (unsigned int)atoi(value);

    } else if (strncmp("--user-styles", s, nlen) == 0) {
      argUserStyle = value;

    } else if (strncmp("--icon-database-path", s, nlen) == 0) {
      argIconDbPath = value;

    } else if (strncmp("--auto-load-images", s, nlen) == 0) {
      page.setAttribute(QWebSettings::AutoLoadImages, value);

    } else if (strncmp("--javascript", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavascriptEnabled, value);

    } else if (strncmp("--java", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavaEnabled, value);

    } else if (strncmp("--plugins", s, nlen) == 0) {
      page.setAttribute(QWebSettings::PluginsEnabled, value);

    } else if (strncmp("--private-browsing", s, nlen) == 0) {
      page.setAttribute(QWebSettings::PrivateBrowsingEnabled, value);

    } else if (strncmp("--js-can-open-windows", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavascriptCanOpenWindows, value);

    } else if (strncmp("--js-can-access-clipboard", s, nlen) == 0) {
      page.setAttribute(QWebSettings::JavascriptCanAccessClipboard, value);

    } else if (strncmp("--developer-extras", s, nlen) == 0) {
      page.setAttribute(QWebSettings::DeveloperExtrasEnabled, value);

    } else if (strncmp("--links-included-in-focus-chain", s, nlen) == 0) {
      page.setAttribute(QWebSettings::LinksIncludedInFocusChain, value);

    } else if (strncmp("--app-name", s, nlen) == 0) {
      app.setApplicationName(value);

    } else if (strncmp("--app-version", s, nlen) == 0) {
      app.setApplicationVersion(value);

    } else if (strncmp("--body-base64", s, nlen) == 0) {
      body = QByteArray::fromBase64(value);

    } else if (strncmp("--body-string", s, nlen) == 0) {
      body = QByteArray(value);

    } else if (strncmp("--user-agent", s, nlen) == 0) {
      page.setUserAgent(value);

    } else if (strncmp("--out-format", s, nlen) == 0) {
      for (int ix = 0; CutyExtMap[ix].id != CutyCapt::OtherFormat; ++ix)
        if (strcmp(value, CutyExtMap[ix].identifier) == 0)
          format = CutyExtMap[ix].id; //, break;

      if (format == CutyCapt::OtherFormat) {
        // TODO: error
        argHelp = 1;
        break;
      }

    } else if (strncmp("--header", s, nlen) == 0) {
      const char* hv = strchr(value, ':');

      if (hv == NULL) {
        // TODO: error
        argHelp = 1;
        break;
      }

      req.setRawHeader(QByteArray(value, hv - value), hv + 1);

    } else if (strncmp("--method", s, nlen) == 0) {
      if (strcmp("value", "get") == 0)
        method = QNetworkAccessManager::GetOperation;
      else if (strcmp("value", "put") == 0)
        method = QNetworkAccessManager::PutOperation;
      else if (strcmp("value", "post") == 0)
        method = QNetworkAccessManager::PostOperation;
      else if (strcmp("value", "head") == 0)
        method = QNetworkAccessManager::HeadOperation;
      else 
        (void)0; // TODO: ...

    } else {
      // TODO: error
      argHelp = 1;
    }
  }

  if (argHelp) {
      CaptHelp();
      return EXIT_FAILURE;
  }

  CutyCapt main(&page, argDelay, format);

  app.connect(&page,
    SIGNAL(loadFinished(bool)),
    &main,
    SLOT(DocumentComplete(bool)));

  app.connect(page.mainFrame(),
    SIGNAL(initialLayoutCompleted()),
    &main,
    SLOT(InitialLayoutCompleted()));

  if (argUserStyle != NULL)
    // TODO: does this need any syntax checking?
    page.settings()->setUserStyleSheetUrl( QUrl(argUserStyle) );

  if (argIconDbPath != NULL)
    // TODO: does this need any syntax checking?
    page.settings()->setIconDatabasePath(argUserStyle);

  // The documentation does not say, but it seems the mainFrame
  // will never change, so we can set this here. Otherwise we'd
  // have to set this in snapshot and trigger an update, which
  // is not currently possible (Qt 4.4.0) as far as I can tell.
  page.mainFrame()->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
  page.mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);

  // set up the memcache
  memcached_server_st *servers = NULL;
  memcached_st *memc;
  memcached_return rc;

  memcached_server_st *memcached_servers_parse (char *server_strings);
  memc= memcached_create(NULL);

  servers= memcached_server_list_append(servers, queueAddress, queuePort, &rc);
  rc= memcached_server_push(memc, servers);

  if (rc == MEMCACHED_SUCCESS)
    fprintf(stderr,"Added server successfully\n");
  else
    fprintf(stderr,"Couldn't add server: %s\n", memcached_strerror(memc, rc));

//    cout << "-- Added server " << endl;
//    const char *testvalue = "http://avatarnewyork.com|*|/tmp/avatar.png";
//    rc= memcached_set(memc, queueName, strlen(queueName), testvalue, strlen(testvalue), (time_t)0, (uint32_t)0);
//    cout << "Injected sample into queue" << endl;  
    // exit(0); 
    
//  std::cout << "Cached message is: " << cached_message << std::endl;
  vector<string> tokens;
  QUrl qurl = QUrl();
  QTimer *timer = new QTimer(&main);
  app.connect(timer, SIGNAL(timeout()), &main, SLOT(Timeout()));
  
  // memcache variables:
//  char *queued_message;
  size_t queued_message_length;
  uint32_t flags;
  int runs = 0;
  while (true) { 
      // cout << "-- ready to run url\n";
      const char* queued_message = memcached_get(memc, primaryQueueName, strlen(primaryQueueName),
                                    &queued_message_length, &flags, &rc);
      if (rc != MEMCACHED_SUCCESS)
      { 
          queued_message = memcached_get(memc, secondaryQueueName, strlen(secondaryQueueName),
                                         &queued_message_length, &flags, &rc);
         if (rc != MEMCACHED_SUCCESS)
         { 
              // cout << "-- Sleeping" << endl;
              usleep(1000 * sleepCheck);
              // cout << "-- Woke from sleep, checking queue" << endl;
              continue;       
          }
      }
      // cout << "-- queued message is: '" << queued_message << "'" << endl;

      Tokenize(queued_message, tokens, "|*|");
      const char* url = tokens[0].c_str();
      QString outfile = tokens[1].c_str();
      const int viewportWidth = atoi(tokens[2].c_str());
      const int viewportHeight = atoi(tokens[3].c_str());
      const int scaledWidth = atoi(tokens[4].c_str());
      
      main.mOutput = outfile;
      main.mScaledWidth = scaledWidth;
      
      cout << "-- url is: '" << url << "'" << endl;
      cout << "-- filename is: '" << main.mOutput.toStdString() << "'" << endl;
      cout << "-- vport width is: '" << viewportWidth << "'" << endl;
      cout << "-- vport height is: '" << viewportHeight << "'" << endl;
      cout << "-- scaledWidth is: '" << main.mScaledWidth << "'" << endl;
      
      qurl.setUrl(url);
      req.setUrl( qurl );

      page.setViewportSize( QSize(viewportWidth, viewportHeight) );

      if (!body.isNull())
        page.mainFrame()->load(req, method, body);
      else
        page.mainFrame()->load(req, method);
      
      timer->start(argMaxWait);
      app.exec();
      timer->stop();
      
      tokens.clear();
      
      runs += 1;
      if (maxRuns != 0 && runs >= maxRuns)
          break;
    }
//  cout << "-- Successful exit\n";
  exit(0);
}
