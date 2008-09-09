/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/


#include "nui.h"
#include "nuiDialog.h"
#include "nuiFileSelector.h"
#include "nuiEditLine.h"

#pragma once

class nuiDialogSelectFile : public nuiDialog
  {
  public:
    nuiDialogSelectFile(nuiMainWindow* pParent, const nglString& rTitle, const nglPath& rPath = nglPath(nglString::Null), const nglPath& rRootPath  = nglPath(nglString::Null), const nglString& defaultEntry = nglString::Null, const nglString& rFilter = _T("*"), bool showHiddenFiles = false);
    nuiDialogSelectFile(nuiMainWindow* pParent, const nglString& rTitle, const nglPath& rPath = nglPath(nglString::Null), const nglPath& rRootPath  = nglPath(nglString::Null), const nglString& defaultEntry = nglString::Null, const std::list<nglString>& rFilters = std::list<nglString>(), bool showHiddenFiles = false);
    virtual ~nuiDialogSelectFile();
    
    bool OnCreateNewFolder(const nuiEvent& rEvent);
    bool OnCreateNewFolderDone(const nuiEvent& rEvent);
    
    const nglPath& GetSelectedFile();
    const nglPath& GetRootPath();
    
    nuiSimpleEventSource<0> FileSelected;
    
  private:
    
    void Init(nuiMainWindow* pParent, const nglString& rTitle, const nglPath& rPath, const nglPath& rRootPath, const nglString& defaultEntry, const std::list<nglString>& rFilters, bool showHiddenFiles);

    bool OnSelectorOK(const nuiEvent& rEvent);
    bool OnDialogDone(const nuiEvent& rEvent);
    
    nuiMainWindow* mpParent;
    nuiSimpleContainer* mpContainer;
    nuiFileSelector* mpSelector;
    nuiEditLine* mpEditLine;
    
    nuiDialog* mpCreateDialog;
    nuiEditLine* mpCreateEditLine;
    
    nglPath mPath;
    nglPath mRootPath;
    std::list<nglString> mFilters;
    
    nuiEventSink<nuiDialogSelectFile> mEventSink;
  };
