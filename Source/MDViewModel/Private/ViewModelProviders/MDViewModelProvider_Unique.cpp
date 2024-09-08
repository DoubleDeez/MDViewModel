#include "ViewModelProviders/MDViewModelProvider_Unique.h"

#include "Engine/Blueprint.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelFunctionLibrary.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_MDVMProvider_Unique, "MDVM.Provider.Unique");


UMDViewModelBase* UMDViewModelProvider_Unique::SetViewModel(IMDViewModelRuntimeInterface& Object, const FMDViewModelAssignment& Assignment, const FMDViewModelAssignmentData& Data)
{
	if (IsValid(Assignment.ViewModelClass))
	{
		UObject* OwningObject = Object.GetOwningObject();
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		UE_LOGFMT(LogMDViewModel, Verbose, "Creating Unique View Model with Assignment [{Assignment}] for Object [{ObjectName}]",
			("ObjectName", GetPathNameSafe(OwningObject)),
			("Assignment", Assignment));
#else
		UE_LOG(LogMDViewModel, Verbose, TEXT("Creating Unique View Model with Assignment [%s (%s)] for Object [%s]"),
			*GetNameSafe(Assignment.ViewModelClass.Get()),
			*Assignment.ViewModelName.ToString(),
			*GetPathNameSafe(OwningObject)
		);
#endif

		const FMDViewModelAssignmentReference AssignmentReference(Assignment);
		if (!Object.IsListeningForChanges(this, AssignmentReference))
		{
			auto Delegate = FMDVMOnViewModelSet::FDelegate::CreateUObject(this, &UMDViewModelProvider_Unique::OnBoundObjectViewModelChanged);
			Object.ListenForChanges(MoveTemp(Delegate), AssignmentReference);
		}

		UMDViewModelBase* ViewModel = Object.SetViewModelOfClass(OwningObject, OwningObject, AssignmentReference, Data.ViewModelSettings);
		UniqueVMs.Add(ViewModel);
		return ViewModel;
	}

	return nullptr;
}

#if WITH_EDITOR
void UMDViewModelProvider_Unique::GetExpectedContextObjectTypes(const FInstancedStruct& ProviderSettings, const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutContextObjectClasses) const
{
	if (IsValid(Blueprint))
	{
		OutContextObjectClasses.Add(Blueprint->GeneratedClass);
	}
}
#endif

void UMDViewModelProvider_Unique::OnBoundObjectViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel)
{
	if (IsValid(OldViewModel) && UniqueVMs.Remove(OldViewModel) > 0)
	{
		OldViewModel->ShutdownViewModelFromProvider();
	}
}
