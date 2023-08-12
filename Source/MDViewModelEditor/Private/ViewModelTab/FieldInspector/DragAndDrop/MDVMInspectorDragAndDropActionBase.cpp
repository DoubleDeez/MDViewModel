#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"

#include "EdGraphSchema_K2.h"
#include "Styling/AppStyle.h"
#include "ViewModel/MDViewModelBase.h"


FReply FMDVMInspectorDragAndDropActionBase::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	UEdGraph* Graph = GetHoveredGraph();
	if (Graph != nullptr && VMAssignment.IsAssignmentValid())
	{		
		if (UEdGraphNode* ResultNode = CreateNodeOnDrop(*Graph, GraphPosition))
		{
			if (UEdGraphPin* FromPin = GetHoveredPin())
			{
				ResultNode->AutowireNewNode(FromPin);
			}
			else if (const UEdGraphNode* FromNode = GetHoveredNode())
			{
				const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
				if (UEdGraphPin* FromNodeIn = Schema->FindExecutionPin(*FromNode, EGPD_Input))
				{
					ResultNode->AutowireNewNode(FromNodeIn);
				}
				else if (UEdGraphPin* FromNodeOut = Schema->FindExecutionPin(*FromNode, EGPD_Output))
				{
					ResultNode->AutowireNewNode(FromNodeOut);
				}
			}
			
			return FReply::Handled();
		}
	}
	
	return FReply::Unhandled();
}

FReply FMDVMInspectorDragAndDropActionBase::DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	return DroppedOnPin(ScreenPosition, GraphPosition);
}

FReply FMDVMInspectorDragAndDropActionBase::DroppedOnPanel(const TSharedRef<SWidget>& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	return DroppedOnPin(ScreenPosition, GraphPosition);
}

bool FMDVMInspectorDragAndDropActionBase::IsSupportedBySchema(const UEdGraphSchema* Schema) const
{
	return Schema != nullptr && Schema->IsA<UEdGraphSchema_K2>();
}

void FMDVMInspectorDragAndDropActionBase::HoverTargetChanged()
{
	const UEdGraph* Graph = GetHoveredGraph();
	if (Graph != nullptr && IsSupportedBySchema(Graph->GetSchema()))
	{
		const FSlateBrush* StatusSymbol = FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
		SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::Format(INVTEXT("Place Node: {0}"), GetNodeTitle()));
	}
	else
	{
		const FSlateBrush* StatusSymbol = FAppStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
		SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::Format(INVTEXT("Cannot place [{0}] here"), GetNodeTitle()));
	}
}

FText FMDVMInspectorDragAndDropActionBase::GetNodeTitle() const
{
	const FText VMClassName = (VMAssignment.ViewModelClass.Get() != nullptr) ? VMAssignment.ViewModelClass.Get()->GetDisplayNameText() : INVTEXT("NULL");
	return FText::Format(INVTEXT("{0} from {1} ({2})"), GetActionTitle(), VMClassName, FText::FromName(VMAssignment.ViewModelName));
}
