#pragma once

#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FBlueprintEditor;
class UWidgetBlueprint;
class UUserWidget;
struct FMDViewModelEditorAssignment;

class SMDViewModelList : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnViewModelSelected, FMDViewModelEditorAssignment*);

	SLATE_BEGIN_ARGS(SMDViewModelList)
		{
		}

		SLATE_EVENT(FOnViewModelSelected, OnViewModelSelected)

	SLATE_END_ARGS()

	virtual ~SMDViewModelList() override;

	void Construct(const FArguments& InArgs, UWidgetBlueprint* InBlueprint, TSharedPtr<FBlueprintEditor> InBlueprintEditor);

	void RefreshList();

private:
	void OnItemSelected(TSharedPtr<FMDViewModelEditorAssignment> Item, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMDViewModelEditorAssignment> Item, const TSharedRef<STableViewBase>& OwningTable);
	TSharedPtr<SWidget> OnContextMenuOpening();
	void PopulateAssignments();

	void OnAssignmentsChanged();

	FReply OnAddViewModel();

	void OnDuplicateItem(TSharedPtr<FMDViewModelEditorAssignment> Item);
	void OnEditItem(TSharedPtr<FMDViewModelEditorAssignment> Item);
	void OnDeleteItem(TSharedPtr<FMDViewModelEditorAssignment> Item);

	TSharedPtr<SListView<TSharedPtr<FMDViewModelEditorAssignment>>> AssignmentList;
	TArray<TSharedPtr<FMDViewModelEditorAssignment>> Assignments;
	TSubclassOf<UUserWidget> WidgetClass;
	UWidgetBlueprint* WidgetBP = nullptr;
	FOnViewModelSelected OnViewModelSelected;
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
};
