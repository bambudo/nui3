/*
  NUI3 - C++ cross-platform GUI framework for OpenGL based applications
  Copyright (C) 2002-2003 Sebastien Metrot

  licence: see nui3/LICENCE.TXT
*/

#include "nui.h"
#include "nuiDialogSelectFile.h"
#include "nuiVBox.h"
#include "nuiHBox.h"


nuiDialogSelectFile::nuiDialogSelectFile(nuiMainWindow* pParent, const nglString& rTitle, const nglPath& rPath, const nglPath& rRootPath, const nglString& defaultEntry, const nglString& rFilter, bool showHiddenFiles)
: nuiDialog(pParent), mpParent(pParent), mPath(rPath), mEventSink(this)
{
  std::list<nglString> filters;
  filters.push_back(rFilter);
  Init(pParent, rTitle, rPath, rRootPath, defaultEntry, filters, showHiddenFiles);
}

nuiDialogSelectFile::nuiDialogSelectFile(nuiMainWindow* pParent, const nglString& rTitle, const nglPath& rPath, const nglPath& rRootPath, const nglString& defaultEntry, const std::list<nglString>& rFilters, bool showHiddenFiles)
: nuiDialog(pParent), mpParent(pParent), mPath(rPath), mEventSink(this)
{
  Init(pParent, rTitle, rPath, rRootPath, defaultEntry, rFilters, showHiddenFiles);
}


void nuiDialogSelectFile::Init(nuiMainWindow* pParent, const nglString& rTitle, const nglPath& rPath, const nglPath& rRootPath, const nglString& defaultEntry, const std::list<nglString>& rFilters, bool showHiddenFiles)
{
  mPath = rPath;
  mRootPath = rRootPath;

  nuiSimpleContainer* pEditContainer = new nuiSimpleContainer();
  
  mpEditLine = new nuiEditLine();
  mpEditLine->SetObjectName(_T("nuiDialog::EditLine"));
  mpEditLine->SetColor(eNormalTextFg, nuiColor(64,64,64));
  mpEditLine->SetColor(eSelectedTextFg, nuiColor(64,64,64));
  pEditContainer->AddChild(mpEditLine);
	
  mpContainer = new nuiSimpleContainer();
  
  
  mpSelector = new nuiFileSelector(rPath, rRootPath, rFilters, mpEditLine);
  
  mEventSink.Connect(mpSelector->OK, &nuiDialogSelectFile::OnSelectorOK);
  
  nuiVBox* pBox = new nuiVBox();
  mpContainer->AddChild(mpSelector);
  pBox->AddCell(mpContainer);
  pBox->AddCell(pEditContainer);
  
  mpContainer->SetUserSize(mpParent->GetWidth() * .8, mpParent->GetHeight() * .8);
  
  pEditContainer->SetUserSize(mpParent->GetWidth() * .8, 20);
  pEditContainer->SetBorder(10, 10);
  
  pBox->SetCellExpand(0, nuiExpandShrinkAndGrow);
  pBox->SetCellExpand(1, nuiExpandFixed);
  

  
  nuiLabel* pTitle = new nuiLabel(rTitle);
  pTitle->SetObjectName(_T("nuiDialog::Title"));
    
  InitDialog(pTitle, NULL, nuiDialog::eDialogButtonOk | nuiDialog::eDialogButtonCancel);
    
  nuiButton* pButton = new nuiButton(_T("New Folder"));
  pButton->SetObjectName(_T("nuiDialog::Button"));
  AddButton(pButton, nuiDialog::eDialogButtonCustom);
  mEventSink.Connect(pButton->Activated, &nuiDialogSelectFile::OnCreateNewFolder);
  
  SetContents(pBox, nuiCenter);
  SetDefaultPos();
  mEventSink.Connect(DialogDone, &nuiDialogSelectFile::OnDialogDone);
  
  mpSelector->UpdateLayout();

  mpEditLine->SetText(defaultEntry);
}



nuiDialogSelectFile::~nuiDialogSelectFile()
{
  
}



bool nuiDialogSelectFile::OnSelectorOK(const nuiEvent& rEvent)
{
//  nglString text = mpEditLine->GetText();
//  text.Trim();
//  if (text == nglString::Null)
//    return false;
  
  mPath = mpSelector->GetPath();
  mRootPath = mpSelector->GetRootPath();
  
  // send event and close the dialog if the user answered true
  if (FileSelected())
    Trash();
  
  return false;
}
bool nuiDialogSelectFile::OnCreateNewFolder(const nuiEvent& rEvent)
{
  mpCreateDialog = new nuiDialog(mpParent);
  nuiSimpleContainer* pContainer = new nuiSimpleContainer();
  pContainer->SetUserSize(400, 80);
  
  mpCreateEditLine = new nuiEditLine();
  mpCreateEditLine->SetPosition(nuiFillHorizontal);
  pContainer->AddChild(mpCreateEditLine);
  
  mpCreateDialog->InitDialog(_T("CREATE A NEW FOLDER"), NULL, nuiDialog::eDialogButtonOk + nuiDialog::eDialogButtonCancel);
  
  mpCreateDialog->SetContents(pContainer);
  mpCreateDialog->SetDefaultPos();
  mEventSink.Connect(mpCreateDialog->DialogDone, &nuiDialogSelectFile::OnCreateNewFolderDone);
  
  mpCreateEditLine->Focus();
  
  NGL_OUT(_T("new folder \n"));
  
  return true;
}


bool nuiDialogSelectFile::OnCreateNewFolderDone(const nuiEvent& rEvent)
{
  
  nuiDialog::DialogResult result = mpCreateDialog->GetResult();
  
  if (result == nuiDialog::eDialogAccepted)
  {
    nglString text = mpCreateEditLine->GetText();
    text.Trim();
    if (text == nglString::Null)
      return true;
    
    mPath = mpSelector->GetPath();
    mPath += nglPath(mpCreateEditLine->GetText());
    mPath.Create();
    
    mRootPath = mpSelector->GetRootPath();
    
    mpSelector->SetRootPath(mRootPath);
  }  
  return false;
}

bool nuiDialogSelectFile::OnDialogDone(const nuiEvent& rEvent)
{
  nuiDialog::DialogResult result = GetResult();
  
  if (result == nuiDialog::eDialogAccepted)
  {
    bool res = !OnSelectorOK(rEvent);
    return res;
  }
  
  return false;
}


const nglPath& nuiDialogSelectFile::GetSelectedFile()
{
  return mPath;
}

const nglPath& nuiDialogSelectFile::GetRootPath()
{
  return mRootPath;
}

