/**************************************************************************
 *   Copyright (C) 2006 by UC Davis Stahlberg Laboratory                   *
 *   HStahlberg@ucdavis.edu                                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "mainWindow.h"
#include "scriptTab.h"
#include <QDebug>
#include <QDesktopServices>
#include <QModelIndexList>
#include <QItemSelectionModel>
#include <QTabWidget>
#include <QToolBar>
#include <iostream>
#include "blockContainer.h"

using namespace std;

mainWindow::mainWindow(const QString &directory, QWidget *parent)
: QMainWindow(parent) {

    m_do_autosave = true;
    mainData = setupMainConfiguration(directory);
    userData = new confData(QDir::homePath() + "/.2dx/" + "2dx_merge-user.cfg", mainData->getDir("config") + "/" + "2dx_merge-user.cfg");
    userData->save();
    mainData->setUserConf(userData);
    
    installedVersion = mainData->version();
    setWindowTitle("2dx (" + installedVersion + ")");
    setUnifiedTitleAndToolBarOnMac(true);
    
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QGridLayout* layout = new QGridLayout(centralWidget);
    layout->setMargin(0);
    layout->setSpacing(0);
    centralWidget->setLayout(layout);

    connect(&importProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(importFinished()));

    progressBar = setupProgressBar();
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->addPermanentWidget(progressBar);
    
    updates = new updateWindow(mainData, this);
    updates->hide();


    setupScriptModules();

    results = new resultsData(mainData, mainData->getDir("working") + "/LOGS/" + "2dx_initialization.results", mainData->getDir("working"), this);

    albumCont = new albumContainer(mainData, results, this);

    logViewer = new LogViewer("Standard Output", NULL);

    connect(standardScripts, SIGNAL(standardOut(const QStringList &)), logViewer, SLOT(insertText(const QStringList &)));
    connect(standardScripts, SIGNAL(standardError(const QByteArray &)), logViewer, SLOT(insertError(const QByteArray &)));

    connect(customScripts, SIGNAL(standardOut(const QStringList &)), logViewer, SLOT(insertText(const QStringList &)));
    connect(customScripts, SIGNAL(standardError(const QByteArray &)), logViewer, SLOT(insertError(const QByteArray &)));

    connect(singleParticleScripts, SIGNAL(standardOut(const QStringList &)), logViewer, SLOT(insertText(const QStringList &)));
    connect(singleParticleScripts, SIGNAL(standardError(const QByteArray &)), logViewer, SLOT(insertError(const QByteArray &)));

    connect(standardScripts, SIGNAL(scriptLaunched()), logViewer, SLOT(clear()));
    connect(customScripts, SIGNAL(scriptLaunched()), logViewer, SLOT(clear()));
    connect(singleParticleScripts, SIGNAL(scriptLaunched()), logViewer, SLOT(clear()));

    verbosityControl = new levelGroup(mainData, 4,
            QStringList() << "Logfile - Silent   (Click here for logbrowser)" << "Logfile - Low Verbosity (Click here for logbrowser)" << "Logfile - Moderate Verbosity (Click here for logbrowser)" << "Logfile - Highest Verbosity (Click here for logbrowser)",
            QStringList() << "Update only on script completion." << "Low Verbosity Output" << "Moderate Verbosity Output" << "Highest Verbosity Output",
            QStringList() << "gbAqua" << "gbBlue" << "gbOrange" << "gbRed");

    verbosityControl->setLevel(1);

    viewContainer *logWindow = new viewContainer("Logfile - Low Verbosity", viewContainer::data, NULL, viewContainer::grey);
    connect(logWindow, SIGNAL(doubleClicked()), this, SLOT(launchLogBrowser()));
    logWindow->setHeaderWidget(verbosityControl);
    logWindow->addWidget(logViewer);
    connect(verbosityControl, SIGNAL(titleChanged(const QString &)), logWindow, SLOT(setText(const QString &)));
    connect(verbosityControl, SIGNAL(levelChanged(int)), logViewer, SLOT(load(int)));
    connect(verbosityControl, SIGNAL(levelChanged(int)), standardScripts, SLOT(setVerbosity(int)));
    connect(verbosityControl, SIGNAL(levelChanged(int)), customScripts, SLOT(setVerbosity(int)));
    connect(verbosityControl, SIGNAL(levelChanged(int)), singleParticleScripts, SLOT(setVerbosity(int)));

    standardScriptsTab = new scriptTab(standardScripts, mainData, this);
    customScriptsTab = new scriptTab(customScripts, mainData, this);
    singleParticleScriptsTab = new scriptTab(singleParticleScripts, mainData, this);

    connect(results, SIGNAL(saved(bool)), standardScriptsTab, SLOT(loadParameters()));
    connect(results, SIGNAL(saved(bool)), customScriptsTab, SLOT(loadParameters()));
    connect(results, SIGNAL(saved(bool)), singleParticleScriptsTab, SLOT(loadParameters()));

    connect(standardScripts, SIGNAL(currentScriptChanged(QModelIndex)), this, SLOT(standardScriptChanged(QModelIndex)));
    connect(customScripts, SIGNAL(currentScriptChanged(QModelIndex)), this, SLOT(customScriptChanged(QModelIndex)));
    connect(singleParticleScripts, SIGNAL(currentScriptChanged(QModelIndex)), this, SLOT(singleParticleScriptChanged(QModelIndex)));

    

    int fontSize = mainData->userConf()->get("fontSize", "value").toInt();
    if (fontSize != 0) {
        QFont mainFont(QApplication::font());
        mainFont.setPointSize(fontSize);
        QApplication::setFont(mainFont);
        updateFontInfo();
    } else {
        QFont font = QApplication::font();
        mainData->userConf()->set("fontSize", QString::number(font.pointSize()));
        mainData->userConf()->save();
        QApplication::setFont(font);
        updateFontInfo();
    }

    viewContainer *resultsContainer = new viewContainer("Results", viewContainer::data, this, viewContainer::grey);
    resultsView = new resultsModule(mainData, results, resultsModule::results, mainData->getDir("project"));
    resultsContainer->addWidget(resultsView);
    resultsContainer->setMinimumSize(QSize(200, 200));

    viewContainer *imagesContainer = new viewContainer("Images  (Click here to open)", viewContainer::data, this, viewContainer::grey);
    connect(imagesContainer, SIGNAL(doubleClicked()), this, SLOT(launchFileBrowser()));
    resultsModule *imagesView = new resultsModule(mainData, results, resultsModule::images, mainData->getDir("project"));
    imagesContainer->addWidget(imagesView);
    imagesContainer->setMinimumSize(QSize(200, 200));

    levelGroup *imageLevelButtons = new levelGroup(mainData, 2, QStringList() << "All Images" << "Important Images", QStringList() << "Show all images" << "Show only important images", QStringList() << "gbAqua" << "gbRed");
    levelGroup *imageNamesButtons = new levelGroup(mainData, 2, QStringList() << "Nicknames" << "Filenames", QStringList() << "Show Nicknames" << "Show File Names", QStringList() << "gbOrange" << "gbPurple");
    imagesContainer->setHeaderWidget(imageLevelButtons, Qt::AlignLeft);
    imagesContainer->setHeaderWidget(imageNamesButtons, Qt::AlignRight);
    connect(imageLevelButtons, SIGNAL(levelChanged(int)), imagesView, SLOT(setImportant(int)));
    connect(imageNamesButtons, SIGNAL(levelChanged(int)), imagesView, SLOT(setShowFilenames(int)));


    QSplitter *resultsSplitter = new QSplitter(Qt::Horizontal, this);
    resultsSplitter->addWidget(resultsContainer);
    resultsSplitter->addWidget(imagesContainer);

    scriptsWidget = new QTabWidget(this);
    scriptsWidget->setDocumentMode(true);
    scriptsWidget->setTabPosition(QTabWidget::West);

#ifdef Q_OS_MAC  
    scriptsWidget->setStyleSheet(
            "QTabBar::tab {color: white;}"
            "QTabBar::tab:selected {color: black;}"
            );
#endif

    scriptsWidget->addTab(standardScriptsTab, "Standard");
    scriptsWidget->addTab(customScriptsTab, "Custom");
    scriptsWidget->addTab(singleParticleScriptsTab, "Single Particle");
    connect(scriptsWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    blockContainer* processContainer = new blockContainer("PROCESS", this);
    processContainer->addWidget(scriptsWidget);

    blockContainer* evaluateContainer = new blockContainer("EVALUATE", this);
    evaluateContainer->addWidget(logWindow);
    evaluateContainer->addWidget(resultsSplitter);


    QSplitter* container = new QSplitter(Qt::Vertical, this);
    container->addWidget(albumCont);
    container->addWidget(processContainer);
    container->addWidget(evaluateContainer);

    //layout->addWidget(headerWidget, 0, 0, 1, 1);
    layout->addWidget(container, 0, 0, 1, 1);
    //layout->addWidget(footerWidget, 1, 0, 1, 1);

    about = new aboutWindow(mainData, this, true);
    about->hide();

    setupActions();
    setupToolBar();
    setupMenuBar();
    
    album = NULL;
    euler = NULL;
    reproject = NULL;

    importCount = 0;

    verbosityControl->setLevel(1);
    standardScripts->initialize();

}

confData* mainWindow::setupMainConfiguration(const QString &directory) 
{
    confData* mainData;
    
    QDir applicationDir, configDir;

#ifdef Q_OS_MAC
    applicationDir = QDir(QApplication::applicationDirPath() + "/../../../");
#else
    applicationDir = QDir(QApplication::applicationDirPath());
#endif

    configDir = QDir(applicationDir.canonicalPath() + "/../" + "config/");

    QString mergeConfigLocation = directory + "/merge/" + "2dx_merge.cfg";
    QString appConfigLocation = configDir.canonicalPath() + "/" + "2dx_master.cfg";
    if (QFileInfo(mergeConfigLocation).exists()) {
        mainData = new confData(mergeConfigLocation, appConfigLocation);
        if (QFileInfo(appConfigLocation).exists()) {
            mainData->updateConf(appConfigLocation);
        }
    } else {
        mainData = new confData(mergeConfigLocation, appConfigLocation);
    }
    mainData->setDir("project", QDir(directory));
    mainData->setDir("working", QDir(directory + "/merge"));

    if (!QFileInfo(mainData->getDir("working") + "/" + "2dx_merge.cfg").exists()) mainData->save();
    
    mainData->setDir("application", applicationDir);

    mainData->setDir("binDir", mainData->getDir("application") + "/bin/");
    mainData->setDir("procDir", mainData->getDir("application") + "../proc/");
    createDir(mainData->getDir("working") + "/config");

    mainData->setDir("config", configDir);

    createDir(QDir::homePath() + "/.2dx/");
    QString userPath = QDir::homePath() + "/.2dx";
    createDir(userPath + "/2dx_merge");

    confData *cfg = new confData(userPath + "/2dx.cfg", mainData->getDir("config") + "/" + "2dx.cfg");
    if (cfg->isEmpty()) {
        cerr << "2dx.cfg not found." << endl;
        exit(0);
    }
    cfg->save();

    mainData->setAppConf(cfg);

    mainData->setDir("home_2dx", userPath);
    mainData->setDir("pluginsDir", mainData->getDir("application") + "/.." + "/plugins");
    mainData->setDir("translatorsDir", mainData->getDir("pluginsDir") + "/translators");
    mainData->setDir("resource", QDir(mainData->getDir("config") + "/resource/"));
    mainData->setDir("2dx_bin", mainData->getDir("application") + "/.." + "/bin");
    mainData->addApp("this", mainData->getDir("application") + "/../" + "bin/" + "2dx_merge");
    mainData->addApp("2dx_image", mainData->getDir("2dx_bin") + "/" + "2dx_image");
    mainData->addApp("2dx_merge", mainData->getDir("2dx_bin") + "/" + "2dx_merge");

    createDir(mainData->getDir("working") + "/proc");
    mainData->setDir("remoteProc", mainData->getDir("working") + "/proc/");
    createDir(mainData->getDir("working") + "/LOGS");
    mainData->setDir("logs", mainData->getDir("working") + "/LOGS");
    mainData->setDir("standardScripts", QDir(mainData->getDir("application") + "../kernel/2dx_merge" + "/" + "scripts-standard/"));
    mainData->setDir("customScripts", QDir(mainData->getDir("application") + "../kernel/2dx_merge" + "/" + "scripts-custom/"));
    mainData->setDir("singleParticleScripts", QDir(mainData->getDir("application") + "../kernel/2dx_merge" + "/" + "scripts-singleparticle/"));
    mainData->addImage("appImage", new QImage("resource/icon.png"));

    mainData->addApp("logBrowser", mainData->getDir("application") + "/../" + "bin/" + "2dx_logbrowser");

    mainData->setURL("help", "http://2dx.org/documentation/2dx-software");
    mainData->setURL("bugReport", "https://github.com/C-CINA/2dx/issues");

    if (!setupIcons(mainData, mainData->getDir("resource"))) cerr << "Error loading images." << mainData->getDir("resource").toStdString() << endl;
    
    connect(mainData, SIGNAL(dataModified(bool)), this, SLOT(setSaveState(bool)));
    
    return mainData;
}

void mainWindow::setupScriptModules() 
{
    /**
     * Prepare the Scripts view container
     */
    standardScripts = new scriptModule(mainData, mainData->getDir("standardScripts"), scriptModule::standard);
    connect(standardScripts, SIGNAL(scriptCompleted(QModelIndex)), this, SLOT(standardScriptCompleted(QModelIndex)));
    connect(standardScripts, SIGNAL(reload()), this, SLOT(reload()));
    connect(standardScripts, SIGNAL(progress(int)), this, SLOT(setScriptProgress(int)));
    connect(standardScripts, SIGNAL(incrementProgress(int)), this, SLOT(increaseScriptProgress(int)));

    customScripts = new scriptModule(mainData, mainData->getDir("customScripts"), scriptModule::custom);
    connect(customScripts, SIGNAL(scriptCompleted(QModelIndex)), this, SLOT(customScriptCompleted(QModelIndex)));
    connect(customScripts, SIGNAL(reload()), this, SLOT(reload()));
    connect(customScripts, SIGNAL(progress(int)), this, SLOT(setScriptProgress(int)));
    connect(customScripts, SIGNAL(incrementProgress(int)), this, SLOT(increaseScriptProgress(int)));

    singleParticleScripts = new scriptModule(mainData, mainData->getDir("singleParticleScripts"), scriptModule::singleparticle);
    connect(singleParticleScripts, SIGNAL(scriptCompleted(QModelIndex)), this, SLOT(singleParticleScriptCompleted(QModelIndex)));
    connect(singleParticleScripts, SIGNAL(reload()), this, SLOT(reload()));
    connect(singleParticleScripts, SIGNAL(progress(int)), this, SLOT(setScriptProgress(int)));
    connect(singleParticleScripts, SIGNAL(incrementProgress(int)), this, SLOT(increaseScriptProgress(int)));

    standardScripts->extendSelectionTo(customScripts);
    standardScripts->extendSelectionTo(singleParticleScripts);

    customScripts->extendSelectionTo(standardScripts);
    customScripts->extendSelectionTo(singleParticleScripts);

    singleParticleScripts->extendSelectionTo(standardScripts);
    singleParticleScripts->extendSelectionTo(customScripts);
}


QProgressBar* mainWindow::setupProgressBar() 
{
    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setMaximum(100);
    progressBar->setFixedWidth(300);
    progressBar->setFixedHeight(10);
    progressBar->setValue(0);
    progressBar->setTextVisible(false);
    
    return progressBar;
}

void mainWindow::updateStatusMessage(const QString& message) {
    progressBar->update();
    statusBar->showMessage(message);
}

void mainWindow::increaseScriptProgress(int increament) {
    if (progressBar->value() + increament <= progressBar->maximum())
        progressBar->setValue(progressBar->value() + increament);
    else
        progressBar->setValue(progressBar->maximum());
}

void mainWindow::setScriptProgress(int progress) {
    progressBar->setValue(progress);
}

void mainWindow::execute(bool halt) {
    scriptTab* currWidget = (scriptTab*) scriptsWidget->currentWidget();
    scriptModule* module = currWidget->getModule();
    if (module->type() == scriptModule::standard) {
        standardScripts->execute(halt);
    }
    if (module->type() == scriptModule::custom) {
        customScripts->execute(halt);
    }
    if (module->type() == scriptModule::singleparticle) {
        singleParticleScripts->execute(halt);
    }

}

void mainWindow::setupActions() {
    openAction = new QAction(*(mainData->getIcon("open")), tr("&New"), this);
    openAction->setShortcut(tr("Ctrl+O"));
    connect(openAction, SIGNAL(triggered()), this, SLOT(open()));

    saveAction = new QAction(*(mainData->getIcon("save")), tr("&Save"), this);
    saveAction->setShortcut(tr("Ctrl+S"));
    connect(saveAction, SIGNAL(triggered()), mainData, SLOT(save()));

    importAction = new QAction(*(mainData->getIcon("import")), tr("&Import Images"), this);
    connect(importAction, SIGNAL(triggered()), this, SLOT(import()));

    showImageLibraryAction = new QAction(*(mainData->getIcon("selected")), tr("&Show Image Library"), this);
    showImageLibraryAction->setShortcut(tr("Ctrl+D"));
    showImageLibraryAction->setCheckable(true);
    showImageLibraryAction->setChecked(true);
    connect(showImageLibraryAction, SIGNAL(toggled(bool)), albumCont, SLOT(showContents(bool)));

    viewAlbum = new QAction(*(mainData->getIcon("album")), tr("&Reconstruction album"), this);
    viewAlbum->setShortcut(tr("Ctrl+Shift+A"));
    connect(viewAlbum, SIGNAL(triggered()), this, SLOT(showAlbum()));

    playAction = new QAction(*(mainData->getIcon("play")), tr("&Run selected script(s)"), this);
    playAction->setCheckable(true);
    connect(playAction, SIGNAL(toggled(bool)), this, SLOT(execute(bool)));

    dryRun = new QAction(*(mainData->getIcon("dryRun")), tr("&Dry Run (No changes in config files)"), this);
    dryRun->setCheckable(true);
    dryRun->setChecked(false);
    connect(dryRun, SIGNAL(toggled(bool)), results, SLOT(setDryRunMode(bool)));

    timer_refresh = 10000;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), mainData, SLOT(save()));
    timer->start(timer_refresh);

    refreshAction = new QAction(*(mainData->getIcon("refresh")), tr("&Refresh Results"), this);
    refreshAction->setShortcut(tr("Ctrl+Shift+r"));
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(reload()));

    manualAction = new QAction(*(mainData->getIcon("help")), tr("Hide Manual"), this);
    manualAction->setCheckable(true);
    connect(manualAction, SIGNAL(triggered(bool)), this, SLOT(hideManual(bool)));

}

void mainWindow::setupToolBar() {
    QToolBar* fileToolBar = addToolBar(tr("File"));
    fileToolBar->addAction(openAction);
    fileToolBar->addAction(importAction);
    fileToolBar->addAction(saveAction);

    QToolBar* viewToolBar = addToolBar(tr("View"));
    viewToolBar->addAction(showImageLibraryAction);
    viewToolBar->addAction(viewAlbum);

    QToolBar* actionToolBar = addToolBar(tr("Action"));
    actionToolBar->addAction(playAction);
    actionToolBar->addAction(refreshAction);
    actionToolBar->addAction(dryRun);

    QToolBar* helpToolBar = addToolBar(tr("Help"));
    helpToolBar->addAction(manualAction);
}

void mainWindow::setupMenuBar() {
    /**
     * Setup File menu
     */
    QMenu *fileMenu = new QMenu("File");
    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addAction(importAction);

    QAction *closeAction = new QAction("Quit", this);
    closeAction->setShortcut(tr("Ctrl+Q"));
    connect(closeAction, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    fileMenu->addAction(closeAction);

    /**
     * Setup Edit menu
     */
    QMenu *editMenu = new QMenu("Edit");

    QAction *increaseFontAction = new QAction("Increase Font Size", this);
    increaseFontAction->setShortcut(tr("]"));
    connect(increaseFontAction, SIGNAL(triggered()), this, SLOT(increaseFontSize()));
    editMenu->addAction(increaseFontAction);

    QAction *decreaseFontAction = new QAction("Decrease Font Size", this);
    decreaseFontAction->setShortcut(tr("["));
    connect(decreaseFontAction, SIGNAL(triggered()), this, SLOT(decreaseFontSize()));
    editMenu->addAction(decreaseFontAction);

    /**
     * setup View Menu
     */
    QMenu* viewMenu = new QMenu("View");
    viewMenu->addAction(showImageLibraryAction);
    
    QAction* showSelectedAction = new QAction(tr("&Show checked images only"), this);
    showSelectedAction->setCheckable(true);
    connect(showSelectedAction, SIGNAL(toggled(bool)), albumCont, SLOT(showSelected(bool)));
    viewMenu->addAction(showSelectedAction);
    
    viewMenu->addAction(viewAlbum);

    QAction *viewReproject = new QAction("Show Reproject GUI", this);
    viewReproject->setShortcut(tr("Ctrl+Shift+P"));
    connect(viewReproject, SIGNAL(triggered()), this, SLOT(showReproject()));
    viewMenu->addAction(viewReproject);


    /**
     * Setup Options menu
     */
    QMenu *optionMenu = new QMenu("Options");

    QAction *openPreferencesAction = new QAction("Preferences", this);
    connect(openPreferencesAction, SIGNAL(triggered()), this, SLOT(editHelperConf()));
    optionMenu->addAction(openPreferencesAction);

    QAction *showAutoSaveAction = new QAction("Autosave On/Off", this);
    connect(showAutoSaveAction, SIGNAL(triggered()), this, SLOT(toggleAutoSave()));
    optionMenu->addAction(showAutoSaveAction);

    /**
     * Setup Actions menu 
     */
    QMenu *actionMenu = new QMenu("Action");
    actionMenu->addAction(playAction);
    actionMenu->addAction(refreshAction);
    actionMenu->addAction(dryRun);

    /**
     * Setup select menu
     */
    QMenu *selectMenu = new QMenu("Select");

    QAction *selectAllAction = new QAction("Select All", this);
    selectAllAction->setShortcut(tr("Ctrl+A"));
    selectMenu->addAction(selectAllAction);
    connect(selectAllAction, SIGNAL(triggered()), albumCont->getDirModel(), SLOT(selectAll()));

    QAction *invertSelectedAction = new QAction("Invert Selection", this);
    invertSelectedAction->setShortcut(tr("Ctrl+I"));
    selectMenu->addAction(invertSelectedAction);
    connect(invertSelectedAction, SIGNAL(triggered()), albumCont->getDirModel(), SLOT(invertSelection()));

    QAction *saveDirectorySelectionAction = new QAction("Save Selection As...", this);
    connect(saveDirectorySelectionAction, SIGNAL(triggered()), this, SLOT(saveDirectorySelection()));
    selectMenu->addAction(saveDirectorySelectionAction);

    QAction *loadDirectorySelectionAction = new QAction("Load Selection...", this);
    connect(loadDirectorySelectionAction, SIGNAL(triggered()), this, SLOT(loadDirectorySelection()));
    selectMenu->addAction(loadDirectorySelectionAction);

    /**
     * Setup Help menu
     */
    QMenu *helpMenu = new QMenu("Help");

    QSignalMapper *mapper = new QSignalMapper(this);

    QAction *viewOnlineHelp = new QAction(*(mainData->getIcon("manual")), tr("&View Online Help"), this);
    viewOnlineHelp->setCheckable(false);
    connect(viewOnlineHelp, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(viewOnlineHelp, mainData->getURL("help"));
    helpMenu->addAction(viewOnlineHelp);

    QAction* bugReport = new QAction(*(mainData->getIcon("Bug")), tr("&Report Issue/Bug"), this);
    bugReport->setCheckable(false);
    connect(bugReport, SIGNAL(triggered()), mapper, SLOT(map()));
    mapper->setMapping(bugReport, mainData->getURL("bugReport"));
    helpMenu->addAction(bugReport);

    helpMenu->addAction(manualAction);

    connect(mapper, SIGNAL(mapped(const QString &)), this, SLOT(openURL(const QString &)));

    QAction *showUpdatesAction = new QAction("Update...", this);
    connect(showUpdatesAction, SIGNAL(triggered()), updates, SLOT(show()));
    helpMenu->addAction(showUpdatesAction);

    QAction *showAboutAction = new QAction("About", this);
    connect(showAboutAction, SIGNAL(triggered()), about, SLOT(show()));
    helpMenu->addAction(showAboutAction);

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(editMenu);
    menuBar()->addMenu(viewMenu);
    menuBar()->addMenu(actionMenu);
    menuBar()->addMenu(optionMenu);
    menuBar()->addMenu(selectMenu);
    menuBar()->addMenu(helpMenu);
}

QWidget *mainWindow::setupConfView(confData *data) {
    confModel *model = new confModel(data, this);

    QTableView *confView = new QTableView;
    confView->setModel(model);
    confView->setItemDelegate(new confDelegate(data));
    confView->setGridStyle(Qt::NoPen);
    confView->resizeRowsToContents();
    confView->setAlternatingRowColors(true);
    confView->horizontalHeader()->hide();
    confView->verticalHeader()->hide();
    confView->setSelectionMode(QAbstractItemView::NoSelection);
    int width = 0;
    for (int i = 0; i < confView->model()->columnCount(); i++) {
        confView->resizeColumnToContents(i);
        width += confView->columnWidth(i);
    }
    confView->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    return confView;
}

bool mainWindow::setupIcons(confData *data, const QDir &directory) {
    if (!directory.exists()) return false;
    QString entry, label, type;
    QHash<QString, QIcon *> icons;

    foreach(entry, directory.entryList(QStringList() << "*", QDir::Files | QDir::NoDotAndDotDot, QDir::Unsorted)) {
        if (entry.contains(QRegExp(".*\\-..\\.png$"))) {
            label = entry.section(".png", 0, 0).section("-", 0, 0).trimmed().toLower();
            type = entry.section(".png", 0, 0).section("-", 1, 1).trimmed().toLower();
            if (icons[label] == NULL) icons.insert(label, new QIcon);
            if (type == "ad") icons[label]->addPixmap(directory.canonicalPath() + "/" + entry, QIcon::Active, QIcon::On);
            if (type == "id") icons[label]->addPixmap(directory.canonicalPath() + "/" + entry, QIcon::Normal, QIcon::On);
            if (type == "au") icons[label]->addPixmap(directory.canonicalPath() + "/" + entry, QIcon::Active, QIcon::Off);
            if (type == "iu") icons[label]->addPixmap(directory.canonicalPath() + "/" + entry, QIcon::Normal, QIcon::Off);
        } else if (entry.contains(".png", Qt::CaseInsensitive)) {
            label = entry.section(".png", 0, 0).trimmed().toLower();
            icons.insert(label, new QIcon);
            icons[label]->addPixmap(directory.canonicalPath() + "/" + entry);
        }
    }

    QHashIterator<QString, QIcon*> it(icons);
    while (it.hasNext()) {
        it.next();
        data->addIcon(it.key(), it.value());
    }
    return true;
}

void mainWindow::scriptChanged(scriptModule *module, QModelIndex index) {
    int uid = index.data(Qt::UserRole).toUInt();
    updateStatusMessage(module->title(index));

    if (localIndex[uid] == 0 && module->conf(index)->size() != 0) {
        confInterface *local = new confInterface(module->conf(index), "");
        localIndex[uid] = standardScriptsTab->addParameterWidget(local) + 1;
        localIndex[uid] = customScriptsTab->addParameterWidget(local) + 1;
        localIndex[uid] = singleParticleScriptsTab->addParameterWidget(local) + 1;
        //    if(localParameters->widget(localIndex[uid] - 1) == NULL) cerr<<"Something's very wrong here."<<endl;
        //    connect(userLevelButtons,SIGNAL(levelChanged(int)),local,SLOT(setSelectionUserLevel(int)));
    }

    if (manualIndex[uid] == 0 && !module->conf(index)->manual().isEmpty()) {
        manualIndex[uid] = standardScriptsTab->addManualWidget(new confManual(mainData, module->conf(index))) + 1;
        manualIndex[uid] = customScriptsTab->addManualWidget(new confManual(mainData, module->conf(index))) + 1;
        manualIndex[uid] = singleParticleScriptsTab->addManualWidget(new confManual(mainData, module->conf(index))) + 1;
    }

    if (localIndex[uid] - 1 < 0) {
        standardScriptsTab->hideLocalParameters();
        customScriptsTab->hideLocalParameters();
        singleParticleScriptsTab->hideLocalParameters();
    } else if (module->type() == scriptModule::standard) {
        standardScriptsTab->showLocalParameters();
        standardScriptsTab->setParameterIndex(localIndex[uid] - 1);
    } else if (module->type() == scriptModule::custom) {
        customScriptsTab->showLocalParameters();
        customScriptsTab->setParameterIndex(localIndex[uid] - 1);
    } else if (module->type() == scriptModule::singleparticle) {
        singleParticleScriptsTab->showLocalParameters();
        singleParticleScriptsTab->setParameterIndex(localIndex[uid] - 1);
    }

    if (module->type() == scriptModule::standard) {
        standardScriptsTab->setManualIndex(manualIndex[uid] - 1);
        standardScriptsTab->selectPrameters(module->displayedVariables(index));
    }
    if (module->type() == scriptModule::custom) {
        customScriptsTab->setManualIndex(manualIndex[uid] - 1);
        customScriptsTab->selectPrameters(module->displayedVariables(index));
    }
    if (module->type() == scriptModule::singleparticle) {
        singleParticleScriptsTab->setManualIndex(manualIndex[uid] - 1);
        singleParticleScriptsTab->selectPrameters(module->displayedVariables(index));
    }

    if (verbosityControl->level() != 0)
        logViewer->loadLogFile(module->logFile(index));
    else
        logViewer->clear();
    results->load(module->resultsFile(index));
    //  container->restoreSplitterState(1);
}

void mainWindow::standardScriptChanged(QModelIndex index) {
    scriptChanged(standardScripts, index);
}

void mainWindow::customScriptChanged(QModelIndex index) {
    scriptChanged(customScripts, index);
}

void mainWindow::singleParticleScriptChanged(QModelIndex index) {
    scriptChanged(singleParticleScripts, index);
}

void mainWindow::scriptLaunched(scriptModule * /*module*/, QModelIndex /*index*/) {

}

void mainWindow::setSaveState(bool state) {
    if (state == false) {
        saveAction->setChecked(false);
        saveAction->setCheckable(false);
    } else {
        saveAction->setCheckable(true);
        saveAction->setChecked(true);
    }
}

void mainWindow::scriptCompleted(scriptModule *module, QModelIndex index) {
    //  cerr<<"Script completed"<<endl;
    results->load(module->resultsFile(index));
    albumCont->maskResults();
    results->save();
    resultsView->load();
    playAction->setChecked(false);
}

void mainWindow::reload() {
    results->load();
    albumCont->reload();
    if (album != NULL)
    {
        album->reload();
    }
}

void mainWindow::standardScriptCompleted(QModelIndex index) {
    //  cerr<<"Standard ";
    scriptCompleted(standardScripts, index);
}

void mainWindow::customScriptCompleted(QModelIndex index) {
    //  cerr<<"Custom ";
    scriptCompleted(customScripts, index);
}

void mainWindow::singleParticleScriptCompleted(QModelIndex index) {
    //  cerr<<"Single Particle ";
    scriptCompleted(singleParticleScripts, index);
}

void mainWindow::tabChanged(int currentIndex) {
    scriptTab* currentTab = (scriptTab*) scriptsWidget->currentWidget();
    scriptModule* module = currentTab->getModule();
    module->select(module->getSelection()->selection());
}

bool mainWindow::createDir(const QString &dir) {
    QDir directory(dir);
    if (!directory.exists())
        return directory.mkdir(dir);
    return false;
}

void mainWindow::launchAlbum(const QString &path) {

    if (album == NULL && albumCont != NULL) {
        album = new imageAlbum(albumCont->getDirModel());
        connect(albumCont->getDirView()->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), album, SLOT(currentSelectionChanged(const QModelIndex&, const QModelIndex&)));
    }
}

void mainWindow::launchEuler() {

    if (euler == NULL) {
        euler = new eulerWindow(mainData);
        //    album->setModel(sortModel);
        //    album->setSelectionModel(dirView->selectionModel());
    }
}

void mainWindow::launchReproject() {
    if (reproject == NULL) {
        reproject = new reprojectWindow(mainData);
    }
}

void mainWindow::closeEvent(QCloseEvent *event) {
    if (!mainData->isModified())
        event->accept();
    else {
        int choice = QMessageBox::question(this, tr("Confirm Exit"), tr("Data not saved, exit?"), tr("Save && Quit"), tr("Quit Anyway"), QString("Cancel"), 0, 1);
        if (choice == 0) {
            mainData->save();
            event->accept();
        } else if (choice == 1)
            event->accept();
        else if (choice == 2)
            event->ignore();
    }
}

void mainWindow::launchFileBrowser() {
    QString path = QDir::toNativeSeparators(mainData->getDir("working"));
    QDesktopServices::openUrl(QUrl("file:///" + path));
}

void mainWindow::importFiles(const QHash<QString, QHash<QString, QString> > &imageList) {
    QHashIterator<QString, QHash<QString, QString> > it(imageList);
    importCount = imageList.size();
    while (it.hasNext()) {
        it.next();
        qDebug() << "Importing File: " << it.key();
        importFile(it.key(), it.value());
    }
}

void mainWindow::importFile(const QString &file, const QHash<QString, QString> &imageCodes) {
    QHashIterator<QString, QString> it(imageCodes);
    QString fileName = file;
    QString pC = imageCodes["protein_code"];
    QString tiltAngle = imageCodes["tilt_angle"];
    QString frame = imageCodes["frame_number"];
    QString subID = imageCodes["sub_image_number"];
    QString ext = QFileInfo(file).suffix();

    qDebug() << "pC=" << pC << "  tiltAngle=" << tiltAngle << "  frame=" << frame << "  subID=" << subID << "  ext=" << ext;

    QDir tiltDir(mainData->getDir("project") + "/" + pC + tiltAngle);
    QString newFile = pC + tiltAngle + frame + subID;
    QString tiltDirectory = pC + tiltAngle;
    QString tiltConfigLocation = mainData->getDir("project") + "/" + tiltDirectory + "/2dx_master.cfg";
    if (!tiltDir.exists()) {
        qDebug() << pC + tiltAngle << " does not exist...creating.";
        tiltDir.setPath(mainData->getDir("project"));
        tiltDir.mkdir(tiltDirectory);
        //confData tiltData(tiltConfigLocation);
        confData tiltData(mainData->getDir("project") + "/" + tiltDirectory + "/2dx_master.cfg", mainData->getDir("project") + "/2dx_master.cfg");
        tiltData.save();
        tiltData.setSymLink("../2dx_master.cfg", mainData->getDir("project") + "/" + tiltDirectory + "/2dx_master.cfg");
    }

    tiltDir.setPath(mainData->getDir("project") + "/" + tiltDirectory);
    tiltDir.mkdir(newFile);

    QFile::copy(fileName, tiltDir.path() + "/" + newFile + "/" + newFile + '.' + ext);
    QString newFilePath = tiltDir.path() + "/" + newFile;
    QString newFileConfigPath = newFilePath + "/2dx_image.cfg";
    if (!QFileInfo(newFileConfigPath).exists()) {
        //HENN>
        qDebug() << "Copying " << tiltConfigLocation << " to " << newFileConfigPath;
        if (!QFile::copy(tiltConfigLocation, newFileConfigPath))
            qDebug() << "Failed to copy " << tiltConfigLocation << " to " << newFileConfigPath;
        else {
            QFileInfo oldFile(fileName);
            QString name = oldFile.fileName();
            QString oldFileDir = oldFile.absolutePath();
            QString oldStackName = oldFileDir + "/../aligned_stacks/" + name;
            QFileInfo oldStack(oldStackName);
            if (!oldStack.exists()) {
                // qDebug() << "Stack " << oldStackName << " not found. Trying ";
                oldStackName = oldFileDir + "/../DC_stacks/" + name;
                // qDebug() << oldStackName;
            }

            QString newStackName = newFilePath + "/" + newFile + "_stack." + ext;
            // qDebug() << "oldStackName = " << oldStackName << ", newStackName = " << newStackName;

            QFile f(newFileConfigPath);
            if (f.open(QIODevice::Append | QIODevice::Text)) {
                // qDebug() << "Apending imagename_original = " << name << " to 2dx_image.cfg file.";
                QTextStream stream(&f);
                // qDebug() << "set imagename = " << newFile;
                stream << "set imagename = " << newFile << endl;
                // qDebug() << "set imagenumber = " << frame+ subID;
                stream << "set imagenumber = " << frame + subID << endl;
                // qDebug() << "set nonmaskimagename = " << newFile;
                stream << "set nonmaskimagename = " << newFile << endl;
                // qDebug() << "set imagename_original = " << name;
                stream << "set imagename_original = " << name << endl;

                if (oldStack.exists()) {
                    qDebug() << "Copying " << oldStackName << " to " << newStackName;
                    stream << "set movie_stackname = " << newFile + "_stack." + ext << endl;
                    if (!QFile::copy(oldStackName, newStackName)) {
                        qDebug() << "ERROR when trying to copy stack." << endl;
                    }
                }
                f.close();
            }
        }
        //HENN<
    }
    importProcess.start(mainData->getApp("2dx_image") + " " + newFilePath + " " + "\"2dx_initialize\"");
    importProcess.waitForFinished(8 * 60 * 60 * 1000);
}

void mainWindow::importFinished() {
    importCount--;
    if (importCount <= 0) albumCont->updateModel();
}

void mainWindow::import() {
    QStringList fileList = QFileDialog::getOpenFileNames(NULL, "Choose image files to add", mainData->getDir("project"), "Images (*.tif *.mrc)");
    if (fileList.isEmpty()) return;
    QStringList importList;
    //  foreach(file, fileList)
    //  {
    //    if(QFileInfo(file).isDir()) 
    //      importList<<subDirFiles(file);
    //    else
    //      importList<<file;

    //  }
    importTool *import = new importTool(mainData, fileList);
    connect(import, SIGNAL(acceptedImages(const QHash< QString, QHash < QString, QString > >&)), this, SLOT(importFiles(const QHash<QString, QHash<QString, QString> > &)));
}

void mainWindow::autoImport() {
    if (!autoImportMonitor) {
        QString dir = QFileDialog::getExistingDirectory(NULL, "Choose import directory", mainData->getDir("project"));
        autoImportMonitor = new autoImportTool(mainData, dir);
    } else
        autoImportMonitor->show();
}

void mainWindow::open() {
    QProcess::startDetached(mainData->getApp("this"));
}

void mainWindow::openURL(const QString &url) {
    QProcess::startDetached(mainData->getApp("webBrowser") + " " + url);
}

void mainWindow::hideManual(bool hide) {
    if (!hide) {
        standardScriptsTab->showManual();
        customScriptsTab->showManual();
        singleParticleScriptsTab->showManual();
    } else {
        standardScriptsTab->hideManual();
        customScriptsTab->hideManual();
        singleParticleScriptsTab->hideManual();
    }
}

void mainWindow::increaseFontSize() {
    QFont font = QApplication::font();
    font.setPointSize(font.pointSize() + 1);
    mainData->userConf()->set("fontSize", QString::number(font.pointSize()));
    mainData->userConf()->save();
    QApplication::setFont(font);
    updateFontInfo();
}

void mainWindow::decreaseFontSize() {
    QFont font = QApplication::font();
    font.setPointSize(font.pointSize() - 1);
    mainData->userConf()->set("fontSize", QString::number(font.pointSize()));
    mainData->userConf()->save();
    QApplication::setFont(font);
    updateFontInfo();
}

void mainWindow::toggleAutoSave() {
    m_do_autosave = !m_do_autosave;

    if (m_do_autosave) {
        QMessageBox::information(NULL, tr("Automatic Saving"), tr("Automatic Saving is now switched on"));
        timer->start(timer_refresh);
    } else {
        QMessageBox::information(NULL, tr("Automatic Saving"), tr("Automatic Saving is now switched off"));
        timer->stop();
    }
}

void mainWindow::updateFontInfo() {
    standardScriptsTab->updateFontInfo();
    customScriptsTab->updateFontInfo();
    singleParticleScriptsTab->updateFontInfo();
    logViewer->updateFontInfo();
    //  emit fontInfoUpdated();
}

void mainWindow::saveDirectorySelection() {
    QString saveName = QFileDialog::getSaveFileName(this, "Save Selection As...", mainData->getDir("working") + "/2dx_merge_dirfile.dat");
    if (QFileInfo(saveName).exists()) QFile::remove(saveName);
    QFile::copy(mainData->getDir("working") + "/2dx_merge_dirfile.dat", saveName);
}

void mainWindow::loadDirectorySelection() {
    QString loadName = QFileDialog::getOpenFileName(this, "Save Selection As...", mainData->getDir("working") + "/2dx_merge_dirfile.dat");
    albumCont->loadSelection(loadName);
}

void mainWindow::showAlbum(bool show) {
    if (album == NULL)
        launchAlbum(mainData->getDir("project"));

    album->setHidden(!show);
}

void mainWindow::showEuler(bool show) {
    if (euler == NULL)
        launchEuler();

    euler->setHidden(!show);
}

void mainWindow::showReproject(bool show) {
    if (reproject == NULL)
        launchReproject();

    reproject->setHidden(!show);
}

void mainWindow::editHelperConf() {
    new confEditor(mainData->getSubConf("appConf"));
}

void mainWindow::launchLogBrowser() {
    QProcess::startDetached(mainData->getApp("logBrowser") + " " + logViewer->getLogFile());
}
