#pragma once

#include "Styling/SlateBrush.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"

class FBlueprintEditor;
class UMDViewModelBase;
class SMDViewModelFieldInspector;

/**
 *
 */
class MDVIEWMODELEDITOR_API SMDViewModelDetails : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelDetails)
		: _DebugViewModel(nullptr)
		{
		}

		SLATE_ARGUMENT(TSubclassOf<UMDViewModelBase>, ViewModelClass)
		SLATE_ARGUMENT(FName, ViewModelName)
		SLATE_ARGUMENT(UMDViewModelBase*, DebugViewModel)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor);

	void UpdateViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UMDViewModelBase* DebugViewModel, const FName& ViewModelName);

private:
	TSharedPtr<SMDViewModelFieldInspector> PropertyInspector;
	TSharedPtr<SMDViewModelFieldInspector> EventInspector;
	TSharedPtr<SMDViewModelFieldInspector> CommandInspector;
};
