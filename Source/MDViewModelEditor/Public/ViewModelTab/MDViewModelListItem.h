#pragma once

#include "FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

class FBlueprintEditor;
struct FMDViewModelEditorAssignment;
class FMenuBuilder;

class FMDVMDragAndDropViewModel : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMDragAndDropViewModel, FMDVMInspectorDragAndDropActionBase)
	
	static TSharedRef<FMDVMDragAndDropViewModel> Create(const FMDViewModelAssignmentReference& InVMAssignment);

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;
	
	virtual FText GetNodeTitle() const override;
};

class SMDViewModelListItem : public STableRow<TSharedPtr<FMDViewModelEditorAssignment>>
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelListItem)
		{
		}

		SLATE_EVENT(FSimpleDelegate, OnDuplicateItemRequested)
		SLATE_EVENT(FSimpleDelegate, OnEditItemRequested)
		SLATE_EVENT(FSimpleDelegate, OnDeleteItemConfirmed)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwningTable, const TSharedPtr<FMDViewModelEditorAssignment>& Item, TWeakPtr<FBlueprintEditor> InBlueprintEditor);

	virtual TOptional<EMouseCursor::Type> GetCursor() const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	void OnContextMenuOpening(FMenuBuilder& ContextMenuBuilder);

private:
	EVisibility GetSourceTextVisibility() const;

	FReply OnContextButtonClicked();

	void OnFindReferencesClicked() const;
	void OnEditClicked() const;
	bool CanEdit() const;
	void OnDuplicateClicked() const;
	bool CanDuplicate() const;
	void OnDeleteClicked() const;
	bool CanDelete() const;

	FString GenerateSearchString() const;

	TSharedPtr<FMDViewModelEditorAssignment> Assignment;
	TWeakPtr<FBlueprintEditor> BlueprintEditor;

	FSlateBrush BackgroundBrush;
	FButtonStyle ButtonStyle;

	FSimpleDelegate OnDuplicateItemRequested;
	FSimpleDelegate OnEditItemRequested;
	FSimpleDelegate OnDeleteItemConfirmed;
};
