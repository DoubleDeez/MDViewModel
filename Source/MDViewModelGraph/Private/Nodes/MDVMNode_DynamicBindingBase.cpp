#include "Nodes/MDVMNode_DynamicBindingBase.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

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
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(FBlueprintEditorUtils::FindBlueprintForNode(this)))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			if (!VMExtension->OnAssignmentChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentChanged.AddUObject(this, &UMDVMNode_DynamicBindingBase::OnAssignmentChanged);
			}
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
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(FBlueprintEditorUtils::FindBlueprintForNode(this)))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			VMExtension->OnAssignmentChanged.RemoveAll(this);
		}
	}
}
