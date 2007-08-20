/***************************************************************************
  begin                : Fre Nov 15 2002
  copyright            : (C) Mark Kretschmann <markey@web.de>
                       : (C) Max Howell <max.howell@methylblue.com>
                       : (C) G??bor Lehel <illissius@gmail.com>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config-amarok.h"           //HAVE_LIBVISUAL definition

#include "actionclasses.h"    //see toolbar construction
#include "amarokconfig.h"
#include "amarok.h"
#include "analyzerwidget.h"
#include "collectionbrowser/CollectionWidget.h"
#include "context/CoverBling.h"
#include "context/ContextView.h"
#include "context/ControlBox.h"
#include "context/DataEngineManager.h"
#include "covermanager.h" // for actions
#include "debug.h"
#include "dynamicmode.h"
#include "editfilterdialog.h"
#include "enginecontroller.h" //for actions in ctor
#include "filebrowser.h"
#include "k3bexporter.h"
#include "lastfm.h"           //check credentials when adding lastfm streams
#include "MainWindow.h"
#include "mediabrowser.h"
#include "mediadevicemanager.h"
#include "playlist/PlaylistModel.h"
#include "playlist/PlaylistWidget.h"
#include "playlistbrowser.h"
#include "playlist.h"
#include "progressslider.h"
#include "scriptmanager.h"
#include "searchwidget.h"
#include "selectLabel.h"
#include "servicebrowser/magnatunestore/MagnatuneStore.h"
#include "servicebrowser/scriptableservice/scriptableservice.h"
#include "servicebrowser/servicebrowser.h"
#include "servicebrowser/jamendo/jamendoservice.h"
//#include "servicebrowser/mp3tunes/mp3tunesservice.h"
#include "sidebar.h"
#include "sidebar.moc"
#include "socketserver.h"
#include "statistics.h"
#include "statusbar.h"
#include "TheInstances.h"
#include "threadmanager.h"
#include "toolbar.h"
#include "volumewidget.h"
#include "PodcastCollection.h"

#include "playlistbrowser/PlaylistBrowser.h"

#include "queuemanager/QueueManager.h"

#include <Q3PopupMenu>
#include <QFont>
#include <QHeaderView>
#include <QLabel>           //search filter label
#include <QPainter>         //dynamic title
#include <QPen>
#include <QTimer>           //search filter timer
#include <QToolTip>         //QToolTip::add()
#include <QVBoxLayout>

#include <KAction>          //m_actionCollection
#include <KActionCollection>
#include <KActionMenu>
#include <kapplication.h>     //kapp
#include <kfiledialog.h>      //savePlaylist(), openPlaylist()
#include <kglobal.h>
#include <kinputdialog.h>     //slotAddStream()
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>      //savePlaylist()
#include <kmenu.h>
#include <kpushbutton.h>
#include <kstandarddirs.h>    //Welcome Tab, locate welcome.html
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <ktoolbarspaceraction.h>
   //createGUI()
#include <kxmlguibuilder.h>   //XMLGUI
#include <kxmlguifactory.h>   //XMLGUI
#include <fixx11h.h>

MainWindow *MainWindow::s_instance = 0;

MainWindow::MainWindow()
        :KXmlGuiWindow( 0, Qt::WGroupLeader )
        , m_lastBrowser( 0 )
{
    setObjectName("MainWindow");
    s_instance = this;

    new PlaylistNS::Model( this );

    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );

    createActions();

    //new K3bExporter();

    if( AmarokConfig::mainWindowSize().isValid() ) {
        // if first ever run, use sizeHint(), and let
        // KWindowSystem place us otherwise use the stored values
        resize( AmarokConfig::mainWindowSize() );
        move( AmarokConfig::mainWindowPos() );
    }
    m_browsers = new SideBar( this, new KVBox );
}

MainWindow::~MainWindow()
{
    AmarokConfig::setMainWindowPos( pos() );  //TODO de XT?
    AmarokConfig::setMainWindowSize( size() ); //TODO de XT?
}


///////// public interface

/**
 * This function will initialize the playlist window.
 */
void MainWindow::init()
{
    DEBUG_BLOCK

    //this function is necessary because Amarok::actionCollection() returns our actionCollection
    //via the App::m_pMainWindow pointer since App::m_pMainWindow is not defined until
    //the above ctor returns it causes a crash unless we do the initialisation in 2 stages.
    //<Dynamic Mode Status Bar />
    KVBox *playlistwindow = new KVBox;
    playlistwindow->setSpacing( 0 );
    playlistwindow->setMargin( 0 );
    playlistwindow->setMaximumSize( QSize( 300, 7000 ) );
    //make the playlist views resizable so the old one can be hidden!
    DynamicBar *dynamicBar = new DynamicBar( playlistwindow );
    Playlist *playlist = new Playlist( 0 ); //Playlist
    new PlaylistNS::Widget( playlistwindow );
    //This is our clear/undo/redo/save buttons
    KToolBar *plBar = new Amarok::ToolBar( playlistwindow );
    plBar->setObjectName( "PlaylistToolBar" );
    m_searchWidget = new SearchWidget( playlistwindow );
    m_searchWidget->setup( this );

    playlistwindow->setMinimumSize( QSize(250,100) );

    { //START Playlist toolbar
        plBar->setToolButtonStyle( Qt::ToolButtonIconOnly );
        plBar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        plBar->setIconDimensions( 22 );
        plBar->setMovable( false );
        plBar->addAction( new KToolBarSpacerAction( this ) );
        plBar->addAction( actionCollection()->action( "playlist_clear") );
        plBar->addAction( actionCollection()->action( "playlist_save") );
        plBar->addSeparator();
            plBar->addAction( actionCollection()->action( "playlist_undo") );
        plBar->addAction( actionCollection()->action( "playlist_redo") );
        plBar->addSeparator();
        plBar->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( actionCollection()->action("repeat") ), plBar ) );
        plBar->addWidget( new SelectLabel( static_cast<Amarok::SelectAction*>( actionCollection()->action("random_mode") ), plBar ) );
        plBar->addAction( new KToolBarSpacerAction( this ) );
    //END Playlist Toolbar
    }
    {
        m_controlBar = new KHBox( this );
        m_controlBar->setMaximumSize( 20000, 62 );
        m_controlBar->setContentsMargins( 0, 0, 0, 0 );
        m_controlBar->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

        AnalyzerWidget *aw = new AnalyzerWidget( m_controlBar );
        aw->setMinimumSize( 200, 30 );
        m_controlBar->layout()->setAlignment( aw, Qt::AlignLeft );
        KVBox *aVBox = new KVBox( m_controlBar );
        ProgressWidget *pWidget = new ProgressWidget( aVBox );
        pWidget->setMinimumSize( 400, 17 );
        pWidget->setMaximumSize( 600000, 17 );
        KHBox *insideBox = new KHBox( aVBox );
        aVBox->setMaximumSize( 50000, 60 );
        KToolBar *m_playerControlsToolbar = new Amarok::ToolBar( insideBox );
        m_playerControlsToolbar->setMinimumSize( 200, 45 );
        insideBox->layout()->setAlignment( m_playerControlsToolbar, Qt::AlignRight );
        VolumeWidget *vw = new VolumeWidget( insideBox );
        insideBox->setMaximumSize( 600000, 45 );
        vw->setMinimumSize( 200, 25 );
        insideBox->layout()->setAlignment( vw, Qt::AlignRight );

        m_playerControlsToolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
        m_playerControlsToolbar->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
        m_playerControlsToolbar->setIconDimensions( 32 );
        m_playerControlsToolbar->setMovable( false );
        m_playerControlsToolbar->addAction( actionCollection()->action( "prev" ) );
        m_playerControlsToolbar->addAction( actionCollection()->action( "play_pause" ) );
        m_playerControlsToolbar->addAction( actionCollection()->action( "stop" ) );
        m_playerControlsToolbar->addAction( actionCollection()->action( "next" ) );

    }

    QPalette p;
    QColor topColor = palette().highlight();
    if( topColor.lighter().isValid() )
        topColor = topColor.lighter().isValid();

    QColor bottomColor = palette().highlight();
    if( bottomColor.lighter().isValid() )
        bottomColor = bottomColor.lighter();

    QLinearGradient toolbarGradiant( m_controlBar->contentsRect().topLeft(),
                                     m_controlBar->contentsRect().bottomLeft() );
    toolbarGradiant.setColorAt( 0, topColor );
    toolbarGradiant.setColorAt( .5, topColor );
    toolbarGradiant.setColorAt( .6, bottomColor );
    toolbarGradiant.setColorAt( 1, bottomColor );
    QBrush b( toolbarGradiant );
    p.setBrush( QPalette::Window, b );

    m_controlBar->setAutoFillBackground( true );
    m_controlBar->setPalette( p );

    dynamicBar->init();
    this->toolBars().clear();

    Amarok::StatusBar *statusbar = new Amarok::StatusBar( playlistwindow );
    QAction* repeatAction = Amarok::actionCollection()->action( "repeat" );
    connect( repeatAction, SIGNAL( activated( int ) ), playlist, SLOT( slotRepeatTrackToggled( int ) ) );

    createMenus();
    QWidget *contextWidget = new QWidget( this );
    new Context::ContextView( contextWidget );
    {
        QVBoxLayout* layout = new QVBoxLayout( contextWidget );
        layout->addWidget( Context::ContextView::self() );
        if( AmarokConfig::useCoverBling() && QGLFormat::hasOpenGL() )
            layout->addWidget( new CoverBling( this ) );
        ControlBox* m_controlBox = new ControlBox( contextWidget );
        m_controlBox->show();
        // TODO fix the location of the controlbox so its not a few pixels out of the
        // context view
        connect(m_controlBox, SIGNAL(zoomIn()), Context::ContextView::self(), SLOT(zoomIn()));
        connect(m_controlBox, SIGNAL(zoomOut()), Context::ContextView::self(), SLOT(zoomOut()));
        connect(m_controlBox, SIGNAL(addApplet(const QString&)), Context::ContextView::self(), SLOT(addApplet(const QString&)));
        connect(m_controlBox, SIGNAL(lockInterface(bool)), Context::ContextView::self()->contextScene(), SLOT(setImmutable(bool)));
    }
    contextWidget->setMinimumSize( QSize(500,100) );

    m_browsers->setMaximumSize( QSize(300,7000) );
    m_browsers->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
    contextWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    playlistwindow->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );

    QWidget *centralWidget = new QWidget( this );

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins( 0, 0, 0, 0 );

    mainLayout->addWidget( m_controlBar );

    QSplitter *childSplitter = new QSplitter( Qt::Horizontal, centralWidget );
    childSplitter->addWidget( m_browsers );
    childSplitter->addWidget( contextWidget );
    childSplitter->addWidget( playlistwindow );
    mainLayout->addWidget( childSplitter );

    centralWidget->setLayout( mainLayout );

    setCentralWidget( centralWidget );

    playlist->setContentsMargins( 2,2,2,2 );
    playlist->installEventFilter( this ); //we intercept keyEvents

    //<XMLGUI>
    {
        QString xmlFile = Amarok::config().readEntry( "XMLFile", "amarokui.rc" );

        // this bug can bite you if you are a pre 1.2 user, we
        // deleted amarokui_first.rc, but we must still support it
        // NOTE 1.4.1 we remove amarokui_xmms.rc too, so we can only be this ui.rc
        xmlFile = "amarokui.rc";

        setXMLFile( xmlFile );
//         createGUI(); //NOTE we implement this
    }
    //</XMLGUI>

    //<Browsers>
    {
        Debug::Block block( "Creating browsers. Please report long start times!" );

        #define addBrowserMacro( Type, name, text, icon ) { \
            Debug::Block block( name ); \
             m_browsers->addWidget( KIcon( icon ), text, new Type( name ) ); \
             m_browserNames.append( name ); }

        #define addInstBrowserMacro( Type, name, text, icon ) { \
             m_browsers->addWidget( KIcon( icon ), text, Type::instance() ); \
             m_browserNames.append( name ); }

//         addBrowserMacro( ContextBrowser, "ContextBrowser", i18n("Context"), Amarok::icon( "info" ) )
        addBrowserMacro( CollectionWidget, "CollectionBrowser", i18n("Collection"), Amarok::icon( "collection" ) )
        //FIXME: figure this out
        //m_browsers->makeDropProxy( "CollectionBrowser", CollectionView::instance() );
        addInstBrowserMacro( PlaylistBrowser, "PlaylistBrowser", i18n("Playlists"), Amarok::icon( "playlist" ) )

        addBrowserMacro( PlaylistBrowserNS::PlaylistBrowser, "NeoPlaylistBrowser", i18n("Playlists"), Amarok::icon( "playlist" ) )

        //DEBUG: Comment out the addBrowserMacro line and uncomment the m_browsers line (passing in a vfat device name) to see the "virtual root" functionality

        addBrowserMacro( FileBrowser, "FileBrowser", i18n("Files"), Amarok::icon( "files" ) )
        //Add Magnatune browser
        //addInstBrowserMacro( MagnatuneStore, "MagnatuneStore", i18n("Magnatune"), Amarok::icon( "magnatune" ) )


        //cant use macros here since we need access to the browsers directly
       // ServiceBrowser * storeServiceBrowser = new ServiceBrowser(this, "Stores" );;
        //m_browsers->addWidget( KIcon( Amarok::icon( "magnatune" ) ), i18n("Stores"), storeServiceBrowser );
        //m_browserNames.append( "Stores" );


        ServiceBrowser * internetContentServiceBrowser = new ServiceBrowser(this, "Internet Content" );;
        m_browsers->addWidget( KIcon( Amarok::icon( "magnatune" ) ), i18n("Internet"), internetContentServiceBrowser );
        m_browserNames.append( "Internet" );

        debug() << "Add me dammit!!!!!";
        internetContentServiceBrowser->setScriptableServiceManager( new ScriptableServiceManager( 0 ) );

        internetContentServiceBrowser->addService( new MagnatuneStore( "Dummy service 1" ) );

        internetContentServiceBrowser->addService( new JamendoService( "Jamendo.com" ) );


        //internetContentServiceBrowser->addService( new Mp3tunesService( "Mp3tunes.com" ) );

        //addInstBrowserMacro( ServiceBrowser, "Stores", i18n("Stores"), Amarok::icon( "magnatune" ) )  //FIXME: icon
        //addInstBrowserMacro( ServiceBrowser, "Internet Content", i18n("Internet Content"), Amarok::icon( "magnatune" ) )  //FIXME: icon

        new MediaBrowser( "MediaBrowser" );
        if( MediaBrowser::isAvailable() )
        {
            addInstBrowserMacro( MediaBrowser, "MediaBrowser", i18n("Devices"), Amarok::icon( "device" ) )
            //to re-enable mediabrowser hiding, uncomment this:
            //connect( MediaBrowser::instance(), SIGNAL( availabilityChanged( bool ) ),
            //         this, SLOT( mbAvailabilityChanged( bool ) ) );
            //FIXME: figure this out
            //m_browsers->makeDropProxy( "MediaBrowser", MediaBrowser::queue() );

        }
        #undef addBrowserMacro
        #undef addInstBrowserMacro
    }
    //</Browsers>

    connect( Playlist::instance()->qscrollview(), SIGNAL( dynamicModeChanged( const DynamicMode* ) ),
             PlaylistBrowser::instance(), SLOT( loadDynamicItems() ) );


    kapp->installEventFilter( this ); // keyboards shortcuts for the browsers

    connect( playlist, SIGNAL( itemCountChanged(     int, int, int, int, int, int ) ),
             statusbar,  SLOT( slotItemCountChanged( int, int, int, int, int, int ) ) );
//     connect( playlist, SIGNAL( queueChanged( const QList<PlaylistItem*> &, const QList<PlaylistItem*> & ) ),
//              statusbar,  SLOT( updateQueueLabel() ) );
//    connect( playlist, SIGNAL( aboutToClear() ), m_lineEdit, SLOT( clear() ) );

    Amarok::MessageQueue::instance()->sendMessages();
}

void MainWindow::slotSetFilter( const QString &filter ) //SLOT
{
    Q_UNUSED( filter );
//    m_lineEdit->setText( filter );
}

void MainWindow::slotEditFilter() //SLOT
{
    EditFilterDialog *fd = new EditFilterDialog( this, true, "" );
    connect( fd, SIGNAL(filterChanged(const QString &)), SLOT(slotSetFilter(const QString &)) );
    if( fd->exec() )
        m_searchWidget->lineEdit()->setText( fd->filter() );
    delete fd;
}

void MainWindow::addBrowser( const QString &name, QWidget *browser, const QString &text, const QString &icon )
{
    if( !m_browserNames.contains( name ) )
    {
        m_browsers->addWidget( KIcon(Amarok::icon( icon )), text, browser );
        m_browserNames.append( name );
    }
}

void MainWindow::showBrowser( const QString &name )
{
    const int index = m_browserNames.indexOf( name );
    if( index >= 0 )
        m_browsers->showWidget( index );
}

/**
 * Reload the amarokui.rc xml file.
 * mainly just used by amarok::Menu
 */
void MainWindow::recreateGUI()
{
#if 0
    reloadXML();
    createGUI();
#endif
}


/**
 * Create the amarok gui from the xml file.
 */
#if 0
void MainWindow::createGUI()
{
    setUpdatesEnabled( false );

    LastFm::Controller::instance(); // create love/ban/skip actions

    m_toolbar->clear();


    /* FIXME: Is this still necessary?
    //KActions don't unplug themselves when the widget that is plugged is deleted!
    //we need to unplug to detect if the menu is plugged in App::applySettings()
    //TODO report to bugs.kde.org
    //we unplug after clear as otherwise it crashes! dunno why..
     KActionPtrList actions = actionCollection()->actions();
     for( KActionPtrList::Iterator it = actions.begin(), end = actions.end(); it != end; ++it )
         (*it)->unplug( m_toolbar );
    */
    KXMLGUIBuilder builder( this );
    KXMLGUIFactory factory( &builder, this );

    //build Toolbar, plug actions
    factory.addClient( this );

    //TEXT ON RIGHT HACK
    //KToolBarButtons have independent settings for their appearance.
    //KToolBarButton::modeChange() causes that button to set its mode to that of its parent KToolBar
    //KToolBar::setIconText() calls modeChange() for children, unless 2nd param is false

    QStringList list;
    list << "toolbutton_playlist_add"
//         << "toolbutton_playlist_clear"
//         << "toolbutton_playlist_shuffle"
//         << "toolbutton_playlist_show"
         << "toolbutton_burn_menu"
         << "toolbutton_amarok_menu";

    const QStringList::ConstIterator end  = list.constEnd();
    const QStringList::ConstIterator last = list.fromLast();
    for( QStringList::ConstIterator it = list.constBegin(); it != end; ++it )
    {
        QToolButton* const button = m_toolbar->findChild<QToolButton*>( (*it).toLatin1() );

        if ( it == last ) {
            //if the user has no PlayerWindow, he MUST have the menu action plugged
            //NOTE this is not saved to the local XMLFile, which is what the user will want
            if ( !AmarokConfig::showMenuBar() && !button )
                m_toolbar->addAction( actionCollection()->action( "amarok_menu" ) );
        }

        if ( button ) {
//TODO: can we delete modeChange safely? It doesn't appear to have a direct QToolButton equiv... is it still needed?
//             button->modeChange();
            button->setFocusPolicy( Qt::NoFocus );
        }
    }

    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly ); //default appearance
    m_toolbar->setMovable( false );
    m_toolbar->setAllowedAreas( Qt::TopToolBarArea );
    KToolBar::setToolBarsLocked( true );
    m_toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
//TODO: is this okay to remove? kdelibs-todo talks about removing it
//    conserveMemory();
    setUpdatesEnabled( true );
}
#endif

/**
 * Apply the loaded settings on the playlist window.
 * this function loads the custom fonts (if chosen) and than calls PlayList::instance()->applySettings();
 */
void MainWindow::applySettings()
{
    switch( AmarokConfig::useCustomFonts() )
    {
    case true:
        Playlist::instance()->setFont( AmarokConfig::playlistWindowFont() );
//        ContextBrowser::instance()->setFont( AmarokConfig::contextBrowserFont() );
        break;
    case false:
        Playlist::instance()->setFont( QFont() );
//        ContextBrowser::instance()->setFont( QFont() );
        break;
    }
}


/**
 * @param o The object
 * @param e The event
 *
 * Here we filter some events for the Playlist Search LineEdit and the Playlist. @n
 * this makes life easier since we have more useful functions available from this class
 */
bool MainWindow::eventFilter( QObject *o, QEvent *e )
{
    Playlist* const pl = Playlist::instance();
    typedef Q3ListViewItemIterator It;

    switch( e->type() )
    {
    case 6/*QEvent::KeyPress*/:

        //there are a few keypresses that we intercept

        #define e static_cast<QKeyEvent*>(e)

        if( e->key() == Qt::Key_F2 )
        {
            // currentItem is ALWAYS visible.
            Q3ListViewItem *item = pl->currentItem();

            // intercept F2 for inline tag renaming
            // NOTE: tab will move to the next tag
            // NOTE: if item is still null don't select first item in playlist, user wouldn't want that. It's silly.
            // TODO: berkus has solved the "inability to cancel" issue with K3ListView, but it's not in kdelibs yet..

            // item may still be null, but this is safe
            // NOTE: column 0 cannot be edited currently, hence we pick column 1
            pl->rename( item, 1 ); //TODO what if this column is hidden?

            return true;
        }


        if( o == m_searchWidget->lineEdit() ) //the search lineedit
        {
            Q3ListViewItem *item;
            switch( e->key() )
            {
            case Qt::Key_Up:
            case Qt::Key_Down:
            case Qt::Key_PageDown:
            case Qt::Key_PageUp:
                pl->setFocus();
                QApplication::sendEvent( pl, e );
                return true;

            case Qt::Key_Return:
            case Qt::Key_Enter:
                item = *It( pl, It::Visible );
                //m_lineEdit->clear();
                pl->m_filtertimer->stop(); //HACK HACK HACK

                if( e->modifiers() & Qt::ControlModifier )
                {
                    QList<PlaylistItem*> in, out;
                    if( e->modifiers() & Qt::ShiftModifier )
                        for( It it( pl, It::Visible ); PlaylistItem *x = static_cast<PlaylistItem*>( *it ); ++it )
                        {
                            pl->queue( x, true );
                            ( pl->m_nextTracks.contains( x ) ? in : out ).append( x );
                        }
                    else
                    {
                        It it( pl, It::Visible );
                        pl->activate( *it );
                        ++it;
                        for( int i = 0; PlaylistItem *x = static_cast<PlaylistItem*>( *it ); ++i, ++it )
                        {
                            in.append( x );
                            pl->m_nextTracks.insert( i, x );
                        }
                    }
                    if( !in.isEmpty() || !out.isEmpty() )
                        emit pl->queueChanged( in, out );
                    pl->setFilter( "" );
                    pl->ensureItemCentered( ( e->modifiers() & Qt::ShiftModifier ) ? item : pl->currentTrack() );
                }
                else
                {
                    pl->setFilter( "" );
                    if( ( e->modifiers() & Qt::ShiftModifier ) && item )
                    {
                        pl->queue( item );
                        pl->ensureItemCentered( item );
                    }
                    else
                    {
                        pl->activate( item );
                        pl->showCurrentTrack();
                    }
                }
                return true;

            case Qt::Key_Escape:
                m_searchWidget->lineEdit()->clear();
                return true;

            default:
                return false;
            }
        }

        //following are for Playlist::instance() only
        //we don't handle these in the playlist because often we manipulate the lineEdit too

        if( o == pl )
        {
            if( pl->currentItem() && ( e->key() == Qt::Key_Up && pl->currentItem()->itemAbove() == 0 && !(e->modifiers() & Qt::ShiftModifier) ) )
            {
                Q3ListViewItem *lastitem = *It( pl, It::Visible );
                if ( !lastitem )
                    return false;
                while( lastitem->itemBelow() )
                    lastitem = lastitem->itemBelow();
                pl->currentItem()->setSelected( false );
                pl->setCurrentItem( lastitem );
                lastitem->setSelected( true );
                pl->ensureItemVisible( lastitem );
                return true;
            }
            if( pl->currentItem() && ( e->key() == Qt::Key_Down && pl->currentItem()->itemBelow() == 0 && !(e->modifiers() & Qt::ShiftModifier) ) )
            {
                pl->currentItem()->setSelected( false );
                pl->setCurrentItem( *It( pl, It::Visible ) );
                (*It( pl, It::Visible ))->setSelected( true );
                pl->ensureItemVisible( *It( pl, It::Visible ) );
                return true;
            }
            if( e->key() == Qt::Key_Delete )
            {
                pl->removeSelectedItems();
                return true;
            }
            if( ( ( e->key() >= Qt::Key_0 && e->key() <= Qt::Key_Z ) || e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Escape ) && ( !e->modifiers() || e->modifiers() == Qt::ShiftModifier ) ) //only if shift or no modifier key is pressed and 0-Z or backspace or escape
            {
                m_searchWidget->lineEdit();
                QApplication::sendEvent( m_searchWidget->lineEdit(), e );
                return true;
            }
        }
        #undef e
        break;

    default:
        break;
    }

    return QWidget::eventFilter( o, e );
}


void MainWindow::closeEvent( QCloseEvent *e )
{
#ifdef Q_WS_MAC
    Q_UNUSED( e );
    hide();
#else
    Amarok::genericEventHandler( this, e );
#endif
}


void MainWindow::showEvent( QShowEvent* )
{
    static bool firstTime = true;
    if( firstTime )
        Playlist::instance()->setFocus();
    firstTime = false;
}

#include <qdesktopwidget.h>
QSize MainWindow::sizeHint() const
{
    return QApplication::desktop()->screenGeometry( (QWidget*)this ).size() / 1.5;
}


void MainWindow::savePlaylist() const //SLOT
{
    Playlist *pl = Playlist::instance();

    PlaylistItem *item = pl->firstChild();
    if( item && !item->isVisible() )
        item = static_cast<PlaylistItem*>( item->itemBelow() );

    QString title = pl->playlistName();

    if( item && title == i18n( "Untitled" ) )
    {
        QString artist = item->artist();
        QString album  = item->album();

        bool useArtist = true, useAlbum = true;

        item = static_cast<PlaylistItem*>( item->itemBelow() );

        for( ; item; item = static_cast<PlaylistItem*>( item->itemBelow() ) )
        {
            if( artist != item->artist() )
                useArtist = false;
            if( album  != item->album() )
                useAlbum = false;

            if( !useArtist && !useAlbum )
                break;
        }

        if( useArtist && useAlbum )
            title = i18n("%1 - %2", artist, album );
        else if( useArtist )
            title = artist;
        else if( useAlbum )
            title = album;
    }

    QString path = PlaylistDialog::getSaveFileName( title, pl->proposeOverwriteOnSave() );

    if( !path.isEmpty() && Playlist::instance()->saveM3U( path ) )
        MainWindow::self()->showBrowser( "PlaylistBrowser" );
}


void MainWindow::slotBurnPlaylist() const //SLOT
{
    K3bExporter::instance()->exportCurrentPlaylist();
}

void MainWindow::slotShowCoverManager() const //SLOT
{
    CoverManager::showOnce();
}

void MainWindow::slotPlayMedia() //SLOT
{
    // Request location and immediately start playback
    slotAddLocation( true );
}


void MainWindow::slotAddLocation( bool directPlay ) //SLOT
{
    // open a file selector to add media to the playlist
    KUrl::List files;
    //files = KFileDialog::getOpenURLs( QString::null, "*.*|" + i18n("All Files"), this, i18n("Add Media") );
    KFileDialog dlg( KUrl(QString()), QString("*.*|"), this );
    dlg.setCaption( directPlay ? i18n("Play Media (Files or URLs)") : i18n("Add Media (Files or URLs)") );
    dlg.setMode( KFile::Files | KFile::Directory );
    dlg.exec();
    files = dlg.selectedUrls();
    if( files.isEmpty() ) return;
    const int options = directPlay ? Playlist::Append | Playlist::DirectPlay : Playlist::Append;

    const KUrl::List::ConstIterator end  = files.constEnd();

    for(  KUrl::List::ConstIterator it = files.constBegin(); it != end; ++it )
        if( it == files.constBegin() )
            The::playlistModel()->insertMedia( *it, options );
        else
            The::playlistModel()->insertMedia( *it, Playlist::Append );
}

void MainWindow::slotAddStream() //SLOT
{
    bool ok;
    QString url = KInputDialog::getText( i18n("Add Stream"), i18n("URL"), QString(), &ok, this );

    if ( !ok ) return;

    KUrl::List media;
    media << KUrl( url );
    The::playlistModel()->insertMedia( media, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::playLastfmPersonal() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/personal" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmPersonal() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/personal" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::playLastfmNeighbor() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/neighbours" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmNeighbor() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    const KUrl url( QString( "lastfm://user/%1/neighbours" )
                    .arg( AmarokConfig::scrobblerUsername() ) );

    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::playLastfmCustom() //SLOT
{
    const QString token = LastFm::Controller::createCustomStation();
    if( token.isEmpty() ) return;

    const KUrl url( "lastfm://artist/" + token + "/similarartists" );
    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmCustom() //SLOT
{
    const QString token = LastFm::Controller::createCustomStation();
    if( token.isEmpty() ) return;

    const KUrl url( "lastfm://artist/" + token + "/similarartists" );
    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay  );
}


void MainWindow::playLastfmGlobaltag() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    KAction *action = dynamic_cast<KAction*>( sender() );
    if( !action ) return;

    const QString tag = action->text();
    const KUrl url( "lastfm://globaltags/" + tag );

    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay );
}


void MainWindow::addLastfmGlobaltag() //SLOT
{
    if( !LastFm::Controller::checkCredentials() ) return;

    KAction *action = dynamic_cast<KAction*>( sender() );
    if( !action ) return;

    const QString tag = action->text();
    const KUrl url( "lastfm://globaltags/" + tag );

    The::playlistModel()->insertMedia( url, Playlist::Append|Playlist::DirectPlay  );
}


void MainWindow::playAudioCD() //SLOT
{
    KUrl::List urls;
    if( EngineController::engine()->getAudioCDContents(QString(), urls) )
    {
        if (!urls.isEmpty())
            The::playlistModel()->insertMedia(urls, Playlist::Replace);
    }
    else
    { // Default behaviour
        showBrowser( "FileBrowser" );
      //FileBrowser *fb = static_cast<FileBrowser *>( m_browsers->at( m_browserNames.indexOf( "FileBrowser" ) ) );
      //fb->setUrl( KUrl("audiocd:/Wav/") );
    }
}

void MainWindow::showScriptSelector() //SLOT
{
    ScriptManager::instance()->show();
    ScriptManager::instance()->raise();
}

void MainWindow::showQueueManager() //SLOT
{
    if( QueueManagerNS::QueueManager::instance() )
    {
        QueueManagerNS::QueueManager::instance()->raise();
        return;
    }

    QueueManagerNS::QueueManager dialog;
    if( dialog.exec() == QDialog::Accepted )
    {
        // TODO: alter queue
    }
}

void MainWindow::showStatistics() //SLOT
{
    if( Statistics::instance() ) {
        Statistics::instance()->raise();
        return;
    }
    Statistics dialog;
    dialog.exec();
}

void MainWindow::slotToggleFocus() //SLOT
{
    if( m_browsers->currentWidget() && ( Playlist::instance()->hasFocus() || m_searchWidget->lineEdit()->hasFocus() ) )
        m_browsers->currentWidget()->setFocus();
}

void MainWindow::slotMenuActivated( int index ) //SLOT
{
    switch( index )
    {
    default:
        //saves duplicating the code and header requirements
        Amarok::Menu::instance()->slotActivated( index );
        break;
    case ID_SHOW_TOOLBAR:
        m_controlBar->setVisible( !m_controlBar->isHidden() );
        AmarokConfig::setShowToolbar( !AmarokConfig::showToolbar() );
        m_settingsMenu->changeItem( index, !m_controlBar->isHidden() ? i18n("Hide Toolbar") : i18n("Show Toolbar") );
        break;
    }
}

void MainWindow::actionsMenuAboutToShow() //SLOT
{
}

void MainWindow::toolsMenuAboutToShow() //SLOT
{
    m_toolsMenu->setItemEnabled( Amarok::Menu::ID_CONFIGURE_EQUALIZER, EngineController::hasEngineProperty( "HasEqualizer" ) );
    Amarok::actionCollection()->action( "rescan_collection" )->setEnabled( !ThreadManager::instance()->isJobPending( "CollectionScanner" ) );
}


#include <kwindowsystem.h>
/**
 * Show/hide playlist global shortcut and PlayerWindow PlaylistButton connect to this slot
 * RULES:
 * 1. hidden & iconified -> deiconify & show @n
 * 2. hidden & deiconified -> show @n
 * 3. shown & iconified -> deiconify @n
 * 4. shown & deiconified -> hide @n
 * 5. don't hide if there is no tray icon or playerWindow. todo (I can't be arsed) @n
 *
 * @note isMinimized() can only be true if the window isVisible()
 * this has taken me hours to get right, change at your peril!
 * there are more contingencies than you can believe
 */
void MainWindow::showHide() //SLOT
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
    const uint desktop = KWindowSystem::currentDesktop();
    const bool isOnThisDesktop = info.isOnDesktop( desktop );
    const bool isShaded = false;

    if( isShaded )
    {
        KWindowSystem::clearState( winId(), NET::Shaded );
        setVisible( true );
    }

    if( !isOnThisDesktop )
    {
        KWindowSystem::setOnDesktop( winId(), desktop );
        setVisible( true );
    }
    else if( !info.isMinimized() && !isShaded ) setVisible( !isVisible() );

    if( isVisible() ) KWindowSystem::unminimizeWindow( winId() );
#else
    setVisible( !isVisible() );
#endif
}

void MainWindow::activate()
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );

    if( KWindowSystem::activeWindow() != winId())
        setVisible( true );
    else if( !info.isMinimized() )
        setVisible( true );
    if( !isHidden() )
        KWindowSystem::activateWindow( winId() );
#else
    setVisible( true );
#endif
}

bool MainWindow::isReallyShown() const
{
#ifdef Q_WS_X11
    const KWindowInfo info = KWindowSystem::windowInfo( winId(), 0, 0 );
    return !isHidden() && !info.isMinimized() && info.isOnDesktop( KWindowSystem::currentDesktop() );
#else
    return isHidden();
#endif
}

void
MainWindow::mbAvailabilityChanged( bool isAvailable ) //SLOT
{
    if( isAvailable )
    {
        if( m_browserNames.indexOf( "MediaBrowser" ) == -1 )
            addBrowser( "MediaBrowser", MediaBrowser::instance(), i18n( "Media Device" ), Amarok::icon( "device" ) );
    }
    else
    {
        if( m_browserNames.indexOf( "MediaBrowser" ) != -1 )
        {
            showBrowser( "CollectionBrowser" );
            //removeMediaBrowser( MediaBrowser::instance() );
            m_browsers->removeWidget( MediaBrowser::instance() );
        }
    }
}

void MainWindow::createActions()
{
    KActionCollection* const ac = actionCollection();
    const EngineController* const ec = EngineController::instance();

    KStandardAction::configureToolbars( kapp, SLOT( slotConfigToolBars() ), ac );
    KStandardAction::keyBindings( kapp, SLOT( slotConfigShortcuts() ), ac );
    KStandardAction::preferences( kapp, SLOT( slotConfigAmarok() ), ac );
    ac->action(KStandardAction::name(KStandardAction::KeyBindings))->setIcon( KIcon( Amarok::icon( "configure" ) ) );
    ac->action(KStandardAction::name(KStandardAction::ConfigureToolbars))->setIcon( KIcon( Amarok::icon( "configure" ) ) );
    ac->action(KStandardAction::name(KStandardAction::Preferences))->setIcon( KIcon( Amarok::icon( "configure" ) ) );

    KStandardAction::quit( kapp, SLOT( quit() ), ac );
    
    KAction *action = new KAction( KIcon( Amarok::icon( "files" ) ), i18n("&Add Media..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddLocation() ) );
    ac->addAction( "playlist_add", action );

    action = new KAction( KIcon( Amarok::icon( "playlist_clear" ) ), i18nc( "clear playlist", "&Clear" ), this );
    connect( action, SIGNAL( triggered( bool ) ), The::playlistModel(), SLOT( clear() ) );
    ac->addAction( "playlist_clear", action );

    action = new KAction( KIcon( Amarok::icon( "files" ) ), i18n("&Add Stream..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( slotAddStream() ) );
    ac->addAction( "stream_add", action );

    action = new KAction( KIcon( Amarok::icon( "save" ) ), i18n("&Save Playlist As..."), this );
    connect( action, SIGNAL( triggered(bool) ), this, SLOT( savePlaylist() ) );
    ac->addAction( "playlist_save", action );

    KAction *burn = new KAction( KIcon(Amarok::icon( "burn" )), i18n( "Burn Current Playlist" ), this );
    connect( burn, SIGNAL( triggered(bool) ), SLOT( slotBurnPlaylist() ) );
    burn->setEnabled( K3bExporter::isAvailable() );
    ac->addAction( "playlist_burn", burn );

    KAction *covermanager = new KAction( KIcon(Amarok::icon( "covermanager" )), i18n( "Cover Manager" ), this );
    connect( covermanager, SIGNAL( triggered(bool) ), SLOT( slotShowCoverManager() ) );
    ac->addAction( "cover_manager", covermanager );

    KAction *visuals = new KAction( KIcon( Amarok::icon("visualizations") ), i18n("&Visualizations"), this );
    // connect( visuals, SIGNAL( triggered(bool) ), Vis::Selector::instance(), SLOT( show() ) );
    ac->addAction( "visualizations", visuals );

//     KAction *update_podcasts = new KAction( this );
//     update_podcasts->setText( i18n( "Update Podcasts" ) );
//     //update_podcasts->setIcon( KIcon(Amarok::icon( "refresh" )) );
//     ac->addAction( "podcasts_update", update_podcasts );
//     connect(update_podcasts, SIGNAL(triggered(bool)),
//             The::podcastCollection(), SLOT(slotUpdateAll()));

    KAction *playact = new KAction( KIcon(Amarok::icon( "files" )), i18n("Play Media..."), this );
    connect(playact, SIGNAL(triggered(bool)), SLOT(slotPlayMedia()));
    ac->addAction( "playlist_playmedia", playact );

    KAction *acd = new KAction( KIcon( Amarok::icon( "album" ) ), i18n("Play Audio CD"), this );
    connect(acd, SIGNAL(triggered(bool)), SLOT(playAudioCD()));
    ac->addAction( "play_audiocd", acd );

    KAction *script = new KAction( KIcon(Amarok::icon( "scripts" )), i18n("Script Manager"), this );
    connect(script, SIGNAL(triggered(bool)), SLOT(showScriptSelector()));
    ac->addAction( "script_manager", script );

    KAction *queue = new KAction( KIcon( Amarok::icon( "queue" )), i18n( "Queue Manager" ), this );
    connect(queue, SIGNAL(triggered(bool)), SLOT(showQueueManager()));
    ac->addAction( "queue_manager", queue );

    KAction *seekForward = new KAction( KIcon( Amarok::icon( "fastforward" ) ), i18n("&Seek Forward"), this );
    seekForward->setShortcut( Qt::Key_Right );
    connect(seekForward, SIGNAL(triggered(bool)), ec, SLOT(seekForward()));
    ac->addAction( "seek_forward", seekForward );

    KAction *seekBackward = new KAction( KIcon( Amarok::icon( "rewind" ) ), i18n("&Seek Backward"), this );
    seekForward->setShortcut( Qt::Key_Left );
    connect(seekForward, SIGNAL(triggered(bool)), ec, SLOT(seekBackward()));
    ac->addAction( "seek_backward", seekBackward );
    
    KAction *statistics = new KAction( KIcon(Amarok::icon( "info" )), i18n( "Statistics" ), this );
    connect(statistics, SIGNAL(triggered(bool)), SLOT(showStatistics()));
    ac->addAction( "statistics", statistics );

    KAction *update = new KAction( KIcon(Amarok::icon( "refresh")), i18n( "Update Collection" ), this );
    connect(update, SIGNAL(triggered(bool)), CollectionDB::instance(), SLOT(scanModifiedDirs()));
    ac->addAction( "update_collection", update );

    KAction *rescan = new KAction( KIcon(Amarok::icon( "rescan")), i18n( "Rescan Collection" ), this );
    connect(update, SIGNAL(triggered(bool)), CollectionDB::instance(), SLOT(startScan()));
    rescan->setEnabled( !ThreadManager::instance()->isJobPending( "CollectionScanner" ) );
    ac->addAction( "rescan_collection", rescan );

    m_lastfmTags << "Alternative" <<  "Ambient" << "Chill Out" << "Classical" << "Dance"
                 << "Electronica" << "Favorites" << "Heavy Metal" << "Hip Hop" << "Indie Rock"
                 << "Industrial" << "Japanese" << "Pop" << "Psytrance" << "Rap" << "Rock"
                 << "Soundtrack" << "Techno" << "Trance";

    KMenu* playTagRadioMenu = new KMenu( this );
    playTagRadioMenu->setTitle( i18n( "Global Tag Radio" ) );
    foreach( QString lastfmTag, m_lastfmTags )
    {
        KAction *lastfmAction = new KAction( lastfmTag, this );
        connect( lastfmAction, SIGNAL( triggered(bool) ), SLOT( playLastfmGlobaltag() ) );
        playTagRadioMenu->addAction( lastfmAction );
    }

    KMenu* addTagRadioMenu = new KMenu( this );
    addTagRadioMenu->setTitle( i18n( "Global Tag Radio" ) );
    foreach( QString lastfmTag, m_lastfmTags )
    {
        KAction *lastfmAction = new KAction( lastfmTag, this );
        connect( lastfmAction, SIGNAL( triggered(bool) ), SLOT( addLastfmGlobaltag() ) );
        addTagRadioMenu->addAction( lastfmAction );
    }

    KActionMenu* playLastfm = new KActionMenu( KIcon(Amarok::icon("audioscrobbler")), i18n( "Play las&t.fm Stream" ), ac);
    KMenu* playLastfmMenu = playLastfm->menu();
    playLastfmMenu->addAction( i18n( "Personal Radio" ), this, SLOT( playLastfmPersonal() ) );
    playLastfmMenu->addAction( i18n( "Neighbor Radio" ), this, SLOT( playLastfmNeighbor() ) );
    playLastfmMenu->addAction( i18n( "Custom Station" ), this, SLOT( playLastfmCustom() ) );
    playLastfmMenu->addMenu( playTagRadioMenu );
    ac->addAction( "lastfm_play", playLastfm );

    KActionMenu* addLastfm = new KActionMenu( KIcon(Amarok::icon("audioscrobbler")), i18n( "Add las&t.fm Stream" ), ac);
    KMenu* addLastfmMenu = addLastfm->menu();
    addLastfmMenu->addAction( i18n( "Personal Radio" ), this, SLOT( addLastfmPersonal() ) );
    addLastfmMenu->addAction( i18n( "Neighbor Radio" ), this, SLOT( addLastfmNeighbor() ) );
    addLastfmMenu->addAction( i18n( "Custom Station" ), this, SLOT( addLastfmCustom() ) );
    addLastfmMenu->addMenu( addTagRadioMenu );
    ac->addAction( "lastfm_add", addLastfm );

    KAction *previous = new KAction( this );
    previous->setIcon( KIcon(Amarok::icon( "back" )) );
    previous->setText( i18n( "Previous Track" ) );
    ac->addAction( "prev", previous );
    connect( previous, SIGNAL(triggered(bool)), ec, SLOT( previous() ) );

    KAction *play = new KAction( this );
    play->setIcon( KIcon(Amarok::icon( "play" )) );
    play->setText( i18n( "Play" ) );
    ac->addAction( "play", play );
    connect( play, SIGNAL(triggered(bool)), ec, SLOT( play() ));

    KAction *pause = new KAction( this );
    pause->setIcon( KIcon(Amarok::icon( "pause" )) );
    pause->setText( i18n( "Pause" ));
    ac->addAction( "pause", pause );
    connect( pause, SIGNAL(triggered(bool)), ec, SLOT( pause() ) );

    KAction *next = new KAction( this );
    next->setIcon( KIcon(Amarok::icon( "next" )) );
    next->setText( i18n( "Next Track" ) );
    ac->addAction( "next", next );
    connect( next, SIGNAL(triggered(bool)), ec, SLOT( next() ) );

    KAction *toggleFocus = new KAction(i18n( "Toggle Focus" ), ac);
    toggleFocus->setShortcut( Qt::ControlModifier + Qt::Key_Tab );
    connect( toggleFocus, SIGNAL(triggered(bool)), SLOT( slotToggleFocus() ));

    KAction *spacer = new KToolBarSpacerAction( this );
    ac->addAction( "spacer", spacer );

    KAction *spacer1 = new KToolBarSpacerAction( this );
    ac->addAction( "spacer1", spacer1 );

    KAction *spacer2 = new KToolBarSpacerAction( this );
    ac->addAction( "spacer2", spacer2 );

    KAction *spacer3 = new KToolBarSpacerAction( this );
    ac->addAction( "spacer3", spacer3 );


    new Amarok::MenuAction( ac );
    new Amarok::StopAction( ac );
    new Amarok::PlayPauseAction( ac );
    new Amarok::RepeatAction( ac );
    new Amarok::RandomAction( ac );
    new Amarok::FavorAction( ac );

    if( K3bExporter::isAvailable() )
        new Amarok::BurnMenuAction( ac );

    ac->setAssociatedWidget( this );
}

void MainWindow::createMenus()
{
    m_menubar = menuBar();//new MenuBar( this );

    //BEGIN Actions menu
    KMenu *actionsMenu = new KMenu( m_menubar );
    actionsMenu->setTitle( i18n("&Amarok") );
    actionsMenu->addAction( actionCollection()->action("playlist_playmedia") );
    actionsMenu->addAction( actionCollection()->action("lastfm_play") );
    actionsMenu->addAction( actionCollection()->action("play_audiocd") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( actionCollection()->action("prev") );
    actionsMenu->addAction( actionCollection()->action("play_pause") );
    actionsMenu->addAction( actionCollection()->action("stop") );
    actionsMenu->addAction( actionCollection()->action("next") );
    actionsMenu->addSeparator();
    actionsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Quit)) );


    connect( actionsMenu, SIGNAL( aboutToShow() ), SLOT( actionsMenuAboutToShow() ) );
    //END Actions menu

    //BEGIN Playlist menu
    KMenu *playlistMenu = new KMenu( m_menubar );
    playlistMenu->setTitle( i18n("&Playlist") );
    playlistMenu->addAction( actionCollection()->action("playlist_add") );
    playlistMenu->addAction( actionCollection()->action("stream_add") );
    playlistMenu->addAction( actionCollection()->action("lastfm_add") );
    playlistMenu->addAction( actionCollection()->action("playlist_save") );
    playlistMenu->addAction( actionCollection()->action("playlist_burn") );
    playlistMenu->addAction( actionCollection()->action("podcasts_update") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("playlist_undo") );
    playlistMenu->addAction( actionCollection()->action("playlist_redo") );
    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("playlist_clear") );
    playlistMenu->addAction( actionCollection()->action("playlist_shuffle") );

    playlistMenu->addSeparator();
    playlistMenu->addAction( actionCollection()->action("queue_selected") );
    playlistMenu->addAction( actionCollection()->action("playlist_remove_duplicates") );
    playlistMenu->addAction( actionCollection()->action("playlist_select_all") );

    //END Playlist menu

    //BEGIN Mode menu
    KMenu *modeMenu = new KMenu( m_menubar );
    modeMenu->setTitle( i18n("&Mode") );
    modeMenu->addAction( actionCollection()->action("repeat") );
    KSelectAction *random = static_cast<KSelectAction*>( actionCollection()->action("random_mode") );
    modeMenu->addAction( random );
    random->menu()->addSeparator();
    random->menu()->addAction( actionCollection()->action("favor_tracks") );
    //END Mode menu

    //BEGIN Tools menu
    m_toolsMenu = new KMenu( m_menubar );
    m_toolsMenu->setTitle( i18n("&Tools") );
    m_toolsMenu->addAction( actionCollection()->action("cover_manager") );
    m_toolsMenu->addAction( actionCollection()->action("queue_manager") );
    m_toolsMenu->addAction( actionCollection()->action("visualizations") );
    m_toolsMenu->addAction( KIcon( Amarok::icon( "equalizer") ), i18n("&Equalizer"), kapp, SLOT( slotConfigEqualizer() ) );
    m_toolsMenu->addAction( actionCollection()->action("script_manager") );
    m_toolsMenu->addAction( actionCollection()->action("statistics") );
    m_toolsMenu->addSeparator();
    m_toolsMenu->addAction( actionCollection()->action("update_collection") );
    m_toolsMenu->addAction( actionCollection()->action("rescan_collection") );

    #ifndef HAVE_LIBVISUAL
    actionCollection()->action( "visualizations" )->setEnabled( false );
    #endif

    connect( m_toolsMenu, SIGNAL( aboutToShow() ), SLOT( toolsMenuAboutToShow() ) );
    connect( m_toolsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END Tools menu

    //BEGIN Settings menu
    m_settingsMenu = new KMenu( m_menubar );
    m_settingsMenu->setTitle( i18n("&Settings") );
    //TODO use KStandardAction or KXmlGuiWindow
#ifndef Q_WS_MAC
    m_settingsMenu->insertItem( AmarokConfig::showToolbar() ? i18n( "Hide Toolbar" ) : i18n("Show Toolbar"), ID_SHOW_TOOLBAR );
    m_settingsMenu->addSeparator();
#endif

#ifdef Q_WS_MAC
    // plug it first, as this item will be moved to the applications first menu
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Preferences)) );
#endif
//    m_settingsMenu->addAction( actionCollection()->action("options_configure_globals") );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings)) );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::ConfigureToolbars)) );
    m_settingsMenu->addAction( actionCollection()->action(KStandardAction::name(KStandardAction::Preferences)) );

    connect( m_settingsMenu, SIGNAL( activated(int) ), SLOT( slotMenuActivated(int) ) );
    //END Settings menu

    m_menubar->addMenu( actionsMenu );
    m_menubar->addMenu( playlistMenu );
    m_menubar->addMenu( modeMenu );
    m_menubar->addMenu( m_toolsMenu );
    m_menubar->addMenu( m_settingsMenu );
    m_menubar->addMenu( Amarok::Menu::helpMenu() );
}

//////////////////////////////////////////////////////////////////////////////////////////
/// DynamicBar
//////////////////////////////////////////////////////////////////////////////////////////
DynamicBar::DynamicBar(QWidget* parent)
       : KHBox( parent)
{
    setObjectName( "dynamicBar" );
    m_titleWidget = new DynamicTitle( this );

    setSpacing( KDialog::spacingHint() );
    QWidget *spacer = new QWidget( this );
    setStretchFactor( spacer, 10 );
}

// necessary because it has to be constructed before Playlist::instance(), but also connect to it
void DynamicBar::init()
{
    connect(Playlist::instance()->qscrollview(), SIGNAL(dynamicModeChanged(const DynamicMode*)),
                                                   SLOT(slotNewDynamicMode(const DynamicMode*)));

    KPushButton* editDynamicButton = new KPushButton( i18n("Edit"), this );
    connect( editDynamicButton, SIGNAL(clicked()), Playlist::instance()->qscrollview(), SLOT(editActiveDynamicMode()) );

    KPushButton* repopButton = new KPushButton( i18n("Repopulate"), this );
    connect( repopButton, SIGNAL(clicked()), Playlist::instance()->qscrollview(), SLOT(repopulate()) );

    KPushButton* disableButton = new KPushButton( i18n("Turn Off"), this );
    connect( disableButton, SIGNAL(clicked()), Playlist::instance()->qscrollview(), SLOT(disableDynamicMode()) );

    slotNewDynamicMode( Playlist::instance()->dynamicMode() );
}

void DynamicBar::slotNewDynamicMode(const DynamicMode* mode)
{
    setVisible(mode);
    if (mode)
        changeTitle(mode->title());
}

void DynamicBar::changeTitle(const QString& title)
{
   m_titleWidget->setTitle(title);
}

//////////////////////////////////////////////////////////////////////////////////////////
/// DynamicTitle
//////////////////////////////////////////////////////////////////////////////////////////
DynamicTitle::DynamicTitle(QWidget* w)
    : QWidget()
{
    setParent( w );
    setObjectName( "dynamicTitle" );
    m_font.setBold( true );
    setTitle("");
}

void DynamicTitle::setTitle(const QString& newTitle)
{
    m_title = newTitle;
    QFontMetrics fm(m_font);
    setMinimumWidth( s_curveWidth*3 + fm.width(m_title) + s_imageSize );
    setMinimumHeight( fm.height() );
}

void DynamicTitle::paintEvent(QPaintEvent* /*e*/)
{
    QPainter p;
    p.begin( this );
    QPen pen( palette().highlightedText(), 0, Qt::NoPen );
    p.setPen( pen );
    p.setBrush( palette().highlight() );
    p.setFont( m_font );


    QFontMetrics fm( m_font );
    int textHeight = fm.height();
    if (textHeight < s_imageSize)
        textHeight = s_imageSize;
    int textWidth = fm.width(m_title);
    int yStart = (height() - textHeight) / 2;
    if(yStart < 0)
        yStart = 0;

    p.drawEllipse( 0, yStart, s_curveWidth * 2, textHeight);
    p.drawEllipse( s_curveWidth + textWidth + s_imageSize, yStart, s_curveWidth*2, textHeight);
    p.fillRect( s_curveWidth, yStart, textWidth + s_imageSize + s_curveWidth, textHeight, QBrush( palette().highlight()) );
    p.drawPixmap( s_curveWidth, yStart + ((textHeight - s_imageSize) /2), SmallIcon("dynamic") );
    //not sure why first arg of Rect shouldn't add @curveWidth
    p.drawText( QRect(s_imageSize, yStart, s_curveWidth + textWidth +s_imageSize, textHeight), Qt::AlignCenter, m_title);
}

#include "MainWindow.moc"
