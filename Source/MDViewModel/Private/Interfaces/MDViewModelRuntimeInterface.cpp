#include "Interfaces/MDViewModelRuntimeInterface.h"

#include "Logging/StructuredLog.h"
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

UMDViewModelBase* IMDViewModelRuntimeInterface::SetViewModel(UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment)
{
	UE_LOGFMT(LogMDViewModel, Verbose, "Setting View Model to [{VMInstance}] for assignment [{Assignment}] on Object [{ObjectName}] (Was [{CurrentVM}])",
		("VMInstance", GetPathNameSafe(ViewModel)),
		("Assignment", Assignment),
		("ObjectName", GetPathNameSafe(GetOwningObject())),
		("CurrentVM", GetPathNameSafe(GetViewModel(Assignment))));
	
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
			UE_LOGFMT(LogMDViewModel, Error, "Attempting to set View Model assignment [{Assignment}] but Object Class [{ObjectClassName}] does not have a matching assignment.",
				("Assignment", Assignment),
				("ObjectClassName", GetPathNameSafe(GetOwningObjectClass())));
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
	UE_LOGFMT(LogMDViewModel, Verbose, "Creating View Model for Assignment [{Assignment}] for Object [{ObjectName}]",
		("Assignment", Assignment),
		("ObjectName", GetPathNameSafe(GetOwningObject())));
	UMDViewModelBase* ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), Assignment.ViewModelClass.LoadSynchronous());
	if (IsValid(ViewModel))
	{
		ViewModel->InitializeViewModelWithContext(ViewModelSettings, ContextObject, WorldContextObject);
		return SetViewModel(ViewModel, Assignment);
	}

	return nullptr;
}

UMDViewModelBase* IMDViewModelRuntimeInterface::GetViewModel(const FMDViewModelAssignmentReference& Assignment) const
{
	return GetViewModels().FindRef(Assignment);
}

void IMDViewModelRuntimeInterface::ClearViewModel(const FMDViewModelAssignmentReference& Assignment)
{
	UE_LOGFMT(LogMDViewModel, Verbose, "Clearing View Model assignment [{Assignment}] for Object [{ObjectName}] (Was [{CurrentVM}])",
		("Assignment", Assignment),
		("ObjectName", GetPathNameSafe(GetOwningObject())),
		("CurrentVM", GetPathNameSafe(GetViewModel(Assignment))));

	if (Assignment.IsAssignmentValid())
	{
		UMDViewModelBase* OldViewModel = GetViewModels().FindRef(Assignment);
		if (GetViewModels().Remove(Assignment) != 0)
		{
			BroadcastViewModelChanged(OldViewModel, nullptr, Assignment);
		}
	}
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
	UE_LOGFMT(LogMDViewModel, Verbose, "Populating View Models for Object [{ObjectName}]", ("ObjectName", GetPathNameSafe(GetOwningObject())));

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
	UE_LOGFMT(LogMDViewModel, Verbose, "Cleaning up View Models for  Object [{ObjectName}]", ("ObjectName", GetPathNameSafe(GetOwningObject())));
	
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
