#ifndef LIBRARYCONTAINER_H
#define	LIBRARYCONTAINER_H

#include <QWidget>
#include <QTreeView>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTimer>
#include <QComboBox>
#include <QStackedWidget>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QToolBar>
#include <QToolButton>
#include <QLineEdit>
#include <QCheckBox>

#include "BlockContainer.h"
#include "ProjectModel.h"
#include "ImageViewer.h"
#include "ParameterConfiguration.h"
#include "ProjectDelegate.h"
#include "LibraryImageStatus.h"

class LibraryTab : public QWidget
{
    Q_OBJECT
           
    public:
        LibraryTab(QWidget* parent=NULL);
        
        ProjectModel* getDirModel();
        QTreeView* getDirView();
    
    public slots:
        void showContents(bool show);
        
        void showSelected(bool enable);
        
        void reload();
        void updateModel();
        void columnActivated(int i);
        void copyImage();
        
        void import();
        
        void autoSelect();
        void extendSelection();
        void reduceSelection();
        void addImageFolder();
        void addImageFolder(const QString& folder);
        void renameImageFolder();
        void moveSelectiontoFolder();
        void moveSelectionToFolder(const QString& targetPath);
        void flagSelection(const QString& color);
        void trashSelection();
        bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath);
        
        void saveProjectState();
        void loadProjectState();
        
        void saveDirectorySelection();
        void loadDirectorySelection();
        
        void setPreviewImages(const QString&);
        void autoSwitch(bool);
        void updatePreview();
        
        void resetSelectionState();
                   
    private:
        void setupDirectoryContainer();
        QToolBar* setupLibraryControls();
        QWidget* setupAutoSelectionTool();
        QWidget* setupPreviewContainer();
        
        QAction* getLibraryToolBarAction(const QString& ic, const QString& tooltip, const QString& shortcut, bool checkable);
        
        void modifySelection(bool select = true);
        void setPreviewLabelText();
        
        QTreeView* dirView;
        ProjectModel* dirModel;
        QSortFilterProxyModel *sortModel;
        
        QLabel* selectionState;
        QLabel* previewLabel;
        
        QToolButton* showHeaderButton;
               
        ImageViewer* mapPreview;
        ImageViewer* refPreview;
        ImageViewer* dualPreview;
        
        QStackedWidget* previews;
        
        QWidget* previewContainer;
        QWidget* autoSelectContainer;
        LibraryImageStatus* imageStatus;
        
        QTimer* previewTimer;
        
        QLineEdit* minDegree;
        QLineEdit* maxDegree;
        QComboBox* parameterToUse;
        QCheckBox* negPosOption;  
        
        QCheckBox* noFlagged;
        QCheckBox* redFlagged;
        QCheckBox* greenFlagged;
        QCheckBox* blueFlagged;
        QCheckBox* goldFlagged;
};


#endif	/* ALBUMCONTAINER_H */

