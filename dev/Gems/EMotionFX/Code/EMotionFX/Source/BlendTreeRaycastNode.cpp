/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <EMotionFX/Source/BlendTreeRaycastNode.h>
#include <Integration/AnimationBus.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeRaycastNode, AnimGraphAllocator, 0)

    BlendTreeRaycastNode::BlendTreeRaycastNode()
        : AnimGraphNode()
    {
        // Setup the input ports.
        InitInputPorts(2);
        SetupInputPort("Ray Start", INPUTPORT_RAY_START, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_RAY_START);
        SetupInputPort("Ray End", INPUTPORT_RAY_END, MCore::AttributeVector3::TYPE_ID, PORTID_INPUT_RAY_END);

        // Setup the output ports.
        InitOutputPorts(3);
        SetupOutputPort("Position", OUTPUTPORT_POSITION, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_POSITION);
        SetupOutputPort("Normal", OUTPUTPORT_NORMAL, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_NORMAL);
        SetupOutputPort("Intersected", OUTPUTPORT_INTERSECTED, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_INTERSECTED);

        if (mAnimGraph)
        {
            Reinit();
        }
    }

    BlendTreeRaycastNode::~BlendTreeRaycastNode()
    {
    }

    void BlendTreeRaycastNode::Reinit()
    {
        AnimGraphNode::Reinit();
    }

    bool BlendTreeRaycastNode::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();
        Reinit();
        return true;
    }

    const char* BlendTreeRaycastNode::GetPaletteName() const
    {
        return "Raycast";
    }

    AnimGraphObject::ECategory BlendTreeRaycastNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MISC;
    }

    void BlendTreeRaycastNode::DoOutput(AnimGraphInstance* animGraphInstance)
    {
        // Get the ray start and end.
        AZ::Vector3 rayStart;
        AZ::Vector3 rayEnd;
        if (!TryGetInputVector3(animGraphInstance, INPUTPORT_RAY_START, rayStart) || 
            !TryGetInputVector3(animGraphInstance, INPUTPORT_RAY_END, rayEnd))
        {
            SetHasError(animGraphInstance, true);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_POSITION)->SetValue(AZ::Vector3(0.0f, 0.0f, 0.0f));
            GetOutputVector3(animGraphInstance, OUTPUTPORT_NORMAL)->SetValue(AZ::Vector3(0.0f, 0.0f, 1.0f));
            GetOutputFloat(animGraphInstance, OUTPUTPORT_INTERSECTED)->SetValue(0.0f);
            return;
        }

        SetHasError(animGraphInstance, false);

        AZ::Vector3 rayDirection = rayEnd - rayStart;
        const float maxDistance = rayDirection.GetLengthExact();
        if (maxDistance > 0.0f)
        {
            rayDirection /= maxDistance;
        }

        Integration::RaycastRequests::RaycastRequest rayRequest;
        rayRequest.m_start      = rayStart;
        rayRequest.m_direction  = rayDirection;
        rayRequest.m_distance   = maxDistance;
        rayRequest.m_queryType  = Physics::QueryType::Static;
        rayRequest.m_hint       = Integration::RaycastRequests::UsecaseHint::Generic;

        // Cast a ray, check for intersections.
        Integration::RaycastRequests::RaycastResult rayResult;
        if (animGraphInstance->GetActorInstance()->GetIsOwnedByRuntime())
        {
            Integration::RaycastRequestBus::BroadcastResult(rayResult, &Integration::RaycastRequestBus::Events::Raycast, animGraphInstance->GetActorInstance()->GetEntityId(), rayRequest);
        }

        // Set the output values.
        if (rayResult.m_intersected)
        {
            GetOutputVector3(animGraphInstance, OUTPUTPORT_POSITION)->SetValue(rayResult.m_position);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_NORMAL)->SetValue(rayResult.m_normal);
            GetOutputFloat(animGraphInstance, OUTPUTPORT_INTERSECTED)->SetValue(1.0);
        }
        else
        {
            GetOutputVector3(animGraphInstance, OUTPUTPORT_POSITION)->SetValue(rayStart);
            GetOutputVector3(animGraphInstance, OUTPUTPORT_NORMAL)->SetValue(AZ::Vector3(0.0f, 0.0f, 1.0f));
            GetOutputFloat(animGraphInstance, OUTPUTPORT_INTERSECTED)->SetValue(0.0f);
        }
    }

    void BlendTreeRaycastNode::Output(AnimGraphInstance* animGraphInstance)
    {
        OutputAllIncomingNodes(animGraphInstance);
        DoOutput(animGraphInstance);
    }

    void BlendTreeRaycastNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeRaycastNode, AnimGraphNode>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeRaycastNode>("Raycast", "Raycast node attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }
} // namespace EMotionFX