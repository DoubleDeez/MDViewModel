#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"

class UWidgetBlueprint;
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
		, _WidgetBP(nullptr)
		{
		}

		SLATE_ARGUMENT(TSubclassOf<UMDViewModelBase>, ViewModelClass)
		SLATE_ARGUMENT(FName, ViewModelName)
		SLATE_ARGUMENT(UMDViewModelBase*, DebugViewModel)
		SLATE_ARGUMENT(UWidgetBlueprint*, WidgetBP)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void UpdateViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UMDViewModelBase* DebugViewModel, const FName& ViewModelName);

private:
	TSharedPtr<SMDViewModelFieldInspector> PropertyInspector;
	TSharedPtr<SMDViewModelFieldInspector> EventInspector;
	TSharedPtr<SMDViewModelFieldInspector> CommandInspector;
};
