#include "Bindings/MDVMBlueprintBindingBase.h"

#include "Blueprint/UserWidget.h"
#include "Interfaces/MDViewModelRuntimeInterface.h"
#include "Util/MDViewModelUtils.h"

void UMDVMBlueprintBindingBase::BindViewModelDelegates(IMDViewModelRuntimeInterface& Object) const
{
	Object.StopListeningForAllNativeViewModelsChanged(this);
		
	TWeakObjectPtr<UObject> WeakObject = Object.GetOwningObject();
	for (int32 i = 0; i < GetNumEntries(); ++i)
	{
		if (const FMDViewModeBindingEntryBase* Entry = GetEntry(i))
		{
			auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDVMBlueprintBindingBase::OnViewModelChanged, i, WeakObject);
			Object.ListenForChanges(MoveTemp(Delegate), { Entry->ViewModelClass, Entry->ViewModelName });
		}
	}
}

void UMDVMBlueprintBindingBase::UnbindViewModelDelegates(IMDViewModelRuntimeInterface& Object) const
{
	Object.StopListeningForAllNativeViewModelsChanged(this);
}

void UMDVMBlueprintBindingBase::BindDynamicDelegates(UObject* InInstance) const
{
	// Only Widgets bind here, actors will bind from UMDViewModelAssignmentComponent::BeginPlay
	if (IsValid(InInstance) && InInstance->IsA<UUserWidget>())
	{
		if (IMDViewModelRuntimeInterface* Object = MDViewModelUtils::GetOrCreateViewModelRuntimeInterface(InInstance))
		{
			BindViewModelDelegates(*Object);
		}
	}
}

void UMDVMBlueprintBindingBase::UnbindDynamicDelegates(UObject* InInstance) const
{
	// Only Widgets unbind here, actors will unbind from UMDViewModelAssignmentComponent::EndPlay
	if (IsValid(InInstance) && InInstance->IsA<UUserWidget>())
	{
		if (IMDViewModelRuntimeInterface* Object = MDViewModelUtils::GetOrCreateViewModelRuntimeInterface(InInstance))
		{
			UnbindViewModelDelegates(*Object);
		}
	}
}
