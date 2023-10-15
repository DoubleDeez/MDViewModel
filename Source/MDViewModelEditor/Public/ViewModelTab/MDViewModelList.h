#pragma once

#include "Templates/SubclassOf.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FBlueprintEditor;
struct FMDViewModelEditorAssignment;
class FUICommandList;
class IMDViewModelAssignableInterface;

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

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void SetupCommands();

	void OnItemSelected(TSharedPtr<FMDViewModelEditorAssignment> Item, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMDViewModelEditorAssignment> Item, const TSharedRef<STableViewBase>& OwningTable);
	TSharedPtr<SWidget> OnContextMenuOpening();
	void PopulateAssignments();

	void OnAssignmentsChanged(UBlueprint* Blueprint);

	FReply OnAddViewModel();
	bool CanAddViewModel() const;
	void OnAssignmentAdded(const FMDViewModelEditorAssignment& Assignment);

	bool IsSelectedAssignmentValid() const;
	bool IsSelectedAssignmentValidAndNotPIE() const;
	bool IsNotPIE() const;

	void EditSelectedAssignment();
	bool CanEditSelectedAssignment() const;
	void CopySelectedAssignment();
	void PasteAssignment();
	bool CanPasteAssignment() const;
	void DuplicateSelectedAssignment();
	void DeleteSelectedAssignment();
	void OnGoToSelectedAssignmentDefinitionClicked() const;
	bool CanGoToSelectedAssignmentDefinition() const;

	void OnItemDoubleClicked(TSharedPtr<FMDViewModelEditorAssignment> Item);

	UBlueprint* GetBlueprint() const;
	UClass* GetGeneratedClass() const;

	TSharedPtr<FMDViewModelEditorAssignment> GetSelectedAssignment() const;

	TSharedPtr<SListView<TSharedPtr<FMDViewModelEditorAssignment>>> AssignmentList;
	TArray<TSharedPtr<FMDViewModelEditorAssignment>> Assignments;
	FOnViewModelSelected OnViewModelSelected;
	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;
	TSharedPtr<FUICommandList> CommandList;
};
