#include "Nodes/MDVMNode_DynamicBindingBase.h"

#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"

void UMDVMNode_DynamicBindingBase::ReconstructNode()
{
	UpdateDeprecatedProperties();

	Super::ReconstructNode();
}

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

void UMDVMNode_DynamicBindingBase::PostLoad()
{
	UpdateDeprecatedProperties();

	Super::PostLoad();
}

FText UMDVMNode_DynamicBindingBase::GetViewModelClassName() const
{
	const UClass* VMClass = Assignment.ViewModelClass.LoadSynchronous();
	return IsValid(VMClass) ? VMClass->GetDisplayNameText() : INVTEXT("NULL");
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

void UMDVMNode_DynamicBindingBase::OnAssignmentChanged(const FMDViewModelAssignmentReference& Old, const FMDViewModelAssignmentReference& New)
{
	if (Assignment == Old)
	{
		Modify();
		Assignment = New;
		OnAssignmentChanged();
		ReconstructNode();
	}
}

void UMDVMNode_DynamicBindingBase::UnbindAssignmentChanges()
{
	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (IMDViewModelAssignableInterface* Assignments = FMDViewModelGraphStatics::GetAssignableInterface(Blueprint))
	{
		Assignments->OnAssignmentChanged.RemoveAll(this);
	}
}

void UMDVMNode_DynamicBindingBase::UpdateDeprecatedProperties()
{
	if (ViewModelClass != nullptr && ViewModelName != NAME_None && !Assignment.IsAssignmentValid())
	{
		Assignment.ViewModelClass = ViewModelClass;
		Assignment.ViewModelName = ViewModelName;
		ViewModelClass = nullptr;
		ViewModelName = NAME_None;
	}
}
