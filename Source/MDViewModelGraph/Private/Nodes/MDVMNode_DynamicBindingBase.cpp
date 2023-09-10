#include "Nodes/MDVMNode_DynamicBindingBase.h"

#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"

void UMDVMNode_DynamicBindingBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	BindAssignmentChanges();
}

void UMDVMNode_DynamicBindingBase::BeginDestroy()
{
	UnbindAssignmentChanges();
	
	Super::BeginDestroy();
}

FString UMDVMNode_DynamicBindingBase::GetFindReferenceSearchString() const
{
	return FString::Printf(TEXT("\"%s\""), *GetNodeTitle(ENodeTitleType::FullTitle).ToString());
}

void UMDVMNode_DynamicBindingBase::OnAssignmentChanged()
{
	CachedNodeTitle.MarkDirty();
}

void UMDVMNode_DynamicBindingBase::BindAssignmentChanges()
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (IMDViewModelAssignableInterface* Assignments = FMDViewModelGraphStatics::GetAssignableInterface(Blueprint))
	{
		if (!Assignments->OnAssignmentChanged.IsBoundToObject(this))
		{
			Assignments->OnAssignmentChanged.AddUObject(this, &UMDVMNode_DynamicBindingBase::OnAssignmentChanged);
		}
	}
}

void UMDVMNode_DynamicBindingBase::OnAssignmentChanged(const FName& OldName, const FName& NewName, TSubclassOf<UMDViewModelBase> OldClass, TSubclassOf<UMDViewModelBase> NewClass)
{
	if (ViewModelClass == OldClass && ViewModelName == OldName)
	{
		Modify();
		ViewModelName = NewName;
		ViewModelClass = NewClass;
		OnAssignmentChanged();
		ReconstructNode();
	}
}

void UMDVMNode_DynamicBindingBase::UnbindAssignmentChanges()
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (IMDViewModelAssignableInterface* Assignments = FMDViewModelGraphStatics::GetAssignableInterface(Blueprint))
	{
		if (!Assignments->OnAssignmentChanged.IsBoundToObject(this))
		{
			Assignments->OnAssignmentChanged.RemoveAll(this);
		}
	}
}
