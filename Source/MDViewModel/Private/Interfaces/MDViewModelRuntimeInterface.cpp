#include "Interfaces/MDViewModelRuntimeInterface.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "UObject/WeakObjectPtrTemplates.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"

UClass* IMDViewModelRuntimeInterface::GetOwningObjectClass() const
{
	const UObject* Object = GetOwningObject();
	return IsValid(Object) ? Object->GetClass() : nullptr;
}

const TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& IMDViewModelRuntimeInterface::GetViewModels() const
{
	return const_cast<IMDViewModelRuntimeInterface*>(this)->GetViewModels();
}

UMDViewModelBase* IMDViewModelRuntimeInterface::SetViewModel(UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment)
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	UE_LOGFMT(LogMDViewModel, Verbose, "Setting View Model to [{VMInstance}] for assignment [{Assignment}] on Object [{ObjectName}] (Was [{CurrentVM}])",
		("VMInstance", GetPathNameSafe(ViewModel)),
		("Assignment", Assignment),
		("ObjectName", GetPathNameSafe(GetOwningObject())),
		("CurrentVM", GetPathNameSafe(GetViewModel(Assignment))));
#else
	UE_LOG(LogMDViewModel, Verbose, TEXT("Setting View Model to [%s] for assignment [%s (%s)] on Object [%s] (Was [%s])"),
		*GetPathNameSafe(ViewModel),
		*GetNameSafe(Assignment.ViewModelClass.Get()),
		*Assignment.ViewModelName.ToString(),
		*GetPathNameSafe(GetOwningObject()),
		*GetPathNameSafe(GetViewModel(Assignment)));
#endif
	
	if (!IsValid(ViewModel))
	{
		ClearViewModel(Assignment);
	}
	else if (Assignment.IsAssignmentValid())
	{
		if (!ensureAlwaysMsgf(ViewModel->IsA(Assignment.ViewModelClass.Get()), TEXT("Attempting to set View Model of class [%s] on object [%s] but it does not inherit from the assigned view model class [%s]"), *ViewModel->GetClass()->GetName(), *GetFullNameSafe(GetOwningObject()), *GetNameSafe(Assignment.ViewModelClass.Get())))
		{
			return nullptr;
		}
			
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
		MDViewModelUtils::SearchViewModelAssignments(GetOwningObjectClass(), Assignments, Assignment.ViewModelClass.Get(), FGameplayTag::EmptyTag, Assignment.ViewModelName);
		if (Assignments.IsEmpty())
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			UE_LOGFMT(LogMDViewModel, Error, "Attempting to set View Model assignment [{Assignment}] but Object Class [{ObjectClassName}] does not have a matching assignment.",
				("Assignment", Assignment),
				("ObjectClassName", GetPathNameSafe(GetOwningObjectClass())));
#else
			UE_LOG(LogMDViewModel, Error, TEXT("Attempting to set View Model assignment [%s (%s)] but Object Class [%s] does not have a matching assignment."),
				*GetNameSafe(Assignment.ViewModelClass.Get()),
				*Assignment.ViewModelName.ToString(),
				*GetPathNameSafe(GetOwningObjectClass()));
#endif
		}
			
		UMDViewModelBase* OldViewModel = GetViewModels().FindRef(Assignment);
		GetViewModels().FindOrAdd(Assignment) = ViewModel;

		if (ViewModel != OldViewModel)
		{
			BroadcastViewModelChanged(OldViewModel, ViewModel, Assignment);
		}

		return ViewModel;
	}

	return nullptr;
}

UMDViewModelBase* IMDViewModelRuntimeInterface::SetViewModelOfClass(const UObject* WorldContextObject, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings)
{
	const UClass* VMClass = Assignment.ViewModelClass.LoadSynchronous();
	if (IsValid(VMClass))
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		const FName NameBase = *FString::Printf(TEXT("SetVMofClass_%s_%s"), *VMClass->GetName(), *Assignment.ViewModelName.ToString());
		const FName VMObjectName = MakeUniqueObjectName(GetTransientPackage(), VMClass, NameBase);
#else
		const FName VMObjectName = NAME_None;
#endif
	
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
		UE_LOGFMT(LogMDViewModel, Verbose, "Creating View Model for Assignment [{Assignment}] for Object [{ObjectName}]",
			("Assignment", Assignment),
			("ObjectName", GetPathNameSafe(GetOwningObject())));
#else
		UE_LOG(LogMDViewModel, Verbose, TEXT("Creating View Model for Assignment [%s (%s)] for Object [%s]"),
			*GetNameSafe(Assignment.ViewModelClass.Get()),
			*Assignment.ViewModelName.ToString(),
			*GetPathNameSafe(GetOwningObject()));
#endif
		UMDViewModelBase* ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), VMClass, VMObjectName);
		if (IsValid(ViewModel))
		{
			ViewModel->InitializeViewModelWithContext(ViewModelSettings, ContextObject, WorldContextObject);
			return SetViewModel(ViewModel, Assignment);
		}
	}

	return nullptr;
}

UMDViewModelBase* IMDViewModelRuntimeInterface::GetViewModel(const FMDViewModelAssignmentReference& Assignment) const
{
	return GetViewModels().FindRef(Assignment);
}

void IMDViewModelRuntimeInterface::ClearViewModel(const FMDViewModelAssignmentReference& Assignment)
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	UE_LOGFMT(LogMDViewModel, Verbose, "Clearing View Model assignment [{Assignment}] for Object [{ObjectName}] (Was [{CurrentVM}])",
		("Assignment", Assignment),
		("ObjectName", GetPathNameSafe(GetOwningObject())),
		("CurrentVM", GetPathNameSafe(GetViewModel(Assignment))));
#else
	UE_LOG(LogMDViewModel, Verbose, TEXT("Clearing View Model assignment [%s (%s)] for Object [%s] (Was [%s])"),
		*GetNameSafe(Assignment.ViewModelClass.Get()),
		*Assignment.ViewModelName.ToString(),
		*GetPathNameSafe(GetOwningObject()),
		*GetPathNameSafe(GetViewModel(Assignment)));
#endif

	if (Assignment.IsAssignmentValid())
	{
		UMDViewModelBase* OldViewModel = GetViewModels().FindRef(Assignment);
		if (GetViewModels().Remove(Assignment) != 0)
		{
			BroadcastViewModelChanged(OldViewModel, nullptr, Assignment);
		}
	}
}

FDelegateHandle IMDViewModelRuntimeInterface::ListenForAnyViewModelChanged(FSimpleDelegate&& Delegate)
{
	return OnAnyViewModelSetDelegates.Add(MoveTemp(Delegate));
}

void IMDViewModelRuntimeInterface::StopListeningForAnyViewModelChanged(FDelegateHandle& Handle)
{
	if (OnAnyViewModelSetDelegates.Remove(Handle))
	{
		Handle.Reset();
	}
}

void IMDViewModelRuntimeInterface::StopListeningForAnyViewModelChanged(const void* BoundObject)
{
	OnAnyViewModelSetDelegates.RemoveAll(BoundObject);
}

FDelegateHandle IMDViewModelRuntimeInterface::ListenForChanges(FMDVMOnViewModelSet::FDelegate&& Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	if (Assignment.IsAssignmentValid())
	{
		return OnViewModelSetDelegates.FindOrAdd(Assignment).Add(MoveTemp(Delegate));
	}

	return {};
}

void IMDViewModelRuntimeInterface::StopListeningForChanges(FDelegateHandle& Handle, const FMDViewModelAssignmentReference& Assignment)
{
	if (Assignment.IsAssignmentValid())
	{
		if (FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Assignment))
		{
			if (Delegate->Remove(Handle))
			{
				Handle.Reset();
			}
		}
	}
}

void IMDViewModelRuntimeInterface::StopListeningForAllNativeViewModelsChanged(const void* BoundObject)
{
	for (auto& Pair : OnViewModelSetDelegates)
	{
		Pair.Value.RemoveAll(BoundObject);
	}
}

void IMDViewModelRuntimeInterface::ListenForChanges(FMDVMOnViewModelSetDynamic&& Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	if (Assignment.IsAssignmentValid())
	{
		OnViewModelSetDynamicDelegates.FindOrAdd(Assignment).Add(MoveTemp(Delegate));
	}
}

void IMDViewModelRuntimeInterface::StopListeningForChanges(const FMDVMOnViewModelSetDynamic& Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	if (Assignment.IsAssignmentValid())
	{
		if (TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Assignment))
		{
			Delegates->Remove(Delegate);
		}
	}
}

void IMDViewModelRuntimeInterface::StopListeningForAllDynamicViewModelsChanged(const UObject* BoundObject)
{
	for (auto& Pair : OnViewModelSetDynamicDelegates)
	{
		for (auto It = Pair.Value.CreateIterator(); It; ++It)
		{
			if (It->IsBoundToObject(BoundObject))
			{
				It.RemoveCurrent();
			}
		}
	}
}

bool IMDViewModelRuntimeInterface::IsListeningForChanges(const UObject* BoundObject, const FMDViewModelAssignmentReference& Assignment) const
{
	if (Assignment.IsAssignmentValid())
	{
		if (const FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Assignment))
		{
			if (Delegate->IsBoundToObject(BoundObject))
			{
				return true;
			}
		}
		
		if (const TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Assignment))
		{
			for (const FMDVMOnViewModelSetDynamic& Delegate : *Delegates)
			{
				if (Delegate.IsBoundToObject(BoundObject))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void IMDViewModelRuntimeInterface::PopulateViewModels()
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	UE_LOGFMT(LogMDViewModel, Verbose, "Populating View Models for Object [{ObjectName}]", ("ObjectName", GetPathNameSafe(GetOwningObject())));
#else
	UE_LOG(LogMDViewModel, Verbose, TEXT("Populating View Models for Object [%s]"), *GetPathNameSafe(GetOwningObject()));
#endif

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	MDViewModelUtils::GetViewModelAssignments(GetOwningObjectClass(), ViewModelAssignments);

	for (const auto& Pair : ViewModelAssignments)
	{
		UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Pair.Key.ProviderTag);
		if (ensureMsgf(IsValid(Provider), TEXT("A View Model Provider with tag [%s] was not found"), *Pair.Key.ProviderTag.ToString()))
		{
			if (!IsValid(Provider->SetViewModel(*this, Pair.Key, Pair.Value)))
			{
				// Broadcast even if a view model doesn't get set so that anything bound can initialize
				BroadcastViewModelChanged(nullptr, nullptr, FMDViewModelAssignmentReference(Pair.Key));
			}
		}
	}
}

void IMDViewModelRuntimeInterface::CleanUpViewModels()
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
	UE_LOGFMT(LogMDViewModel, Verbose, "Cleaning up View Models for Object [{ObjectName}]", ("ObjectName", GetPathNameSafe(GetOwningObject())));
#else
	UE_LOG(LogMDViewModel, Verbose, TEXT("Cleaning up View Models for Object [%s]"), *GetPathNameSafe(GetOwningObject()));
#endif
	
	// Broadcast out that we're null-ing out view models
	{
		for (auto It = OnViewModelSetDelegates.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = GetViewModels().FindRef(It.Key());
			if (IsValid(ViewModel))
			{
				It.Value().Broadcast(ViewModel, nullptr);
			}
		}

		for (auto It = OnViewModelSetDynamicDelegates.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = GetViewModels().FindRef(It.Key());
			if (IsValid(ViewModel))
			{
				for (const FMDVMOnViewModelSetDynamic& Delegate : It.Value())
				{
					Delegate.ExecuteIfBound(ViewModel, nullptr);
				}
			}
		}
	}

	// View models themselves
	{
		UObject* OwnerObject = GetOwningObject();
		for (auto It = GetViewModels().CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = It.Value();
			if (IsValid(ViewModel))
			{
				ViewModel->OnUnsetFromObject(OwnerObject);
			}
		}

		GetViewModels().Reset();
	}
}

void IMDViewModelRuntimeInterface::BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, const FMDViewModelAssignmentReference& Assignment)
{
	UObject* OwnerObject = GetOwningObject();
	if (IsValid(OldViewModel))
	{
		OldViewModel->OnUnsetFromObject(OwnerObject);
	}

	if (IsValid(NewViewModel))
	{
		NewViewModel->OnSetOnObject(OwnerObject);
	}

	if (const FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Assignment))
	{
		Delegate->Broadcast(OldViewModel, NewViewModel);
	}

	if (const TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Assignment))
	{
		for (const FMDVMOnViewModelSetDynamic& Delegate : *Delegates)
		{
			Delegate.ExecuteIfBound(OldViewModel, NewViewModel);
		}
	}
}
