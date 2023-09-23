#pragma once

#include "GraphEditorDragDropAction.h"
#include "MDViewModelFunctionDebugLineItem.h"
#include "SPinValueInspector.h"
#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakFieldPtr.h"
#include "Util/MDViewModelAssignmentReference.h"

class FBlueprintEditor;
class FMDViewModelChangedDebugLineItem;
class FMDViewModelEventDebugLineItem;
class FMDViewModelFunctionDebugLineItem;
class FMDViewModelFieldDebugLineItem;
class UMDVMNode_ViewModelEvent;
class UMDViewModelBase;

enum class EMDViewModelFieldInspectorType : uint8
{
	None,
	Properties,
	Events,
	Commands,
	Helpers
};

/**
 * Widget that displays all the exposed properties of a viewmodel and their values when debugging
 */
class MDVIEWMODELEDITOR_API SMDViewModelFieldInspector : public SPinValueInspector
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelFieldInspector)
	{
	}
		SLATE_ARGUMENT(EMDViewModelFieldInspectorType, InspectorType)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor);

	void SetReferences(TSubclassOf<UMDViewModelBase> InViewModelClass, UMDViewModelBase* InDebugViewModel, const FName& InViewModelName);

	void RefreshList();

protected:
	virtual void PopulateTreeView() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// We can't support search since that requires inheriting from FLineItemWithChildren but that's private
	virtual EVisibility GetSearchFilterVisibility() const override { return EVisibility::Collapsed; }

private:
	void GetDragAndDropCreatorForFunction(const UFunction& Func, bool bIsFieldNotify, FMDVMDragAndDropCreatorFunc& DragAndDropCreatorFunc) const;

	TMap<const FProperty*, TSharedPtr<FMDViewModelFieldDebugLineItem>> PropertyTreeItems;
	TMap<const UFunction*, TSharedPtr<FMDViewModelFunctionDebugLineItem>> FunctionTreeItems;
	TMap<const FMulticastDelegateProperty*, TSharedPtr<FMDViewModelEventDebugLineItem>> EventTreeItems;
	TSharedPtr<FMDViewModelChangedDebugLineItem> VMChangedItem;

	TSubclassOf<UMDViewModelBase> ViewModelClass;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
	bool bIsDebugging = false;
	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;
	FName ViewModelName = NAME_None;

	EMDViewModelFieldInspectorType InspectorType = EMDViewModelFieldInspectorType::None;
};
