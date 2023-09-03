#include "WidgetExtensions/MDViewModelWidgetExtension.h"

#include "Util/MDViewModelUtils.h"
#include "Launch/Resources/Version.h"
#include "Blueprint/UserWidget.h"
#include "Logging/StructuredLog.h"
#include "UObject/Package.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelLog.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"

void UMDViewModelWidgetExtension::Construct()
{
	Super::Construct();

	PopulateViewModels();
}

void UMDViewModelWidgetExtension::Destruct()
{
	CleanUpViewModels();
	
	Super::Destruct();
}

void UMDViewModelWidgetExtension::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnBeginDestroy.Broadcast();
	}
	
	Super::BeginDestroy();
}

UMDViewModelWidgetExtension* UMDViewModelWidgetExtension::GetOrCreate(UUserWidget* Widget)
{
	if (IsValid(Widget))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			return Extension;
		}

		return Widget->AddExtension<UMDViewModelWidgetExtension>();
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::SetViewModel(UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModel) && IsValid(ViewModelClass))
	{
		if (!ensureAlwaysMsgf(ViewModel->IsA(ViewModelClass), TEXT("Attempting to set View Model of class [%s] on widget [%s] but it does not inherit from the assigned view model class [%s]"), *ViewModel->GetClass()->GetName(), *GetFullNameSafe(GetUserWidget()), *ViewModelClass->GetName()))
		{
			return nullptr;
		}
		
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
		MDViewModelUtils::SearchViewModelAssignments(Assignments, GetUserWidget()->GetClass(), ViewModelClass, FGameplayTag::EmptyTag, ViewModelName);
		if (Assignments.IsEmpty())
		{
			UE_LOGFMT(LogMDViewModel, Error, "Attempting to set View Model of type [{VMType}] with name [{VMName}] but Widget [{Widget}] does not have a matching assignment.",
				("VMType", ViewModelClass->GetFName()),
				("VMName", ViewModelName),
				("Widget", GetUserWidget()->GetClass()->GetPathName()));
		}
		
		UE_LOGFMT(LogMDViewModel, Verbose, "Setting View Model to [{VMInstance}] named [{VMName}] of class [{VMClass}] on Widget [{WidgetName}] (Was [{CurrentVM}])",
			("VMInstance", ViewModel->GetPathName()),
			("VMName", ViewModelName),
			("VMClass", ViewModelClass->GetPathName()),
			("WidgetName", GetUserWidget()->GetPathName()),
			("CurrentVM", GetPathNameSafe(GetViewModel(ViewModelClass, ViewModelName))));
		
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			UMDViewModelBase* OldViewModel = ViewModels.FindRef(Key);
			ViewModels.FindOrAdd(Key) = ViewModel;

			if (ViewModel != OldViewModel)
			{
				BroadcastViewModelChanged(OldViewModel, ViewModel, ViewModelClass, ViewModelName);
			}

			return ViewModel;
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::SetViewModelOfClass(const UObject* WorldContextObject, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		UE_LOGFMT(LogMDViewModel, Verbose, "Creating View Model of Class [{VMClassName}] for Widget [{WidgetName}]",
			("VMClassName", ViewModelClass->GetPathName()),
			("WidgetName", GetUserWidget()->GetPathName()));
		UMDViewModelBase* ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), ViewModelClass);
		if (IsValid(ViewModel))
		{
			ViewModel->InitializeViewModelWithContext(ViewModelSettings, ContextObject, WorldContextObject);
			return SetViewModel(ViewModel, ViewModelClass, ViewModelName);
		}
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelWidgetExtension::GetViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName) const
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			return ViewModels.FindRef(Key);
		}
	}

	return nullptr;
}

void UMDViewModelWidgetExtension::ClearViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (ViewModelClass != nullptr)
	{
		UE_LOGFMT(LogMDViewModel, Verbose, "Clearing View Model named [{VMName}] of Class [{VMClassName}] for Widget [{WidgetName}] (Was [{CurrentVM}])",
			("VMName", ViewModelName),
			("VMClassName", ViewModelClass->GetPathName()),
			("WidgetName", GetUserWidget()->GetPathName()),
			("CurrentVM", GetPathNameSafe(GetViewModel(ViewModelClass, ViewModelName))));
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		if (ViewModelName != NAME_None)
		{
			const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
			UMDViewModelBase* OldViewModel = ViewModels.FindRef(Key);
			ViewModels.FindOrAdd(Key) = nullptr;

			BroadcastViewModelChanged(OldViewModel, nullptr, ViewModelClass, ViewModelName);
		}
	}
}

void UMDViewModelWidgetExtension::OnProviderViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass, FGameplayTag ProviderTag)
{
	UUserWidget* Widget = GetUserWidget();
	if (!IsValid(Widget))
	{
		return;
	}

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	MDViewModelUtils::SearchViewModelAssignments(Assignments, GetUserWidget()->GetClass(), ViewModelClass, ProviderTag);

	UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(ProviderTag);
	if (!IsValid(Provider))
	{
		return;
	}

	for (const auto& Pair : Assignments)
	{
		Provider->SetViewModel(*Widget, Pair.Key, Pair.Value);
	}
}

FDelegateHandle UMDViewModelWidgetExtension::ListenForChanges(FMDVMOnViewModelSet::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		return OnViewModelSetDelegates.FindOrAdd(Key).Add(MoveTemp(Delegate));
	}

	return {};
}

void UMDViewModelWidgetExtension::StopListeningForChanges(FDelegateHandle& Handle, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Key))
		{
			if (Delegate->Remove(Handle))
			{
				Handle.Reset();
			}
		}
	}
}

void UMDViewModelWidgetExtension::StopListeningForAllNativeViewModelsChanged(const void* BoundObject)
{
	for (auto& Pair : OnViewModelSetDelegates)
	{
		Pair.Value.RemoveAll(BoundObject);
	}
}

void UMDViewModelWidgetExtension::ListenForChanges(FMDVMOnViewModelSetDynamic&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		OnViewModelSetDynamicDelegates.FindOrAdd(Key).Add(MoveTemp(Delegate));
	}
}

void UMDViewModelWidgetExtension::StopListeningForChanges(const FMDVMOnViewModelSetDynamic& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Key))
		{
			Delegates->Remove(Delegate);
		}
	}
}

void UMDViewModelWidgetExtension::StopListeningForAllDynamicViewModelsChanged(const UObject* BoundObject)
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

bool UMDViewModelWidgetExtension::IsListeningForChanges(const UObject* BoundObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName) const
{
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (const FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Key))
		{
			if (Delegate->IsBoundToObject(BoundObject))
			{
				return true;
			}
		}
	}
	
	if (IsValid(ViewModelClass))
	{
		ViewModelName = MDViewModelUtils::ResolveViewModelName(ViewModelClass, ViewModelName);
		const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
		if (const TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Key))
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

void UMDViewModelWidgetExtension::BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	UUserWidget* OwnerWidget = GetUserWidget();
	if (IsValid(OldViewModel))
	{
		OldViewModel->OnUnsetFromWidget(OwnerWidget);
	}

	if (IsValid(NewViewModel))
	{
		NewViewModel->OnSetOnWidget(OwnerWidget);
	}

	const FMDViewModelInstanceKey Key = { ViewModelName, ViewModelClass };
	if (const FMDVMOnViewModelSet* Delegate = OnViewModelSetDelegates.Find(Key))
	{
		Delegate->Broadcast(OldViewModel, NewViewModel);
	}

	if (const TArray<FMDVMOnViewModelSetDynamic>* Delegates = OnViewModelSetDynamicDelegates.Find(Key))
	{
		for (const FMDVMOnViewModelSetDynamic& Delegate : *Delegates)
		{
			Delegate.ExecuteIfBound(OldViewModel, NewViewModel);
		}
	}
}

void UMDViewModelWidgetExtension::PopulateViewModels()
{
	UUserWidget* Widget = GetUserWidget();
	if (!IsValid(Widget))
	{
		return;
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 2
	// TODO - Support DebugMode with editable debug view models
	if (Widget->IsPreviewTime())
	{
		return;
	}
#endif

	UE_LOGFMT(LogMDViewModel, Verbose, "Populating View Models for Widget [{WidgetName}]",
		("WidgetName", GetUserWidget()->GetPathName()));
	
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
	MDViewModelUtils::GetViewModelAssignmentsForWidgetClass(Widget->GetClass(), Assignments);

	for (const auto& Pair : Assignments)
	{
		UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Pair.Key.ProviderTag);
		if (ensureMsgf(IsValid(Provider), TEXT("A View Model Provider with tag [%s] was not found"), *Pair.Key.ProviderTag.ToString()))
		{
			Provider->SetViewModel(*Widget, Pair.Key, Pair.Value);
			Provider->OnViewModelUpdated.AddUObject(this, &UMDViewModelWidgetExtension::OnProviderViewModelUpdated, Pair.Key.ProviderTag);
		}
	}
}

void UMDViewModelWidgetExtension::CleanUpViewModels()
{
	UE_LOGFMT(LogMDViewModel, Verbose, "Cleaning up View Models for Widget [{WidgetName}]",
		("WidgetName", GetUserWidget()->GetPathName()));
	
	// Broadcast out that we're null-ing out viewmodels
	{
		for (auto It = OnViewModelSetDelegates.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = ViewModels.FindRef(It.Key());
			if (IsValid(ViewModel))
			{
				It.Value().Broadcast(ViewModel, nullptr);
			}
		}

		for (auto It = OnViewModelSetDynamicDelegates.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = ViewModels.FindRef(It.Key());
			if (IsValid(ViewModel))
			{
				for (const FMDVMOnViewModelSetDynamic& Delegate : It.Value())
				{
					Delegate.ExecuteIfBound(ViewModel, nullptr);
				}
			}
		}
	}

	// Viewmodels themselves
	{
		UUserWidget* OwnerWidget = GetUserWidget();
		for (auto It = ViewModels.CreateConstIterator(); It; ++It)
		{
			UMDViewModelBase* ViewModel = It.Value();
			if (IsValid(ViewModel))
			{
				ViewModel->OnUnsetFromWidget(OwnerWidget);
			}
		}

		ViewModels.Reset();
	}
}
