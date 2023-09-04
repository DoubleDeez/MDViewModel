#pragma once

#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class UMDViewModelWidgetBlueprintExtension;
class FBlueprintEditor;
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

	void Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& InBlueprintEditor);

	void RefreshList();

private:
	void OnItemSelected(TSharedPtr<FMDViewModelEditorAssignment> Item, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMDViewModelEditorAssignment> Item, const TSharedRef<STableViewBase>& OwningTable);
	TSharedPtr<SWidget> OnContextMenuOpening();
	void PopulateAssignments();

	void OnAssignmentsChanged();

	FReply OnAddViewModel();
	bool CanAddViewModel() const;

	void OnPasteClicked();
	bool CanPaste() const;

	void OnDuplicateItem(TSharedPtr<FMDViewModelEditorAssignment> Item);
	void OnEditItem(TSharedPtr<FMDViewModelEditorAssignment> Item);
	void OnDeleteItem(TSharedPtr<FMDViewModelEditorAssignment> Item);

	UBlueprint* GetBlueprint() const;
	UClass* GetGeneratedClass() const;

	TSharedPtr<SListView<TSharedPtr<FMDViewModelEditorAssignment>>> AssignmentList;
	TArray<TSharedPtr<FMDViewModelEditorAssignment>> Assignments;
	FOnViewModelSelected OnViewModelSelected;
	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;
};
