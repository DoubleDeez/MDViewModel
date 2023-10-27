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

struct FMDVMCachedViewModelNodeParams
{
	FMDViewModelAssignmentReference VMAssignment;
	FVector2D GraphPosition = FVector2D(0);
	TWeakObjectPtr<UEdGraph> Graph;
	TWeakObjectPtr<UEdGraphNode> Node;
	FEdGraphPinReference Pin;
};

class FMDVMDragAndDropViewModel : public FMDVMInspectorDragAndDropActionBase
{
public:
	DRAG_DROP_OPERATOR_TYPE(FMDVMDragAndDropViewModel, FMDVMInspectorDragAndDropActionBase)

	static TSharedRef<FMDVMDragAndDropViewModel> Create(const FMDViewModelAssignmentReference& InVMAssignment);

	static void CreateGetter(FMDVMCachedViewModelNodeParams Params);
	static void CreateSetter(FMDVMCachedViewModelNodeParams Params);
	static void FinalizeNode(UEdGraphNode* NewNode, const FMDVMCachedViewModelNodeParams& Params);

	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) override;

	virtual UEdGraphNode* CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition) override;

	virtual FText GetNodeTitle() const override;

	void SetAltDrag(bool InIsAltDrag) {	bAltDrag = InIsAltDrag; }
	void SetCtrlDrag(bool InIsCtrlDrag) { bControlDrag = InIsCtrlDrag; }
	void SetCanSet(bool InCanSet);

private:
	TOptional<bool> bIsGetter;
	bool bCanSet = true;
	bool bControlDrag = false;
	bool bAltDrag = false;
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
	EVisibility GetDebugVisibility() const;

	FText GetDebugViewModelTooltip() const;
	const FSlateBrush* GetViewModelInstanceIcon() const;

	FReply OnContextButtonClicked();

	void OnFindReferencesClicked() const;
	void OnClassReferenceViewerClicked() const;
	void OnAssignmentReferenceViewerClicked() const;
	void OnProviderReferenceViewerClicked() const;
	void OnOpenOwnerAssetClicked() const;
	bool CanOpenOwnerAsset() const;

	void OnCopyViewModelNameClicked() const;
	bool CanCopyViewModelName() const;

	FString GenerateSearchString() const;

	UBlueprint* GetBlueprint() const;

	TSharedPtr<FMDViewModelEditorAssignment> Assignment;
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	TSharedPtr<FUICommandList> CommandList;

	FSlateBrush BackgroundBrush;
	FButtonStyle ButtonStyle;
};
