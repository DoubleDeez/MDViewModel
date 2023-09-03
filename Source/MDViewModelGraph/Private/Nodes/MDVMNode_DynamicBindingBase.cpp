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
			if (!VMExtension->OnAssignmentNameChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentNameChanged.AddUObject(this, &UMDVMNode_DynamicBindingBase::OnAssignmentNameChanged);
			}
			
			if (!VMExtension->OnAssignmentClassChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentClassChanged.AddUObject(this, &UMDVMNode_DynamicBindingBase::OnAssignmentClassChanged);
			}
		}
	}
}

void UMDVMNode_DynamicBindingBase::OnAssignmentNameChanged(TSubclassOf<UMDViewModelBase> VMClass, const FName& OldName, const FName& NewName)
{
	if (ViewModelClass == VMClass && ViewModelName == OldName)
	{
		Modify();
		ViewModelName = NewName;
		OnAssignmentChanged();
	}
}

void UMDVMNode_DynamicBindingBase::OnAssignmentClassChanged(const FName& VMName, TSubclassOf<UMDViewModelBase> OldClass, TSubclassOf<UMDViewModelBase> NewClass)
{
	if (ViewModelClass == OldClass && ViewModelName == VMName)
	{
		Modify();
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
			VMExtension->OnAssignmentNameChanged.RemoveAll(this);
		}
		
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			VMExtension->OnAssignmentClassChanged.RemoveAll(this);
		}
	}
}
