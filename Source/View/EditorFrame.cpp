/*
 Copyright (C) 2010-2012 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "EditorFrame.h"

#include "Model/MapDocument.h"
#include "Utility/Console.h"
#include "View/EditorView.h"
#include "View/Inspector.h"
#include "View/MapGLCanvas.h"
#include "View/CommandIds.h"
#include "TrenchBroomApp.h"

#include <wx/colour.h>
#include <wx/config.h>
#include <wx/docview.h>
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>

namespace TrenchBroom {
    namespace View {
        BEGIN_EVENT_TABLE(EditorFrame, wxFrame)
		EVT_CLOSE(EditorFrame::OnClose)
		END_EVENT_TABLE()

        void EditorFrame::CreateGui(Model::MapDocument& document, EditorView& view) {
            wxSplitterWindow* logSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE);
            logSplitter->SetSashGravity(1.0f);
            logSplitter->SetMinimumPaneSize(0);
            
            wxSplitterWindow* inspectorSplitter = new wxSplitterWindow(logSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3DSASH | wxSP_LIVE_UPDATE);
            inspectorSplitter->SetSashGravity(1.0f);
            inspectorSplitter->SetMinimumPaneSize(350);
            
            m_logView = new wxTextCtrl(logSplitter, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
            m_logView->SetDefaultStyle(wxTextAttr(*wxLIGHT_GREY, *wxBLACK));
            m_logView->SetBackgroundColour(*wxBLACK);
            
            m_mapCanvas = new MapGLCanvas(inspectorSplitter, document, view);
            Inspector* inspector = new Inspector(inspectorSplitter, document, view);
            
            inspectorSplitter->SplitVertically(m_mapCanvas, inspector, 0);
            logSplitter->SplitHorizontally(inspectorSplitter, m_logView);
            
            wxSizer* logSplitterSizer = new wxBoxSizer(wxVERTICAL);
            logSplitterSizer->Add(logSplitter, 1, wxEXPAND);
            SetSizer(logSplitterSizer);
            
            SetSize(800, 600);
            inspectorSplitter->SetSashPosition(GetSize().x - 350);
            logSplitter->SetSashPosition(GetSize().y - 150);
            Layout();
        }
        
        EditorFrame::EditorFrame(Model::MapDocument& document, EditorView& view) :
        wxFrame(NULL, wxID_ANY, wxT("TrenchBroom")),
        m_document(document),
        m_view(view) {
            CreateGui(document, view);

            wxMenuBar* menuBar = static_cast<TrenchBroomApp*>(wxTheApp)->CreateMenuBar(&view);
            SetMenuBar(menuBar);
        }

        void EditorFrame::OnClose(wxCloseEvent& event) {
            // if the user closes the editor frame, the document must also be closed:
            m_document.GetDocumentManager()->CloseDocument(&m_document);
        }
    }
}