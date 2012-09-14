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

#include "EditorView.h"

#include "Controller/CameraEvent.h"
#include "Controller/Command.h"
#include "Controller/ChangeEditStateCommand.h"
#include "Model/Brush.h"
#include "Model/Entity.h"
#include "Model/EntityDefinition.h"
#include "Model/Face.h"
#include "Model/Filter.h"
#include "Model/MapDocument.h"
#include "Model/Map.h"
#include "Renderer/Camera.h"
#include "Renderer/MapRenderer.h"
#include "Utility/Console.h"
#include "Utility/Preferences.h"
#include "View/EditorFrame.h"
#include "View/MapGLCanvas.h"
#include "View/CommandIds.h"
#include "View/ViewOptions.h"

namespace TrenchBroom {
    namespace View {
        BEGIN_EVENT_TABLE(EditorView, wxView)
        EVT_CAMERA_MOVE(EditorView::OnCameraMove)
        EVT_CAMERA_LOOK(EditorView::OnCameraLook)
        EVT_CAMERA_ORBIT(EditorView::OnCameraOrbit)

        EVT_MENU(CommandIds::Menu::EditSelectAll, EditorView::OnEditSelectAll)
        EVT_MENU(CommandIds::Menu::EditSelectNone, EditorView::OnEditSelectNone)
        
        EVT_MENU(CommandIds::Menu::EditHideSelected, EditorView::OnEditHideSelected)
        EVT_MENU(CommandIds::Menu::EditHideUnselected, EditorView::OnEditHideUnselected)
        EVT_MENU(CommandIds::Menu::EditUnhideAll, EditorView::OnEditUnhideAll)
        
        EVT_MENU(CommandIds::Menu::EditLockSelected, EditorView::OnEditLockSelected)
        EVT_MENU(CommandIds::Menu::EditLockUnselected, EditorView::OnEditLockUnselected)
        EVT_MENU(CommandIds::Menu::EditUnlockAll, EditorView::OnEditUnlockAll)

        EVT_UPDATE_UI_RANGE(CommandIds::Menu::Lowest, CommandIds::Menu::Highest, EditorView::OnUpdateMenuItem)
        END_EVENT_TABLE()

        IMPLEMENT_DYNAMIC_CLASS(EditorView, wxView);
        
        EditorView::EditorView() :
        wxView(),
        m_camera(NULL),
        m_renderer(NULL),
        m_filter(NULL),
        m_viewOptions(NULL) {}
        
        ViewOptions& EditorView::viewOptions() const {
            return *m_viewOptions;
        }

        Model::Filter& EditorView::Filter() const {
            return *m_filter;
        }
        
        Model::MapDocument& EditorView::MapDocument() const {
            return *static_cast<Model::MapDocument*>(GetDocument());
        }

        Renderer::Camera& EditorView::Camera() const {
            return *m_camera;
        }
        
        Renderer::MapRenderer& EditorView::Renderer() const {
            return *m_renderer;
        }
        
        Utility::Console& EditorView::Console() const {
            return *m_console;
        }
        
        void EditorView::Submit(wxCommand* command) {
            MapDocument().GetCommandProcessor()->Submit(command);
        }

        bool EditorView::OnCreate(wxDocument* doc, long flags) {
            m_console = new Utility::Console();
            m_viewOptions = new ViewOptions();
            m_filter = new Model::DefaultFilter(*m_viewOptions);
//            m_filter->setPattern("light");
            
            Preferences::PreferenceManager& prefs = Preferences::PreferenceManager::preferences();
            float fieldOfVision = prefs.getFloat(Preferences::CameraFieldOfVision);
            float nearPlane = prefs.getFloat(Preferences::CameraNearPlane);
            float farPlane = prefs.getFloat(Preferences::CameraFarPlane);
            Vec3f position(0.0f, 0.0f, 0.0f);
            Vec3f direction(1.0f, 0.0f, 0.0f);
            m_camera = new Renderer::Camera(fieldOfVision, nearPlane, farPlane, position, direction);
            
            Model::MapDocument& document = *static_cast<Model::MapDocument*>(doc);
            m_renderer = new Renderer::MapRenderer(document);
            
            EditorFrame* frame = new EditorFrame(document, *this);
            m_console->setTextCtrl(frame->LogView());

            SetFrame(frame);
            frame->Show();

            
            return wxView::OnCreate(doc, flags);
        }
        
        void EditorView::OnUpdate(wxView* sender, wxObject* hint) {
            if (hint != NULL) {
                Controller::Command* command = static_cast<Controller::Command*>(hint);
                switch (command->type()) {
                    case Controller::Command::LoadMap:
                        m_renderer->loadMap();
                        break;
                    case Controller::Command::ClearMap:
                        m_renderer->clearMap();
                        break;
                    case Controller::Command::ChangeEditState: {
                        Controller::ChangeEditStateCommand* changeEditStateCommand = static_cast<Controller::ChangeEditStateCommand*>(command);
                        m_renderer->changeEditState(changeEditStateCommand->changeSet());
                        break;
                    }
                    case Controller::Command::InvalidateRendererEntityState:
                        m_renderer->invalidateEntities();
                        break;
                    case Controller::Command::InvalidateRendererBrushState:
                        m_renderer->invalidateBrushes();
                        break;
                    case Controller::Command::InvalidateRendererState:
                        m_renderer->invalidateAll();
                        break;
                    case Controller::Command::InvalidateEntityRendererCache:
                        m_renderer->invalidateEntityRendererCache();
                        break;
                    default:
                        break;
                }
            }

            EditorFrame* frame = static_cast<EditorFrame*>(GetFrame());
            if (frame != NULL)
                frame->MapCanvas().Refresh();
        }
        
        void EditorView::OnDraw(wxDC* dc) {
        }
        
        bool EditorView::OnClose(bool deleteWindow) {
            if (!wxView::OnClose(deleteWindow))
                return false;
            
            if (deleteWindow) {
                EditorFrame* frame = static_cast<EditorFrame*>(GetFrame());
                if (frame != NULL)
                    wxDELETE(frame);
            }

            if (m_filter != NULL) {
                delete m_filter;
                m_filter = NULL;
            }
            if (m_viewOptions != NULL) {
                delete m_viewOptions;
                m_viewOptions = NULL;
            }
            if (m_camera != NULL) {
                delete m_camera;
                m_camera = NULL;
            }
            if (m_renderer != NULL) {
                delete m_renderer;
                m_renderer = NULL;
            }
            return true;
        }

        void EditorView::OnCameraMove(Controller::CameraMoveEvent& event) {
            m_camera->moveBy(event.forward(), event.right(), event.up());
            OnUpdate(this);
        }
        
        void EditorView::OnCameraLook(Controller::CameraLookEvent& event) {
            m_camera->rotate(event.hAngle(), event.vAngle());
            OnUpdate(this);
        }
        
        void EditorView::OnCameraOrbit(Controller::CameraOrbitEvent& event) {
            m_camera->orbit(event.center(), event.hAngle(), event.vAngle());
            OnUpdate(this);
        }
        
        void EditorView::OnEditSelectAll(wxCommandEvent& event) {
            const Model::EntityList& entities = MapDocument().Map().entities();
            Model::EntityList selectEntities;
            Model::BrushList selectBrushes;
            
            for (unsigned int i = 0; i < entities.size(); i++) {
                Model::Entity& entity = *entities[i];
                if (entity.selectable() && m_filter->entityVisible(entity)) {
                    selectEntities.push_back(&entity);
                } else {
                    const Model::BrushList& entityBrushes = entity.brushes();
                    for (unsigned int j = 0; j < entityBrushes.size(); j++) {
                        Model::Brush& brush = *entityBrushes[j];
                        if (m_filter->brushVisible(brush))
                            selectBrushes.push_back(&brush);
                    }
                }
            }
    
            wxCommand* command = Controller::ChangeEditStateCommand::replace(MapDocument(), selectEntities, selectBrushes);
            Submit(command);
        }

        void EditorView::OnEditSelectNone(wxCommandEvent& event) {
            wxCommand* command = Controller::ChangeEditStateCommand::deselectAll(MapDocument());
            Submit(command);
        }
        
        void EditorView::OnEditHideSelected(wxCommandEvent& event) {
            Model::EditStateManager& editStateManager = MapDocument().EditStateManager();
            const Model::EntityList& hideEntities = editStateManager.selectedEntities();
            const Model::BrushList& hideBrushes = editStateManager.selectedBrushes();
            
            wxCommand* command = Controller::ChangeEditStateCommand::hide(MapDocument(), hideEntities, hideBrushes);
            Submit(command);
        }
        
        void EditorView::OnEditHideUnselected(wxCommandEvent& event) {
            const Model::EntityList& entities = MapDocument().Map().entities();
            Model::EntityList hideEntities;
            Model::BrushList hideBrushes;
            
            for (unsigned int i = 0; i < entities.size(); i++) {
                Model::Entity& entity = *entities[i];
                if (!entity.selected() && entity.hideable()) {
                    hideEntities.push_back(&entity);

                    const Model::BrushList& entityBrushes = entity.brushes();
                    for (unsigned int j = 0; j < entityBrushes.size(); j++) {
                        Model::Brush& brush = *entityBrushes[j];
                        if (!brush.selected() && brush.hideable())
                            hideBrushes.push_back(&brush);
                    }
                }
            }
            
            wxCommand* command = Controller::ChangeEditStateCommand::hide(MapDocument(), hideEntities, hideBrushes);
            Submit(command);
        }
        
        void EditorView::OnEditUnhideAll(wxCommandEvent& event) {
            wxCommand* command = Controller::ChangeEditStateCommand::unhideAll(MapDocument());
            Submit(command);
        }
        
        void EditorView::OnEditLockSelected(wxCommandEvent& event) {
            Model::EditStateManager& editStateManager = MapDocument().EditStateManager();
            const Model::EntityList& lockEntities = editStateManager.selectedEntities();
            const Model::BrushList& lockBrushes = editStateManager.selectedBrushes();
            
            wxCommand* command = Controller::ChangeEditStateCommand::lock(MapDocument(), lockEntities, lockBrushes);
            Submit(command);
        }
        
        void EditorView::OnEditLockUnselected(wxCommandEvent& event) {
            const Model::EntityList& entities = MapDocument().Map().entities();
            Model::EntityList lockEntities;
            Model::BrushList lockBrushes;
            
            for (unsigned int i = 0; i < entities.size(); i++) {
                Model::Entity& entity = *entities[i];
                if (!entity.selected() && entity.lockable()) {
                    lockEntities.push_back(&entity);
                    
                    const Model::BrushList& entityBrushes = entity.brushes();
                    for (unsigned int j = 0; j < entityBrushes.size(); j++) {
                        Model::Brush& brush = *entityBrushes[j];
                        if (!brush.selected() && brush.lockable())
                            lockBrushes.push_back(&brush);
                    }
                }
            }
            
            wxCommand* command = Controller::ChangeEditStateCommand::lock(MapDocument(), lockEntities, lockBrushes);
            Submit(command);
        }
        
        void EditorView::OnEditUnlockAll(wxCommandEvent& event) {
            wxCommand* command = Controller::ChangeEditStateCommand::unlockAll(MapDocument());
            Submit(command);
        }

        void EditorView::OnUpdateMenuItem(wxUpdateUIEvent& event) {
            Model::EditStateManager& editStateManager = MapDocument().EditStateManager();
            switch (event.GetId()) {
                case CommandIds::Menu::EditSelectAll:
                    event.Enable(true);
                    break;
                case CommandIds::Menu::EditSelectSiblings:
                    event.Enable(false);
                    break;
                case CommandIds::Menu::EditSelectTouching:
                    event.Enable(false);
                    break;
                case CommandIds::Menu::EditSelectNone:
                    event.Enable(editStateManager.selectionMode() != Model::EditStateManager::None);
                    break;
                case CommandIds::Menu::EditHideSelected:
                case CommandIds::Menu::EditHideUnselected:
                    event.Enable(editStateManager.selectionMode() != Model::EditStateManager::None &&
                                 editStateManager.selectionMode() != Model::EditStateManager::Faces);
                    break;
                case CommandIds::Menu::EditUnhideAll:
                    event.Enable(editStateManager.hasHiddenObjects());
                    break;
                case CommandIds::Menu::EditLockSelected:
                case CommandIds::Menu::EditLockUnselected:
                    event.Enable(editStateManager.selectionMode() != Model::EditStateManager::None &&
                                 editStateManager.selectionMode() != Model::EditStateManager::Faces);
                    break;
                case CommandIds::Menu::EditUnlockAll:
                    event.Enable(editStateManager.hasLockedObjects());
                    break;
            }
        }
    }
}