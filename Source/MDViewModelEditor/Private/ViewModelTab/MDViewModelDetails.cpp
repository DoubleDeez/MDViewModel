#include "ViewModelTab/MDViewModelDetails.h"

#include "ViewModelTab/MDViewModelFieldInspector.h"

void SMDViewModelDetails::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SAssignNew(FieldInspector, SMDViewModelFieldInspector)
	];

	UpdateViewModel(InArgs._ViewModelClass, InArgs._DebugViewModel);
}

void SMDViewModelDetails::UpdateViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UMDViewModelBase* DebugViewModel)
{
	if (FieldInspector.IsValid())
	{
		FieldInspector->SetReferences(ViewModelClass, DebugViewModel);
	}
}
