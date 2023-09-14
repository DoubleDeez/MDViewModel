#include "ViewModelTab/MDViewModelEditor.h"

#include "BlueprintEditor.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelDetails.h"
#include "ViewModelTab/MDViewModelList.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"


void SMDViewModelEditor::Construct(const FArguments& InArgs, const TSharedPtr<FBlueprintEditor>& BlueprintEditor)
{
	UBlueprint* Blueprint = BlueprintEditor->GetBlueprintObj();
	if (!ensureMsgf(IsValid(Blueprint), TEXT("Attempting to edit an invalid blueprint")))
	{
		return;	
	}
	
	Blueprint->OnSetObjectBeingDebugged().AddSP(this, &SMDViewModelEditor::OnSetObjectBeingDebugged);
	
	TArray<UBlueprint*> Hierarchy;
	UBlueprint::GetBlueprintHierarchyFromClass(Blueprint->GeneratedClass, Hierarchy);
	for (UBlueprint* BP : Hierarchy)
	{
		BP->OnCompiled().AddSP(this, &SMDViewModelEditor::OnBlueprintCompiled);
	}

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+SSplitter::Slot()
		.MinSize(300.f)
		.Value(0.15f)
		[
			SAssignNew(ViewModelListWidget, SMDViewModelList, BlueprintEditor)
			.OnViewModelSelected(this, &SMDViewModelEditor::OnViewModelSelected)
		]
		+SSplitter::Slot()
		[
			SAssignNew(ViewModelDetailsWidget, SMDViewModelDetails, BlueprintEditor)
		]
	];

	OnSetObjectBeingDebugged(Blueprint->GetObjectBeingDebugged());
}

void SMDViewModelEditor::PostUndo(bool bSuccess)
{
	if (ViewModelListWidget.IsValid())
	{
		ViewModelListWidget->RefreshList();
	}
}

void SMDViewModelEditor::PostRedo(bool bSuccess)
{
	if (ViewModelListWidget.IsValid())
	{
		ViewModelListWidget->RefreshList();
	}
}

void SMDViewModelEditor::OnSetObjectBeingDebugged(UObject* Object)
{
	ObjectBeingDebugged = Object;

	OnViewModelChanged();
}

void SMDViewModelEditor::OnViewModelSelected(FMDViewModelEditorAssignment* Assignment)
{
	if (Assignment != nullptr)
	{
		SelectedViewModelClass = Assignment->Assignment.ViewModelClass;
		SelectedViewModelName = Assignment->Assignment.ViewModelName;
	}
	else
	{
		SelectedViewModelClass = nullptr;
		SelectedViewModelName = NAME_None;
	}

	OnViewModelChanged();
}

void SMDViewModelEditor::OnViewModelChanged()
{	
	if (ViewModelDetailsWidget.IsValid())
	{
		if (SelectedViewModelClass != nullptr)
		{
			bool bIsValid = false;
			UMDViewModelBase* DebugViewModel = UMDViewModelFunctionLibrary::BP_GetViewModel(ObjectBeingDebugged.Get(), { SelectedViewModelClass, SelectedViewModelName }, bIsValid);

			ViewModelDetailsWidget->UpdateViewModel(SelectedViewModelClass, DebugViewModel, SelectedViewModelName);
		}
		else
		{
			ViewModelDetailsWidget->UpdateViewModel(nullptr, nullptr, NAME_None);
		}
	}
}

void SMDViewModelEditor::OnBlueprintCompiled(UBlueprint* BP)
{
	if (ViewModelListWidget.IsValid())
	{
		ViewModelListWidget->RefreshList();
	}
	
	// Force a refresh
	OnViewModelChanged();
}
