#include "ViewModelTab/MDViewModelEditor.h"

#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelDetails.h"
#include "ViewModelTab/MDViewModelList.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"


void SMDViewModelEditor::Construct(const FArguments& InArgs, TSharedPtr<FWidgetBlueprintEditor> BlueprintEditor)
{
	UWidgetBlueprint* WidgetBP = BlueprintEditor->GetWidgetBlueprintObj();
	WidgetBP->OnSetObjectBeingDebugged().AddSP(this, &SMDViewModelEditor::OnSetObjectBeingDebugged);

	ChildSlot
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+SSplitter::Slot()
		.MinSize(350.f)
		.Value(0.15f)
		[
			SNew(SMDViewModelList, WidgetBP)
			.OnViewModelSelected(this, &SMDViewModelEditor::OnViewModelSelected)
		]
		+SSplitter::Slot()
		.MinSize(400.f)
		.Value(0.15f)
		[
			SAssignNew(ViewModelDetailsWidget, SMDViewModelDetails)
			.Visibility(this, &SMDViewModelEditor::GetViewModelDetailsVisibility)
		]
		+SSplitter::Slot()
		[
			SNew(SSpacer)
		]
	];

	OnSetObjectBeingDebugged(WidgetBP->GetObjectBeingDebugged());
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

EVisibility SMDViewModelEditor::GetViewModelDetailsVisibility() const
{
	return SelectedViewModelClass != nullptr ? EVisibility::Visible : EVisibility::Hidden;
}

void SMDViewModelEditor::OnViewModelChanged()
{
	if (ViewModelDetailsWidget.IsValid())
	{
		if (SelectedViewModelClass != nullptr)
		{
			UMDViewModelBase* DebugViewModel = nullptr;
			if (UUserWidget* DebugWidget = Cast<UUserWidget>(ObjectBeingDebugged))
			{
				DebugViewModel = UMDViewModelFunctionLibrary::GetViewModel(DebugWidget, SelectedViewModelClass, SelectedViewModelName);
			}

			ViewModelDetailsWidget->UpdateViewModel(SelectedViewModelClass, DebugViewModel);
		}
		else
		{
			ViewModelDetailsWidget->UpdateViewModel(nullptr, nullptr);
		}
	}
}
