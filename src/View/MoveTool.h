/*
 Copyright (C) 2010-2013 Kristian Duske
 
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
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TrenchBroom__MoveTool__
#define __TrenchBroom__MoveTool__

#include "View/Tool.h"

#include "TrenchBroom.h"
#include "VecMath.h"
#include "StringUtils.h"
#include "Renderer/RenderContext.h"
#include "View/ControllerFacade.h"
#include "View/Grid.h"
#include "View/InputState.h"
#include "View/MapDocument.h"
#include "View/MovementRestriction.h"
#include "View/ViewTypes.h"

namespace TrenchBroom {
    namespace View {
        class ControllerFacade;
        
        template <class ActivationPolicyType, class PickingPolicyType, class MousePolicyType, class RenderPolicyType>
        class MoveTool : public Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType> {
        public:
            typedef enum {
                Conclude,
                Deny,
                Continue
            } MoveResult;
        private:
            MovementRestriction& m_movementRestriction;
        public:
            MoveTool(BaseTool* next, MapDocumentPtr document, ControllerFacade& controller, MovementRestriction& movementRestriction) :
            Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType>(next, document, controller),
            m_movementRestriction(movementRestriction) {}
        private:
            void doModifierKeyChange(const InputState& inputState) {
                if (!handleEvent(inputState))
                    return;
                
                m_movementRestriction.setVerticalRestriction(inputState.modifierKeysPressed(ModifierKeys::MKAlt));
                if (Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType>::dragging())
                    PlaneDragPolicy::resetPlane(inputState);
            }

            bool doStartPlaneDrag(const InputState& inputState, Plane3& plane, Vec3& initialPoint) {
                if (!inputState.mouseButtonsPressed(MouseButtons::MBLeft))
                    return false;
                if (!handleEvent(inputState))
                    return false;
                plane = dragPlane(inputState, initialPoint);
                
                startMove(inputState);
                Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType>::controller().beginUndoableGroup(getActionName(inputState));
                return true;
            }
            
            bool doPlaneDrag(const InputState& inputState, const Vec3& lastPoint, const Vec3& curPoint, Vec3& refPoint) {
                const Vec3 delta = snapDelta(inputState, m_movementRestriction.apply(curPoint - refPoint));
                if (delta.null())
                    return true;
                
                const MoveResult result = move(delta);
                if (result == Conclude)
                    return false;
                if (result == Continue)
                    refPoint += delta;
                return true;
            }
            
            void doEndPlaneDrag(const InputState& inputState) {
                Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType>::controller().closeGroup();
            }
            
            void doCancelPlaneDrag(const InputState& inputState) {
                Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType>::controller().rollbackGroup();
                Tool<ActivationPolicyType, PickingPolicyType, MousePolicyType, PlaneDragPolicy, RenderPolicyType>::controller().closeGroup();
            }
            
            void doResetPlane(const InputState& inputState, Plane3& plane, Vec3& initialPoint) {
                const FloatType distance = plane.intersectWithRay(inputState.pickRay());
                if (Math::isnan(distance))
                    return;
                initialPoint = inputState.pickRay().pointAtDistance(distance);
                plane = dragPlane(inputState, initialPoint);
            }
            
            void renderMoveIndicator(const Renderer::RenderContext& renderContext) {
            }

            bool handleEvent(const InputState& inputState) const {
                return doHandleEvent(inputState);
            }
            
            String getActionName(const InputState& inputState) const {
                return doGetActionName(inputState);
            }
            
            void startMove(const InputState& inputState) {
                doStartMove(inputState);
            }
            
            Vec3 snapDelta(const InputState& inputState, const Vec3& delta) const {
                return doSnapDelta(inputState, delta);
            }
            
            MoveResult move(const Vec3& delta) {
                return doMove(delta);
            }
            
            void endMove(const InputState& inputState) {
                return doEndMove(inputState);
            }
            
            Plane3 dragPlane(const InputState& inputState, const Vec3& initialPoint) const {
                if (m_movementRestriction.isRestricted(Math::Axis::AZ)) {
                    Vec3 planeNorm = inputState.pickRay().direction;
                    planeNorm[2] = 0.0;
                    planeNorm.normalize();
                    return Plane3(initialPoint, planeNorm);
                }
                return horizontalDragPlane(initialPoint);
            }
            
            virtual bool doHandleEvent(const InputState& inputState) const = 0;
            virtual String doGetActionName(const InputState& inputState) const = 0;
            virtual void doStartMove(const InputState& inputState) = 0;
            virtual Vec3 doSnapDelta(const InputState& inputState, const Vec3& delta) const = 0;
            virtual MoveResult doMove(const Vec3& delta) = 0;
            virtual void doEndMove(const InputState& inputState) = 0;
        };
    }
}

#endif /* defined(__TrenchBroom__MoveTool__) */
