#pragma once

#include "FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

class FBlueprintEditor;
struct FMDViewModelEditorAssignment;
class FMenuBuilder;
class FUICommandList;

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
		{}

		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, CommandList)

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
	void OnOpenDefinitionClicked() const;
	bool CanOpenDefinition() const;
	void OnOpenOwnerAssetClicked() const;
	bool CanOpenOwnerAsset() const;

	FString GenerateSearchString() const;

	UBlueprint* GetBlueprint() const;

	TSharedPtr<FMDViewModelEditorAssignment> Assignment;
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	TSharedPtr<FUICommandList> CommandList;

	FSlateBrush BackgroundBrush;
	FButtonStyle ButtonStyle;
};
