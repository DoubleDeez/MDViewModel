#include "Nodes/MDVMNode_DynamicBindingBase.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

void UMDVMNode_DynamicBindingBase::ReconstructNode()
{
	Super::ReconstructNode();

	BindAssignmentNameChanged();
}

void UMDVMNode_DynamicBindingBase::BeginDestroy()
{
	UnbindAssignmentNameChanged();
	
	Super::BeginDestroy();
}

void UMDVMNode_DynamicBindingBase::BindAssignmentNameChanged()
{
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(FBlueprintEditorUtils::FindBlueprintForNode(this)))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			if (!VMExtension->OnAssignmentNameChanged.IsBoundToObject(this))
			{
				VMExtension->OnAssignmentNameChanged.AddUObject(this, &UMDVMNode_DynamicBindingBase::OnAssignmentNameChanged);
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
		CachedNodeTitle.MarkDirty();
	}
}

void UMDVMNode_DynamicBindingBase::UnbindAssignmentNameChanged()
{
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(FBlueprintEditorUtils::FindBlueprintForNode(this)))
	{
		if (auto* VMExtension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			VMExtension->OnAssignmentNameChanged.RemoveAll(this);
		}
	}
}
