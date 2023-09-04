#include "Nodes/MDVMNode_DynamicBindingBase.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

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
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			if (!VMExtension->OnAssignmentChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentChanged.AddUObject(this, &UMDVMNode_DynamicBindingBase::OnAssignmentChanged);
			}
		}
	}
	else if (IsValid(Blueprint))
	{
		// TODO - Actor View Models
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
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			VMExtension->OnAssignmentChanged.RemoveAll(this);
		}
	}
	else if (IsValid(Blueprint))
	{
		// TODO - Actor View Models
	}
}
