#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FMDViewModelProviderBase;

struct FMDViewModelClassItem
{
	TWeakObjectPtr<UClass> Class;

	FText DisplayName;

	FTopLevelAssetPath ClassPath;

	FTopLevelAssetPath ParentClassPath;

	TWeakPtr<FMDViewModelClassItem> ParentItem;
	TSet<TSharedPtr<FMDViewModelClassItem>> Children;
};

/**
 * Helper widget to select a view model class since SClassViewer isn't exported
 */
class MDVIEWMODELEDITOR_API SMDViewModelClassList : public SCompoundWidget
{
public:
	using FOnSelectionChanged = SListView<TSharedPtr<FMDViewModelClassItem>>::FOnSelectionChanged;

	SLATE_BEGIN_ARGS(SMDViewModelClassList)
		{
		}

		SLATE_ARGUMENT(UClass*, SelectedClass)
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<FMDViewModelProviderBase>& Provider);

private:
	TSharedRef<ITableRow> OnGenerateRowForClassViewer(TSharedPtr<FMDViewModelClassItem> Item, const TSharedRef<STableViewBase>& OwnerTable);

	void PopulateClassItems(const TSharedPtr<FMDViewModelProviderBase>& Provider);

	TSharedPtr<SListView<TSharedPtr<FMDViewModelClassItem>>> ClassList;
	TArray<TSharedPtr<FMDViewModelClassItem>> ClassItems;

	TSharedPtr<FMDViewModelClassItem> SelectedItem;
	UClass* SelectedClass = nullptr;
};
